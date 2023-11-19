/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TSDB feature.
 *
 * Time series log (like TSDB) feature implement source file.
 *
 * TSL is time series log, the TSDB saved many TSLs.
 */

#include <inttypes.h>
#include <string.h>
#include <flashdb.h>
#include <fdb_low_lvl.h>

#define FDB_LOG_TAG "[tsl]"
/* rewrite log prefix */
#undef  FDB_LOG_PREFIX2
#define FDB_LOG_PREFIX2()                         FDB_PRINT("[%s][%s] ", db_name(db), _fdb_db_path((fdb_db_t)db))

#if defined(FDB_USING_TSDB)

#if (FDB_WRITE_GRAN == 64) || (FDB_WRITE_GRAN == 128)
#error "Flash 64 or 128 bits write granularity is not supported in TSDB yet!"
#endif

/* magic word(`T`, `S`, `L`, `0`) */
#define SECTOR_MAGIC_WORD                        0x304C5354

#define TSL_STATUS_TABLE_SIZE                    FDB_STATUS_TABLE_SIZE(FDB_TSL_STATUS_NUM)

#define SECTOR_HDR_DATA_SIZE                     (FDB_WG_ALIGN(sizeof(struct sector_hdr_data)))
#define LOG_IDX_DATA_SIZE                        (FDB_WG_ALIGN(sizeof(struct log_idx_data)))
#define LOG_IDX_TS_OFFSET                        ((unsigned long)(&((struct log_idx_data *)0)->time))
#define SECTOR_MAGIC_OFFSET                      ((unsigned long)(&((struct sector_hdr_data *)0)->magic))
#define SECTOR_START_TIME_OFFSET                 ((unsigned long)(&((struct sector_hdr_data *)0)->start_time))
#define SECTOR_END0_TIME_OFFSET                  ((unsigned long)(&((struct sector_hdr_data *)0)->end_info[0].time))
#define SECTOR_END0_IDX_OFFSET                   ((unsigned long)(&((struct sector_hdr_data *)0)->end_info[0].index))
#define SECTOR_END0_STATUS_OFFSET                ((unsigned long)(&((struct sector_hdr_data *)0)->end_info[0].status))
#define SECTOR_END1_TIME_OFFSET                  ((unsigned long)(&((struct sector_hdr_data *)0)->end_info[1].time))
#define SECTOR_END1_IDX_OFFSET                   ((unsigned long)(&((struct sector_hdr_data *)0)->end_info[1].index))
#define SECTOR_END1_STATUS_OFFSET                ((unsigned long)(&((struct sector_hdr_data *)0)->end_info[1].status))

/* the next address is get failed */
#define FAILED_ADDR                              0xFFFFFFFF

#define db_name(db)                              (((fdb_db_t)db)->name)
#define db_init_ok(db)                           (((fdb_db_t)db)->init_ok)
#define db_sec_size(db)                          (((fdb_db_t)db)->sec_size)
#define db_max_size(db)                          (((fdb_db_t)db)->max_size)
#define db_oldest_addr(db)                       (((fdb_db_t)db)->oldest_addr)

#define db_lock(db)                                                            \
    do {                                                                       \
        if (((fdb_db_t)db)->lock) ((fdb_db_t)db)->lock((fdb_db_t)db);          \
    } while(0);

#define db_unlock(db)                                                          \
    do {                                                                       \
        if (((fdb_db_t)db)->unlock) ((fdb_db_t)db)->unlock((fdb_db_t)db);      \
    } while(0);

#define _FDB_WRITE_STATUS(db, addr, status_table, status_num, status_index, sync)    \
    do {                                                                       \
        result = _fdb_write_status((fdb_db_t)db, addr, status_table, status_num, status_index, sync);\
        if (result != FDB_NO_ERR) return result;                               \
    } while(0);

#define FLASH_WRITE(db, addr, buf, size, sync)                                 \
    do {                                                                       \
        result = _fdb_flash_write((fdb_db_t)db, addr, buf, size, sync);        \
        if (result != FDB_NO_ERR) return result;                               \
    } while(0);

struct sector_hdr_data {
    uint8_t status[FDB_STORE_STATUS_TABLE_SIZE]; /**< sector store status @see fdb_sector_store_status_t */
    uint32_t magic;                              /**< magic word(`T`, `S`, `L`, `0`) */
    fdb_time_t start_time;                       /**< the first start node's timestamp */
    struct {
        fdb_time_t time;                         /**< the last end node's timestamp */
        uint32_t index;                          /**< the last end node's index */
        uint8_t status[TSL_STATUS_TABLE_SIZE];   /**< end node status, @see fdb_tsl_status_t */
    } end_info[2];
    uint32_t reserved;
};
typedef struct sector_hdr_data *sector_hdr_data_t;

/* time series log node index data */
struct log_idx_data {
    uint8_t status_table[TSL_STATUS_TABLE_SIZE]; /**< node status, @see fdb_tsl_status_t */
    fdb_time_t time;                             /**< node timestamp */
    uint32_t log_len;                            /**< node total length (header + name + value), must align by FDB_WRITE_GRAN */
    uint32_t log_addr;                           /**< node address */
};
typedef struct log_idx_data *log_idx_data_t;

