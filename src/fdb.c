/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Initialize interface.
 *
 * Some initialize interface for this library.
 */

#include <flashdb.h>
#include <fdb_low_lvl.h>

#define FDB_LOG_TAG ""

/**
 * Set database lock and unlock funtion.
 *
 * @param db database object
 * @param lock lock function
 * @param unlock lock function
 */
void fdb_lock_set(fdb_db_t db, void (*lock)(fdb_db_t db), void (*unlock)(fdb_db_t db))
{
    FDB_ASSERT(db);

    db->lock = lock;
    db->unlock = unlock;
}

/**
 * Set the sector size for database.
 *
 * @note The sector size MUST align by partition block size.
 * @note The sector size change MUST before database initialization.
 *
 * @param db database object
 * @param sec_size
 */
void fdb_sec_size_set(fdb_db_t db, uint32_t sec_size)
{
    FDB_ASSERT(db);
    /* the sector size change MUST before database initialization */
    FDB_ASSERT(db->init_ok == false);

    db->sec_size = sec_size;
}

fdb_err_t _fdb_init_ex(fdb_db_t db, const char *name, const char *part_name, fdb_db_type type, void *user_data)
{
    size_t block_size;

    FDB_ASSERT(db);
    FDB_ASSERT(name);
    FDB_ASSERT(part_name);

    if (db->init_ok) {
        return FDB_NO_ERR;
    }

    db->name = name;
    db->type = type;
    db->user_data = user_data;
    /* FAL (Flash Abstraction Layer) initialization */
    fal_init();
    /* check the flash partition */
    if ((db->part = fal_partition_find(part_name)) == NULL) {
        FDB_INFO("Error: Partition (%s) not found.\n", part_name);
        return FDB_PART_NOT_FOUND;
    }

    block_size = fal_flash_device_find(db->part->flash_name)->blk_size;
    if (db->sec_size == 0) {
        db->sec_size = block_size;
    } else {
        /* must be aligned with block size */
        FDB_ASSERT(db->sec_size % block_size == 0);
    }

    return FDB_NO_ERR;
}

void _fdb_init_finish(fdb_db_t db, fdb_err_t result)
{
    static bool log_is_show = false;
    if (result == FDB_NO_ERR) {
        db->init_ok = true;
        if (!log_is_show) {
            FDB_INFO("FlashDB V%s is initialize success.\n", FDB_SW_VERSION);
            FDB_INFO("You can get the latest version on https://github.com/armink/FlashDB .\n");
            log_is_show = true;
        }
    } else {
        FDB_INFO("Error: %s(%s) at partition %s is initialize fail(%d).\n", db->type == FDB_DB_TYPE_KV ? "KV" : "TS",
                db->name, db->part->name, result);
    }
}
