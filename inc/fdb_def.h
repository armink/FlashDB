/*
 * Copyright (c) 2020-2023, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public definition.
 */

#ifndef _FDB_DEF_H_
#define _FDB_DEF_H_

#ifdef FDB_USING_NATIVE_ASSERT
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* software version number */
#define FDB_SW_VERSION                 "2.1.0"
#define FDB_SW_VERSION_NUM             0x20100

/* the KV max name length must less then it */
#ifndef FDB_KV_NAME_MAX
#define FDB_KV_NAME_MAX                64
#endif

/* the KV cache table size, it will improve KV search speed when using cache */
#ifndef FDB_KV_CACHE_TABLE_SIZE
#define FDB_KV_CACHE_TABLE_SIZE        64
#endif

/* the sector cache table size, it will improve KV save speed when using cache */
#ifndef FDB_SECTOR_CACHE_TABLE_SIZE
#define FDB_SECTOR_CACHE_TABLE_SIZE    8
#endif

#if (FDB_KV_CACHE_TABLE_SIZE > 0) && (FDB_SECTOR_CACHE_TABLE_SIZE > 0)
#define FDB_KV_USING_CACHE
#endif

#if defined(FDB_USING_FILE_LIBC_MODE) || defined(FDB_USING_FILE_POSIX_MODE)
#define FDB_USING_FILE_MODE
#endif

/* the file cache table size, it will improve GC speed in file mode when using cache */
#ifndef FDB_FILE_CACHE_TABLE_SIZE
#define FDB_FILE_CACHE_TABLE_SIZE    2
#endif

#ifndef FDB_WRITE_GRAN
#define FDB_WRITE_GRAN 1
#endif