struct query_count_args {
    fdb_tsl_status_t status;
    size_t count;
};

struct check_sec_hdr_cb_args {
    fdb_tsdb_t db;
    bool check_failed;
    size_t empty_num;
    uint32_t empty_addr;
};

static fdb_err_t read_tsl(fdb_tsdb_t db, fdb_tsl_t tsl)
{
    struct log_idx_data idx;
    /* read TSL index raw data */
    _fdb_flash_read((fdb_db_t)db, tsl->addr.index, (uint32_t *) &idx, sizeof(struct log_idx_data));
    tsl->status = (fdb_tsl_status_t) _fdb_get_status(idx.status_table, FDB_TSL_STATUS_NUM);
    if ((tsl->status == FDB_TSL_PRE_WRITE) || (tsl->status == FDB_TSL_UNUSED)) {
        tsl->log_len = db->max_len;
        tsl->addr.log = FDB_DATA_UNUSED;
        tsl->time = 0;
    } else {
        tsl->log_len = idx.log_len;
        tsl->addr.log = idx.log_addr;
        tsl->time = idx.time;
    }

    return FDB_NO_ERR;
}

static uint32_t get_next_sector_addr(fdb_tsdb_t db, tsdb_sec_info_t pre_sec, uint32_t traversed_len)
{
    if (traversed_len + db_sec_size(db) <= db_max_size(db)) {
        if (pre_sec->addr + db_sec_size(db) < db_max_size(db)) {
            return pre_sec->addr + db_sec_size(db);
        } else {
            /* the next sector is on the top of the database */
            return 0;
        }
    } else {
        /* finished */
        return FAILED_ADDR;
    }
}

static uint32_t get_next_tsl_addr(tsdb_sec_info_t sector, fdb_tsl_t pre_tsl)
{
    uint32_t addr = FAILED_ADDR;

    if (sector->status == FDB_SECTOR_STORE_EMPTY) {
        return FAILED_ADDR;
    }

    if (pre_tsl->addr.index + LOG_IDX_DATA_SIZE <= sector->end_idx) {
        addr = pre_tsl->addr.index + LOG_IDX_DATA_SIZE;
    } else {
        /* no TSL */
        return FAILED_ADDR;
    }

    return addr;
}

static uint32_t get_last_tsl_addr(tsdb_sec_info_t sector, fdb_tsl_t pre_tsl)
{
    uint32_t addr = FAILED_ADDR;

    if (sector->status == FDB_SECTOR_STORE_EMPTY) {
        return FAILED_ADDR;
    }

    if (pre_tsl->addr.index >= (sector->addr + SECTOR_HDR_DATA_SIZE + LOG_IDX_DATA_SIZE)) {
        addr = pre_tsl->addr.index - LOG_IDX_DATA_SIZE;
    } else {
        return FAILED_ADDR;
    }

    return addr;
}

static uint32_t get_last_sector_addr(fdb_tsdb_t db, tsdb_sec_info_t pre_sec, uint32_t traversed_len)
{
    if (traversed_len + db_sec_size(db) <= db_max_size(db)) {
        if (pre_sec->addr >= db_sec_size(db)) {
            /* the next sector is previous sector */
            return pre_sec->addr - db_sec_size(db);
        } else {
            /* the next sector is the last sector */
            return db_max_size(db) - db_sec_size(db);
        }
    } else {
        return FAILED_ADDR;
    }
}

static fdb_err_t read_sector_info(fdb_tsdb_t db, uint32_t addr, tsdb_sec_info_t sector, bool traversal)
{
    fdb_err_t result = FDB_NO_ERR;
    struct sector_hdr_data sec_hdr;

    FDB_ASSERT(sector);

    /* read sector header raw data */
    _fdb_flash_read((fdb_db_t)db, addr, (uint32_t *)&sec_hdr, sizeof(struct sector_hdr_data));

    sector->addr = addr;
    sector->magic = sec_hdr.magic;

    /* check magic word */
    if (sector->magic != SECTOR_MAGIC_WORD) {
        sector->check_ok = false;
        return FDB_INIT_FAILED;
    }
    sector->check_ok = true;
    sector->status = (fdb_sector_store_status_t) _fdb_get_status(sec_hdr.status, FDB_SECTOR_STORE_STATUS_NUM);
    sector->start_time = sec_hdr.start_time;
    sector->end_info_stat[0] = (fdb_tsl_status_t) _fdb_get_status(sec_hdr.end_info[0].status, FDB_TSL_STATUS_NUM);
    sector->end_info_stat[1] = (fdb_tsl_status_t) _fdb_get_status(sec_hdr.end_info[1].status, FDB_TSL_STATUS_NUM);
    if (sector->end_info_stat[0] == FDB_TSL_WRITE) {
        sector->end_time = sec_hdr.end_info[0].time;
        sector->end_idx = sec_hdr.end_info[0].index;
    } else if (sector->end_info_stat[1] == FDB_TSL_WRITE) {
        sector->end_time = sec_hdr.end_info[1].time;
        sector->end_idx = sec_hdr.end_info[1].index;
    } else if (sector->end_info_stat[0] == FDB_TSL_PRE_WRITE && sector->end_info_stat[1] == FDB_TSL_PRE_WRITE) {
        //TODO There is no valid end node info on this sector, need impl fast query this sector by fdb_tsl_iter_by_time
        FDB_ASSERT(0);
    }
    /* traversal all TSL and calculate the remain space size */
    sector->empty_idx = sector->addr + SECTOR_HDR_DATA_SIZE;
    sector->empty_data = sector->addr + db_sec_size(db);
    /* the TSL's data is saved from sector bottom, and the TSL's index saved from the sector top */
    sector->remain = sector->empty_data - sector->empty_idx;
    if (sector->status == FDB_SECTOR_STORE_USING && traversal) {
        struct fdb_tsl tsl;

        tsl.addr.index = sector->empty_idx;
        while (read_tsl(db, &tsl) == FDB_NO_ERR) {
            if (tsl.status == FDB_TSL_UNUSED) {
                break;
            }
            sector->end_time = tsl.time;
            sector->end_idx = tsl.addr.index;
            sector->empty_idx += LOG_IDX_DATA_SIZE;
            sector->empty_data -= FDB_WG_ALIGN(tsl.log_len);
            tsl.addr.index += LOG_IDX_DATA_SIZE;
            if (sector->remain > LOG_IDX_DATA_SIZE + FDB_WG_ALIGN(tsl.log_len)) {
                sector->remain -= (LOG_IDX_DATA_SIZE + FDB_WG_ALIGN(tsl.log_len));
            } else {
                FDB_INFO("Error: this TSL (0x%08" PRIX32 ") size (%" PRIu32 ") is out of bound.\n", tsl.addr.index, tsl.log_len);
                sector->remain = 0;
                result = FDB_READ_ERR;
                break;
            }
        }
    }

    return result;
}

static fdb_err_t format_sector(fdb_tsdb_t db, uint32_t addr)
{
    fdb_err_t result = FDB_NO_ERR;
    struct sector_hdr_data sec_hdr;

    FDB_ASSERT(addr % db_sec_size(db) == 0);

    result = _fdb_flash_erase((fdb_db_t)db, addr, db_sec_size(db));
    if (result == FDB_NO_ERR) {
        _FDB_WRITE_STATUS(db, addr, sec_hdr.status, FDB_SECTOR_STORE_STATUS_NUM, FDB_SECTOR_STORE_EMPTY, true);
        /* set the magic */
        sec_hdr.magic = SECTOR_MAGIC_WORD;
        FLASH_WRITE(db, addr + SECTOR_MAGIC_OFFSET, &sec_hdr.magic, sizeof(sec_hdr.magic), true);
    }

    return result;
}

static void sector_iterator(fdb_tsdb_t db, tsdb_sec_info_t sector, fdb_sector_store_status_t status, void *arg1,
        void *arg2, bool (*callback)(tsdb_sec_info_t sector, void *arg1, void *arg2), bool traversal)
{
    uint32_t sec_addr = sector->addr, traversed_len = 0;

    /* search all sectors */
    do {
        read_sector_info(db, sec_addr, sector, false);
        if (status == FDB_SECTOR_STORE_UNUSED || status == sector->status) {
            if (traversal) {
                read_sector_info(db, sec_addr, sector, true);
            }
            /* iterator is interrupted when callback return true */
            if (callback && callback(sector, arg1, arg2)) {
                return;
            }
        }
        traversed_len += db_sec_size(db);
    } while ((sec_addr = get_next_sector_addr(db, sector, traversed_len)) != FAILED_ADDR);
}

static fdb_err_t write_tsl(fdb_tsdb_t db, fdb_blob_t blob, fdb_time_t time)
{
    fdb_err_t result = FDB_NO_ERR;
    struct log_idx_data idx;
    uint32_t idx_addr = db->cur_sec.empty_idx;

    idx.log_len = blob->size;
    idx.time = time;
    idx.log_addr = db->cur_sec.empty_data - FDB_WG_ALIGN(idx.log_len);
    /* write the status will by write granularity */
    _FDB_WRITE_STATUS(db, idx_addr, idx.status_table, FDB_TSL_STATUS_NUM, FDB_TSL_PRE_WRITE, false);
    /* write other index info */
    FLASH_WRITE(db, idx_addr + LOG_IDX_TS_OFFSET, &idx.time,  sizeof(struct log_idx_data) - LOG_IDX_TS_OFFSET, false);
    /* write blob data */
    FLASH_WRITE(db, idx.log_addr, blob->buf, blob->size, false);
    /* write the status will by write granularity */
    _FDB_WRITE_STATUS(db, idx_addr, idx.status_table, FDB_TSL_STATUS_NUM, FDB_TSL_WRITE, true);

    return result;
}