/* log function. default FDB_PRINT macro is printf() */
#ifndef FDB_PRINT
#define FDB_PRINT(...)                 printf(__VA_ARGS__)
#endif
#define FDB_LOG_PREFIX1()              FDB_PRINT("[FlashDB]" FDB_LOG_TAG)
#define FDB_LOG_PREFIX2()              FDB_PRINT(" ")
#define FDB_LOG_PREFIX()               FDB_LOG_PREFIX1();FDB_LOG_PREFIX2()
#ifdef FDB_DEBUG_ENABLE
#define FDB_DEBUG(...)                 FDB_LOG_PREFIX();FDB_PRINT("(%s:%d) ", __FILE__, __LINE__);FDB_PRINT(__VA_ARGS__)
#else
#define FDB_DEBUG(...)
#endif
/* routine print function. Must be implement by user. */
#define FDB_INFO(...)                  FDB_LOG_PREFIX();FDB_PRINT(__VA_ARGS__)
/* assert for developer. */
#ifdef FDB_USING_NATIVE_ASSERT
#define FDB_ASSERT(EXPR)               assert(EXPR);
#else
#ifndef FDB_ASSERT
#define FDB_ASSERT(EXPR)                                                      \
if (!(EXPR))                                                                  \
{                                                                             \
    FDB_INFO("(%s) has assert failed at %s.\n", #EXPR, __func__);             \
    while (1);                                                                \
}
#endif /* FDB_ASSERT */
#endif /* FDB_USING_NATIVE_ASSERT */

#define FDB_KVDB_CTRL_SET_SEC_SIZE     0x00             /**< set sector size control command, this change MUST before database initialization */
#define FDB_KVDB_CTRL_GET_SEC_SIZE     0x01             /**< get sector size control command */
#define FDB_KVDB_CTRL_SET_LOCK         0x02             /**< set lock function control command */
#define FDB_KVDB_CTRL_SET_UNLOCK       0x03             /**< set unlock function control command */
#define FDB_KVDB_CTRL_SET_FILE_MODE    0x09             /**< set file mode control command, this change MUST before database initialization */
#define FDB_KVDB_CTRL_SET_MAX_SIZE     0x0A             /**< set database max size in file mode control command, this change MUST before database initialization */
#define FDB_KVDB_CTRL_SET_NOT_FORMAT   0x0B             /**< set database NOT format mode control command, this change MUST before database initialization */

#define FDB_TSDB_CTRL_SET_SEC_SIZE     0x00             /**< set sector size control command, this change MUST before database initialization */
#define FDB_TSDB_CTRL_GET_SEC_SIZE     0x01             /**< get sector size control command */
#define FDB_TSDB_CTRL_SET_LOCK         0x02             /**< set lock function control command */
#define FDB_TSDB_CTRL_SET_UNLOCK       0x03             /**< set unlock function control command */
#define FDB_TSDB_CTRL_SET_ROLLOVER     0x04             /**< set rollover control command, this change MUST after database initialization */
#define FDB_TSDB_CTRL_GET_ROLLOVER     0x05             /**< get rollover control command */
#define FDB_TSDB_CTRL_GET_LAST_TIME    0x06             /**< get last save time control command */
#define FDB_TSDB_CTRL_SET_FILE_MODE    0x09             /**< set file mode control command, this change MUST before database initialization */
#define FDB_TSDB_CTRL_SET_MAX_SIZE     0x0A             /**< set database max size in file mode control command, this change MUST before database initialization */
#define FDB_TSDB_CTRL_SET_NOT_FORMAT   0x0B             /**< set database NOT formatable mode control command, this change MUST before database initialization */

#ifdef FDB_USING_TIMESTAMP_64BIT
    typedef int64_t fdb_time_t;
#else
    typedef int32_t fdb_time_t;
#endif /* FDB_USING_TIMESTAMP_64BIT */

typedef fdb_time_t (*fdb_get_time)(void);

struct fdb_default_kv_node {
    char *key;
    void *value;
    size_t value_len;
};

struct fdb_default_kv {
    struct fdb_default_kv_node *kvs;
    size_t num;
};

/* error code */
typedef enum {
    FDB_NO_ERR,
    FDB_ERASE_ERR,
    FDB_READ_ERR,
    FDB_WRITE_ERR,
    FDB_PART_NOT_FOUND,
    FDB_KV_NAME_ERR,
    FDB_KV_NAME_EXIST,
    FDB_SAVED_FULL,
    FDB_INIT_FAILED,
} fdb_err_t;

enum fdb_kv_status {
    FDB_KV_UNUSED,
    FDB_KV_PRE_WRITE,
    FDB_KV_WRITE,
    FDB_KV_PRE_DELETE,
    FDB_KV_DELETED,
    FDB_KV_ERR_HDR,
#define FDB_KV_STATUS_NUM                        6
};
typedef enum fdb_kv_status fdb_kv_status_t;

enum fdb_tsl_status {
    FDB_TSL_UNUSED,
    FDB_TSL_PRE_WRITE,
    FDB_TSL_WRITE,
    FDB_TSL_USER_STATUS1,
    FDB_TSL_DELETED,
    FDB_TSL_USER_STATUS2,
#define FDB_TSL_STATUS_NUM                       6
};
typedef enum fdb_tsl_status fdb_tsl_status_t;

/* key-value node object */
struct fdb_kv {
    fdb_kv_status_t status;                      /**< node status, @see fdb_kv_status_t */
    bool crc_is_ok;                              /**< node CRC32 check is OK */
    uint8_t name_len;                            /**< name length */
    uint32_t magic;                              /**< magic word(`K`, `V`, `4`, `0`) */
    uint32_t len;                                /**< node total length (header + name + value), must align by FDB_WRITE_GRAN */
    uint32_t value_len;                          /**< value length */
    char name[FDB_KV_NAME_MAX];                  /**< name */
    struct {
        uint32_t start;                          /**< node start address */
        uint32_t value;                          /**< value start address */
    } addr;
};
typedef struct fdb_kv *fdb_kv_t;

struct fdb_kv_iterator {
    struct fdb_kv curr_kv;                       /**< Current KV we get from the iterator */
    uint32_t iterated_cnt;                       /**< How many KVs have we iterated already */
    size_t iterated_obj_bytes;                   /**< Total storage size of KVs we have iterated. */
    size_t iterated_value_bytes;                 /**< Total value size of KVs we have iterated. */
    uint32_t sector_addr;                        /**< Current sector address we're iterating. DO NOT touch it. */
    uint32_t traversed_len;                      /**< Traversed sector total length. */
};
typedef struct fdb_kv_iterator *fdb_kv_iterator_t;

/* time series log node object */
struct fdb_tsl {
    fdb_tsl_status_t status;                     /**< node status, @see fdb_log_status_t */
    fdb_time_t time;                             /**< node timestamp */
    uint32_t log_len;                            /**< log length, must align by FDB_WRITE_GRAN */
    struct {
        uint32_t index;                          /**< node index address */
        uint32_t log;                            /**< log data address */
    } addr;
};
typedef struct fdb_tsl *fdb_tsl_t;
typedef bool (*fdb_tsl_cb)(fdb_tsl_t tsl, void *arg);

typedef enum {
    FDB_DB_TYPE_KV,
    FDB_DB_TYPE_TS,
} fdb_db_type;

/* the flash sector store status */
enum fdb_sector_store_status {
    FDB_SECTOR_STORE_UNUSED,
    FDB_SECTOR_STORE_EMPTY,
    FDB_SECTOR_STORE_USING,
    FDB_SECTOR_STORE_FULL,
#define FDB_SECTOR_STORE_STATUS_NUM              4
};
typedef enum fdb_sector_store_status fdb_sector_store_status_t;

/* the flash sector dirty status */
enum fdb_sector_dirty_status {
    FDB_SECTOR_DIRTY_UNUSED,
    FDB_SECTOR_DIRTY_FALSE,
    FDB_SECTOR_DIRTY_TRUE,
    FDB_SECTOR_DIRTY_GC,
#define FDB_SECTOR_DIRTY_STATUS_NUM              4
};
typedef enum fdb_sector_dirty_status fdb_sector_dirty_status_t;

/* KVDB section information */
struct kvdb_sec_info {
    bool check_ok;                               /**< sector header check is OK */
    struct {
        fdb_sector_store_status_t store;         /**< sector store status @see fdb_sector_store_status_t */
        fdb_sector_dirty_status_t dirty;         /**< sector dirty status @see sector_dirty_status_t */
    } status;
    uint32_t addr;                               /**< sector start address */
    uint32_t magic;                              /**< magic word(`E`, `F`, `4`, `0`) */
    uint32_t combined;                           /**< the combined next sector number, 0xFFFFFFFF: not combined */
    size_t remain;                               /**< remain size */
    uint32_t empty_kv;                           /**< the next empty KV node start address */
};
typedef struct kvdb_sec_info *kv_sec_info_t;

/* TSDB section information */
struct tsdb_sec_info {
    bool check_ok;                               /**< sector header check is OK */
    fdb_sector_store_status_t status;            /**< sector store status @see fdb_sector_store_status_t */
    uint32_t addr;                               /**< sector start address */
    uint32_t magic;                              /**< magic word(`T`, `S`, `L`, `0`) */
    fdb_time_t start_time;                       /**< the first start node's timestamp, 0xFFFFFFFF: unused */
    fdb_time_t end_time;                         /**< the last end node's timestamp, 0xFFFFFFFF: unused */
    uint32_t end_idx;                            /**< the last end node's index, 0xFFFFFFFF: unused */
    fdb_tsl_status_t end_info_stat[2];           /**< the last end node's info status */
    size_t remain;                               /**< remain size */
    uint32_t empty_idx;                          /**< the next empty node index address */
    uint32_t empty_data;                         /**< the next empty node's data end address */
};
typedef struct tsdb_sec_info *tsdb_sec_info_t;

struct kv_cache_node {
    uint16_t name_crc;                           /**< KV name's CRC32 low 16bit value */
    uint16_t active;                             /**< KV node access active degree */
    uint32_t addr;                               /**< KV node address */
};
typedef struct kv_cache_node *kv_cache_node_t;

/* database structure */
typedef struct fdb_db *fdb_db_t;
struct fdb_db {
    const char *name;                            /**< database name */
    fdb_db_type type;                            /**< database type */
    union {
#ifdef FDB_USING_FAL_MODE
        const struct fal_partition *part;        /**< flash partition for saving database */
#endif
#ifdef FDB_USING_FILE_MODE
        const char *dir;                         /**< directory path for saving database */
#endif
    } storage;
    uint32_t sec_size;                           /**< flash section size. It's a multiple of block size */
    uint32_t max_size;                           /**< database max size. It's a multiple of section size */
    uint32_t oldest_addr;                        /**< the oldest sector start address */
    bool init_ok;                                /**< initialized successfully */
    bool file_mode;                              /**< is file mode, default is false */
    bool not_formatable;                         /**< is can NOT be formated mode, default is false */
#ifdef FDB_USING_FILE_MODE
    uint32_t cur_file_sec[FDB_FILE_CACHE_TABLE_SIZE];/**< last operate sector address  */
#if defined(FDB_USING_FILE_POSIX_MODE)
    int cur_file[FDB_FILE_CACHE_TABLE_SIZE];     /**< current file object */
#elif defined(FDB_USING_FILE_LIBC_MODE)
    FILE *cur_file[FDB_FILE_CACHE_TABLE_SIZE];   /**< current file object */
#endif /* FDB_USING_FILE_MODE */
    uint32_t cur_sec;                            /**< current operate sector address  */
#endif
    void (*lock)(fdb_db_t db);                   /**< lock the database operate */
    void (*unlock)(fdb_db_t db);                 /**< unlock the database operate */

    void *user_data;
};

/* KVDB structure */
struct fdb_kvdb {
    struct fdb_db parent;                        /**< inherit from fdb_db */
    struct fdb_default_kv default_kvs;           /**< default KV */
    bool gc_request;                             /**< request a GC check */
    bool in_recovery_check;                      /**< is in recovery check status when first reboot */
    struct fdb_kv cur_kv;
    struct kvdb_sec_info cur_sector;
    bool last_is_complete_del;

#ifdef FDB_KV_USING_CACHE
    /* KV cache table */
    struct kv_cache_node kv_cache_table[FDB_KV_CACHE_TABLE_SIZE];
    /* sector cache table, it caching the sector info which status is current using */
    struct kvdb_sec_info sector_cache_table[FDB_SECTOR_CACHE_TABLE_SIZE];
#endif /* FDB_KV_USING_CACHE */

#ifdef FDB_KV_AUTO_UPDATE
    uint32_t ver_num;                            /**< setting version number for update */
#endif

    void *user_data;
};
typedef struct fdb_kvdb *fdb_kvdb_t;

/* TSDB structure */
struct fdb_tsdb {
    struct fdb_db parent;                        /**< inherit from fdb_db */
    struct tsdb_sec_info cur_sec;                /**< current using sector */
    fdb_time_t last_time;                        /**< last TSL timestamp */
    fdb_get_time get_time;                       /**< the current timestamp get function */
    size_t max_len;                              /**< the maximum length of each log */
    bool rollover;                               /**< the oldest data will rollover by newest data, default is true */

    void *user_data;
};
typedef struct fdb_tsdb *fdb_tsdb_t;

/* blob structure */
struct fdb_blob {
    void *buf;                                   /**< blob data buffer */
    size_t size;                                 /**< blob data buffer size */
    struct {
        uint32_t meta_addr;                      /**< saved KV or TSL index address */
        uint32_t addr;                           /**< blob data saved address */
        size_t len;                              /**< blob data saved length */
    } saved;
};
typedef struct fdb_blob *fdb_blob_t;

#ifdef __cplusplus
}
#endif

#endif /* _FDB_DEF_H_ */