static fdb_err_t update_sec_status(fdb_tsdb_t db, tsdb_sec_info_t sector, fdb_blob_t blob, fdb_time_t cur_time)
{
    fdb_err_t result = FDB_NO_ERR;
    uint8_t status[FDB_STORE_STATUS_TABLE_SIZE];

    if (sector->status == FDB_SECTOR_STORE_USING && sector->remain < LOG_IDX_DATA_SIZE + FDB_WG_ALIGN(blob->size)) {
        uint8_t end_status[TSL_STATUS_TABLE_SIZE];
        uint32_t end_index = sector->empty_idx - LOG_IDX_DATA_SIZE, new_sec_addr, cur_sec_addr = sector->addr;
        /* save the end node index and timestamp */
        if (sector->end_info_stat[0] == FDB_TSL_UNUSED) {
            _FDB_WRITE_STATUS(db, cur_sec_addr + SECTOR_END0_STATUS_OFFSET, end_status, FDB_TSL_STATUS_NUM, FDB_TSL_PRE_WRITE, false);
            FLASH_WRITE(db, cur_sec_addr + SECTOR_END0_TIME_OFFSET, (uint32_t * )&db->last_time, sizeof(fdb_time_t), false);
            FLASH_WRITE(db, cur_sec_addr + SECTOR_END0_IDX_OFFSET, &end_index, sizeof(end_index), false);
            _FDB_WRITE_STATUS(db, cur_sec_addr + SECTOR_END0_STATUS_OFFSET, end_status, FDB_TSL_STATUS_NUM, FDB_TSL_WRITE, true);
        } else if (sector->end_info_stat[1] == FDB_TSL_UNUSED) {
            _FDB_WRITE_STATUS(db, cur_sec_addr + SECTOR_END1_STATUS_OFFSET, end_status, FDB_TSL_STATUS_NUM, FDB_TSL_PRE_WRITE, false);
            FLASH_WRITE(db, cur_sec_addr + SECTOR_END1_TIME_OFFSET, (uint32_t * )&db->last_time, sizeof(fdb_time_t), false);
            FLASH_WRITE(db, cur_sec_addr + SECTOR_END1_IDX_OFFSET, &end_index, sizeof(end_index), false);
            _FDB_WRITE_STATUS(db, cur_sec_addr + SECTOR_END1_STATUS_OFFSET, end_status, FDB_TSL_STATUS_NUM, FDB_TSL_WRITE, true);
        }
        /* change current sector to full */
        _FDB_WRITE_STATUS(db, cur_sec_addr, status, FDB_SECTOR_STORE_STATUS_NUM, FDB_SECTOR_STORE_FULL, true);
        sector->status = FDB_SECTOR_STORE_FULL;
        /* calculate next sector address */
        if (sector->addr + db_sec_size(db) < db_max_size(db)) {
            new_sec_addr = sector->addr + db_sec_size(db);
        }
        else if (db->rollover) {
            new_sec_addr = 0;
        } else {
            /* not rollover */
            return FDB_SAVED_FULL;
        }
        read_sector_info(db, new_sec_addr, &db->cur_sec, false);
        if (sector->status != FDB_SECTOR_STORE_EMPTY) {
            /* calculate the oldest sector address */
            if (new_sec_addr + db_sec_size(db) < db_max_size(db)) {
                db_oldest_addr(db) = new_sec_addr + db_sec_size(db);
            } else {
                db_oldest_addr(db) = 0;
            }
            format_sector(db, new_sec_addr);
            read_sector_info(db, new_sec_addr, &db->cur_sec, false);
        }
    } else if (sector->status == FDB_SECTOR_STORE_FULL) {
        /* database full */
        return FDB_SAVED_FULL;
    }

    if (sector->status == FDB_SECTOR_STORE_EMPTY) {
        /* change the sector to using */
        sector->status = FDB_SECTOR_STORE_USING;
        sector->start_time = cur_time;
        _FDB_WRITE_STATUS(db, sector->addr, status, FDB_SECTOR_STORE_STATUS_NUM, FDB_SECTOR_STORE_USING, true);
        /* save the start timestamp */
        FLASH_WRITE(db, sector->addr + SECTOR_START_TIME_OFFSET, (uint32_t *)&cur_time, sizeof(fdb_time_t), true);
    }

    return result;
}

static fdb_err_t tsl_append(fdb_tsdb_t db, fdb_blob_t blob, fdb_time_t *timestamp)
{
    fdb_err_t result = FDB_NO_ERR;
    fdb_time_t cur_time = timestamp == NULL ? db->get_time() : *timestamp;

    /* check the append length, MUST less than the db->max_len */
    if(blob->size > db->max_len)
    {
        FDB_INFO("Warning: append length (%" PRIdMAX ") is more than the db->max_len (%" PRIdMAX "). This tsl will be dropped.\n", 
                (intmax_t)blob->size, (intmax_t)(db->max_len));
        return FDB_WRITE_ERR;
    }

    /* check the current timestamp, MUST more than the last save timestamp */
    if (cur_time <= db->last_time) {
        FDB_INFO("Warning: current timestamp (%" PRIdMAX ") is less than or equal to the last save timestamp (%" PRIdMAX "). This tsl will be dropped.\n",
                (intmax_t )cur_time, (intmax_t )(db->last_time));
        return FDB_WRITE_ERR;
    }

    result = update_sec_status(db, &db->cur_sec, blob, cur_time);
    if (result != FDB_NO_ERR) {
        FDB_INFO("Error: update the sector status failed (%d)", result);
        return result;
    }
    /* write the TSL node */
    result = write_tsl(db, blob, cur_time);
    if (result != FDB_NO_ERR) {
        FDB_INFO("Error: write tsl failed (%d)", result);
        return result;
    }

    /* recalculate the current using sector info */
    db->cur_sec.end_idx = db->cur_sec.empty_idx;
    db->cur_sec.end_time = cur_time;
    db->cur_sec.empty_idx += LOG_IDX_DATA_SIZE;
    db->cur_sec.empty_data -= FDB_WG_ALIGN(blob->size);
    db->cur_sec.remain -= LOG_IDX_DATA_SIZE + FDB_WG_ALIGN(blob->size);
    db->last_time = cur_time;

    return result;
}

/**
 * Append a new log to TSDB.
 *
 * @param db database object
 * @param blob log blob data
 *
 * @return result
 */
fdb_err_t fdb_tsl_append(fdb_tsdb_t db, fdb_blob_t blob)
{
    fdb_err_t result = FDB_NO_ERR;

    if (!db_init_ok(db)) {
        FDB_INFO("Error: TSL (%s) isn't initialize OK.\n", db_name(db));
        return FDB_INIT_FAILED;
    }

    db_lock(db);
    result = tsl_append(db, blob, NULL);
    db_unlock(db);

    return result;
}

/**
 * Append a new log to TSDB with specific timestamp.
 *
 * @param db database object
 * @param blob log blob data
 *
 * @return result
 */
fdb_err_t fdb_tsl_append_with_ts(fdb_tsdb_t db, fdb_blob_t blob, fdb_time_t timestamp)
{
    fdb_err_t result = FDB_NO_ERR;

    if (!db_init_ok(db)) {
        FDB_INFO("Error: TSL (%s) isn't initialize OK.\n", db_name(db));
        return FDB_INIT_FAILED;
    }

    db_lock(db);
    result = tsl_append(db, blob, &timestamp);
    db_unlock(db);

    return result;
}

/**
 * The TSDB iterator for each TSL.
 *
 * @param db database object
 * @param cb callback
 * @param arg callback argument
 */
void fdb_tsl_iter(fdb_tsdb_t db, fdb_tsl_cb cb, void *arg)
{
    struct tsdb_sec_info sector;
    uint32_t sec_addr, traversed_len = 0;
    struct fdb_tsl tsl;

    if (!db_init_ok(db)) {
        FDB_INFO("Error: TSL (%s) isn't initialize OK.\n", db_name(db));
    }

    if (cb == NULL) {
        return;
    }

    sec_addr = db_oldest_addr(db);
	db_lock(db);
    /* search all sectors */
    do {
        traversed_len += db_sec_size(db);
        if (read_sector_info(db, sec_addr, &sector, false) != FDB_NO_ERR) {
            continue;
        }
        /* sector has TSL */
        if (sector.status == FDB_SECTOR_STORE_USING || sector.status == FDB_SECTOR_STORE_FULL) {
            if (sector.status == FDB_SECTOR_STORE_USING) {
                /* copy the current using sector status  */
                sector = db->cur_sec;
            }
            tsl.addr.index = sector.addr + SECTOR_HDR_DATA_SIZE;
            /* search all TSL */
            do {
                read_tsl(db, &tsl);
                /* iterator is interrupted when callback return true */
                if (cb(&tsl, arg)) {
                    db_unlock(db);
                    return;
                }
            } while ((tsl.addr.index = get_next_tsl_addr(&sector, &tsl)) != FAILED_ADDR);
        }
    } while ((sec_addr = get_next_sector_addr(db, &sector, traversed_len)) != FAILED_ADDR);
    db_unlock(db);
}

/**
 * The TSDB iterator for each TSL.
 *
 * @param db database object
 * @param cb callback
 * @param arg callback argument
 */
void fdb_tsl_iter_reverse(fdb_tsdb_t db, fdb_tsl_cb cb, void *cb_arg)
{
    struct tsdb_sec_info sector;
    uint32_t sec_addr, traversed_len = 0;
    struct fdb_tsl tsl;

    if (!db_init_ok(db)) {
        FDB_INFO("Error: TSL (%s) isn't initialize OK.\n", db_name(db));
    }

    if (cb == NULL) {
        return;
    }

    sec_addr = db->cur_sec.addr;
    db_lock(db);
    /* search all sectors */
    do {
        traversed_len += db_sec_size(db);
        if (read_sector_info(db, sec_addr, &sector, false) != FDB_NO_ERR) {
            continue;
        }
        /* sector has TSL */
        if (sector.status == FDB_SECTOR_STORE_USING || sector.status == FDB_SECTOR_STORE_FULL) {
            if (sector.status == FDB_SECTOR_STORE_USING) {
                /* copy the current using sector status  */
                sector = db->cur_sec;
            }
            tsl.addr.index = sector.end_idx;
            /* search all TSL */
            do {
                read_tsl(db, &tsl);
                /* iterator is interrupted when callback return true */
                if (cb(&tsl, cb_arg)) {
                    goto __exit;
                }
            } while ((tsl.addr.index = get_last_tsl_addr(&sector, &tsl)) != FAILED_ADDR);
        } else if (sector.status == FDB_SECTOR_STORE_EMPTY || sector.status == FDB_SECTOR_STORE_UNUSED)
            goto __exit;
    } while ((sec_addr = get_last_sector_addr(db, &sector, traversed_len)) != FAILED_ADDR);

__exit:
    db_unlock(db);
}

/*
 * Found the matched TSL address.
 */
static int search_start_tsl_addr(fdb_tsdb_t db, int start, int end, fdb_time_t from, fdb_time_t to)
{
    struct fdb_tsl tsl;
    while (true) {
        tsl.addr.index = start + FDB_ALIGN((end - start) / 2, LOG_IDX_DATA_SIZE);
        read_tsl(db, &tsl);
        if (tsl.time < from) {
            start = tsl.addr.index + LOG_IDX_DATA_SIZE;
        } else if (tsl.time > from) {
            end = tsl.addr.index - LOG_IDX_DATA_SIZE;
        } else {
            return tsl.addr.index;
        }

        if (start > end) {
            if (from > to) {
                tsl.addr.index = start;
                read_tsl(db, &tsl);
                if (tsl.time > from) {
                    start -= LOG_IDX_DATA_SIZE;
                }
            }
            break;
        }
    }
    return start;
}

/**
 * The TSDB iterator for each TSL by timestamp.
 *
 * @param db database object
 * @param from starting timestamp. It will be a reverse iterator when ending timestamp less than starting timestamp
 * @param to ending timestamp
 * @param cb callback
 * @param arg callback argument
 */
void fdb_tsl_iter_by_time(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_cb cb, void *cb_arg)
{
    struct tsdb_sec_info sector;
    uint32_t sec_addr, start_addr, traversed_len = 0;
    struct fdb_tsl tsl;
    bool found_start_tsl = false;

    uint32_t (*get_sector_addr)(fdb_tsdb_t , tsdb_sec_info_t , uint32_t);
    uint32_t (*get_tsl_addr)(tsdb_sec_info_t , fdb_tsl_t);

    if (!db_init_ok(db)) {
        FDB_INFO("Error: TSL (%s) isn't initialize OK.\n", db_name(db));
    }

    if(from <= to) {
        start_addr = db_oldest_addr(db);
        get_sector_addr = get_next_sector_addr;
        get_tsl_addr = get_next_tsl_addr;
    } else {
        start_addr = db->cur_sec.addr;
        get_sector_addr = get_last_sector_addr;
        get_tsl_addr = get_last_tsl_addr;
    }

//    FDB_INFO("from %s", ctime((const time_t * )&from));
//    FDB_INFO("to %s", ctime((const time_t * )&to));

    if (cb == NULL) {
        return;
    }

    sec_addr = start_addr;
    db_lock(db);
    /* search all sectors */
    do {
        traversed_len += db_sec_size(db);
        if (read_sector_info(db, sec_addr, &sector, false) != FDB_NO_ERR) {
            continue;
        }
        /* sector has TSL */
        if ((sector.status == FDB_SECTOR_STORE_USING || sector.status == FDB_SECTOR_STORE_FULL)) {
            if (sector.status == FDB_SECTOR_STORE_USING) {
                /* copy the current using sector status  */
                sector = db->cur_sec;
            }
            if ((found_start_tsl)
                    || (!found_start_tsl &&
                            ((from <= to && ((sec_addr == start_addr && from <= sector.start_time) || from <= sector.end_time)) ||
                             (from > to  && ((sec_addr == start_addr && from >= sector.end_time) || from >= sector.start_time)))
                             )) {
                uint32_t start = sector.addr + SECTOR_HDR_DATA_SIZE, end = sector.end_idx;

                found_start_tsl = true;
                /* search the first start TSL address */
                tsl.addr.index = search_start_tsl_addr(db, start, end, from, to);
                /* search all TSL */
                do {
                    read_tsl(db, &tsl);
                    if (tsl.status != FDB_TSL_UNUSED) {
                        if ((from <= to && tsl.time >= from && tsl.time <= to)
                                || (from > to && tsl.time <= from && tsl.time >= to)) {
                            /* iterator is interrupted when callback return true */
                            if (cb(&tsl, cb_arg)) {
                                goto __exit;
                            }
                        } else {
                            goto __exit;
                        }
                    }
                } while ((tsl.addr.index = get_tsl_addr(&sector, &tsl)) != FAILED_ADDR);
            }
        } else if (sector.status == FDB_SECTOR_STORE_EMPTY) {
            goto __exit;
        }
    } while ((sec_addr = get_sector_addr(db, &sector, traversed_len)) != FAILED_ADDR);

__exit:
    db_unlock(db);
}

static bool query_count_cb(fdb_tsl_t tsl, void *arg)
{
    struct query_count_args *args = arg;

    if (tsl->status == args->status) {
        args->count++;
    }

    return false;
}

/**
 * Query some TSL's count by timestamp and status.
 *
 * @param db database object
 * @param from starting timestamp
 * @param to ending timestamp
 * @param status status
 */
size_t fdb_tsl_query_count(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_status_t status)
{
    struct query_count_args arg = { FDB_TSL_UNUSED, 0 };

    arg.status = status;

    if (!db_init_ok(db)) {
        FDB_INFO("Error: TSL (%s) isn't initialize OK.\n", db_name(db));
        return 0;
    }

    fdb_tsl_iter_by_time(db, from, to, query_count_cb, &arg);

    return arg.count;

}

/**
 * Set the TSL status.
 *
 * @param db database object
 * @param tsl TSL object
 * @param status status
 *
 * @return result
 */
fdb_err_t fdb_tsl_set_status(fdb_tsdb_t db, fdb_tsl_t tsl, fdb_tsl_status_t status)
{
    fdb_err_t result = FDB_NO_ERR;
    uint8_t status_table[TSL_STATUS_TABLE_SIZE];

    /* write the status will by write granularity */
    _FDB_WRITE_STATUS(db, tsl->addr.index, status_table, FDB_TSL_STATUS_NUM, status, true);

    return result;
}

/**
 * Convert the TSL object to blob object
 *
 * @param tsl TSL object
 * @param blob blob object
 *
 * @return new blob object
 */
fdb_blob_t fdb_tsl_to_blob(fdb_tsl_t tsl, fdb_blob_t blob)
{
    blob->saved.addr = tsl->addr.log;
    blob->saved.meta_addr = tsl->addr.index;
    blob->saved.len = tsl->log_len;

    return blob;
}

static bool check_sec_hdr_cb(tsdb_sec_info_t sector, void *arg1, void *arg2)
{
    struct check_sec_hdr_cb_args *arg = arg1;
    fdb_tsdb_t db = arg->db;

    if (!sector->check_ok) {
        FDB_INFO("Sector (0x%08" PRIX32 ") header info is incorrect.\n", sector->addr);
        (arg->check_failed) = true;
        return true;
    } else if (sector->status == FDB_SECTOR_STORE_USING) {
        if (db->cur_sec.addr == FDB_DATA_UNUSED) {
            memcpy(&db->cur_sec, sector, sizeof(struct tsdb_sec_info));
        } else {
            FDB_INFO("Warning: Sector status is wrong, there are multiple sectors in use.\n");
            (arg->check_failed) = true;
            return true;
        }
    } else if (sector->status == FDB_SECTOR_STORE_EMPTY) {
        (arg->empty_num) += 1;
        arg->empty_addr = sector->addr;
        if ((arg->empty_num) == 1 && db->cur_sec.addr == FDB_DATA_UNUSED) {
            memcpy(&db->cur_sec, sector, sizeof(struct tsdb_sec_info));
        }
    }

    return false;
}
static bool format_all_cb(tsdb_sec_info_t sector, void *arg1, void *arg2)
{
    fdb_tsdb_t db = arg1;

    format_sector(db, sector->addr);

    return false;
}

static void tsl_format_all(fdb_tsdb_t db)
{
    struct tsdb_sec_info sector;

    sector.addr = 0;
    sector_iterator(db, &sector, FDB_SECTOR_STORE_UNUSED, db, NULL, format_all_cb, false);
    db_oldest_addr(db) = 0;
    db->cur_sec.addr = 0;
    db->last_time = 0;
    /* read the current using sector info */
    read_sector_info(db, db->cur_sec.addr, &db->cur_sec, false);

    FDB_INFO("All sector format finished.\n");
}

/**
 * Clean all the data in the TSDB.
 *
 * @note It's DANGEROUS. This operation is not reversible.
 *
 * @param db database object
 */
void fdb_tsl_clean(fdb_tsdb_t db)
{
    db_lock(db);
    tsl_format_all(db);
    db_unlock(db);
}

/**
 * This function will get or set some options of the database
 *
 * @param db database object
 * @param cmd the control command
 * @param arg the argument
 */
void fdb_tsdb_control(fdb_tsdb_t db, int cmd, void *arg)
{
    FDB_ASSERT(db);

    switch (cmd) {
    case FDB_TSDB_CTRL_SET_SEC_SIZE:
        /* this change MUST before database initialization */
        FDB_ASSERT(db->parent.init_ok == false);
        db->parent.sec_size = *(uint32_t *)arg;
        break;
    case FDB_TSDB_CTRL_GET_SEC_SIZE:
        *(uint32_t *)arg = db->parent.sec_size;
        break;
    case FDB_TSDB_CTRL_SET_LOCK:
#if !defined(__ARMCC_VERSION) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
        db->parent.lock = (void (*)(fdb_db_t db))arg;
#if !defined(__ARMCC_VERSION) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        break;
    case FDB_TSDB_CTRL_SET_UNLOCK:
#if !defined(__ARMCC_VERSION) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
        db->parent.unlock = (void (*)(fdb_db_t db))arg;
#if !defined(__ARMCC_VERSION) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        break;
    case FDB_TSDB_CTRL_SET_ROLLOVER:
        /* this change MUST after database initialized */
        FDB_ASSERT(db->parent.init_ok == true);
        db->rollover = *(bool *)arg;
        break;
    case FDB_TSDB_CTRL_GET_ROLLOVER:
        *(bool *)arg = db->rollover;
        break;
    case FDB_TSDB_CTRL_GET_LAST_TIME:
        *(fdb_time_t *)arg = db->last_time;
        break;
    case FDB_TSDB_CTRL_SET_FILE_MODE:
#ifdef FDB_USING_FILE_MODE
        /* this change MUST before database initialization */
        FDB_ASSERT(db->parent.init_ok == false);
        db->parent.file_mode = *(bool *)arg;
#else
        FDB_INFO("Error: set file mode Failed. Please defined the FDB_USING_FILE_MODE macro.");
#endif
        break;
    case FDB_TSDB_CTRL_SET_MAX_SIZE:
#ifdef FDB_USING_FILE_MODE
        /* this change MUST before database initialization */
        FDB_ASSERT(db->parent.init_ok == false);
        db->parent.max_size = *(uint32_t *)arg;
#endif
        break;
    case FDB_TSDB_CTRL_SET_NOT_FORMAT:
        /* this change MUST before database initialization */
        FDB_ASSERT(db->parent.init_ok == false);
        db->parent.not_formatable = *(bool *)arg;
        break;
    }
}

/**
 * The time series database initialization.
 *
 * @param db database object
 * @param name database name
 * @param path FAL mode: partition name, file mode: database saved directory path
 * @param get_time get current time function
 * @param max_len maximum length of each log
 * @param user_data user data
 *
 * @return result
 */
fdb_err_t fdb_tsdb_init(fdb_tsdb_t db, const char *name, const char *path, fdb_get_time get_time, size_t max_len, void *user_data)
{
    fdb_err_t result = FDB_NO_ERR;
    struct tsdb_sec_info sector;
    struct check_sec_hdr_cb_args check_sec_arg = { db, false, 0, 0};

    FDB_ASSERT(get_time);

    result = _fdb_init_ex((fdb_db_t)db, name, path, FDB_DB_TYPE_TS, user_data);
    if (result != FDB_NO_ERR) {
        goto __exit;
    }

    /* lock the TSDB */
    db_lock(db);

    db->get_time = get_time;
    db->max_len = max_len;
    /* default rollover flag is true */
    db->rollover = true;
    db_oldest_addr(db) = FDB_DATA_UNUSED;
    db->cur_sec.addr = FDB_DATA_UNUSED;
    /* must less than sector size */
    FDB_ASSERT(max_len < db_sec_size(db));

    /* check all sector header */
    sector.addr = 0;
    sector_iterator(db, &sector, FDB_SECTOR_STORE_UNUSED, &check_sec_arg, NULL, check_sec_hdr_cb, true);
    /* format all sector when check failed */
    if (check_sec_arg.check_failed) {
        if (db->parent.not_formatable) {
            result = FDB_READ_ERR;
            goto __exit;
        } else {
            tsl_format_all(db);
        }
    } else {
        uint32_t latest_addr;
        if (check_sec_arg.empty_num > 0) {
            latest_addr = check_sec_arg.empty_addr;
        } else {
            if (db->rollover) {
                latest_addr = db->cur_sec.addr;
            } else {
                /* There is no empty sector. */
                latest_addr = db->cur_sec.addr = db_max_size(db) - db_sec_size(db);
            }
        }
        /* db->cur_sec is the latest sector, and the next is the oldest sector */
        if (latest_addr + db_sec_size(db) >= db_max_size(db)) {
            /* db->cur_sec is the the bottom of the database */
            db_oldest_addr(db) = 0;
        } else {
            db_oldest_addr(db) = latest_addr + db_sec_size(db);
        }
    }
    FDB_DEBUG("TSDB (%s) oldest sectors is 0x%08" PRIX32 ", current using sector is 0x%08" PRIX32 ".\n", db_name(db), db_oldest_addr(db),
            db->cur_sec.addr);
    /* read the current using sector info */
    read_sector_info(db, db->cur_sec.addr, &db->cur_sec, true);
    /* get last save time */
    if (db->cur_sec.status == FDB_SECTOR_STORE_USING) {
        db->last_time = db->cur_sec.end_time;
    } else if (db->cur_sec.status == FDB_SECTOR_STORE_EMPTY && db_oldest_addr(db) != db->cur_sec.addr) {
        struct tsdb_sec_info sec;
        uint32_t addr = db->cur_sec.addr;

        if (addr == 0) {
            addr = db_max_size(db) - db_sec_size(db);
        } else {
            addr -= db_sec_size(db);
        }
        read_sector_info(db, addr, &sec, false);
        db->last_time = sec.end_time;
    }

    /* unlock the TSDB */
    db_unlock(db);

__exit:

    _fdb_init_finish((fdb_db_t)db, result);

    return result;
}

/**
 * The time series database deinitialization.
 *
 * @param db database object
 *
 * @return result
 */
fdb_err_t fdb_tsdb_deinit(fdb_tsdb_t db)
{
    _fdb_deinit((fdb_db_t) db);

    return FDB_NO_ERR;
}

#endif /* defined(FDB_USING_TSDB) */
