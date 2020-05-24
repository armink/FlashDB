/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief blob KV samples.
 *
 * Key-Value Database blob type KV feature samples
 */

#include <flashdb.h>

static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
        {"username", "armink", 0}, /* string KV */
        {"password", "123456", 0}, /* string KV */
        {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
        {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};
/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

static void lock(fdb_db_t db)
{
    /* YOUR CODE HERE */
}

static void unlock(fdb_db_t db)
{
    /* YOUR CODE HERE */
}

void kvdb_type_blob_sample(void)
{
    fdb_err_t result;
    struct fdb_blob blob;

    { /* database initialization */
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        /* set the lock and unlock function if you want */
        fdb_lock_set((fdb_db_t)&kvdb, lock, unlock);
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         *  "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

        if (result != FDB_NO_ERR) {
            return;
        }
    }

    { /* CREATE new Key-Value */
        int temp_data = 36;

        /* It will create new KV node when "temp" KV not in database.
         * fdb_blob_make: It's a blob make function, and it will return the blob when make finish.
         */
        fdb_kv_set_blob(&kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
    }

    { /* GET the KV value */
        int temp_data = 0;

        /* get the "temp" KV value */
        fdb_kv_get_blob(&kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_PRINT("temp_data: %d\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        int temp_data = 38;

        /* change the "temp" KV's value to 38.1 */
        fdb_kv_set_blob(&kvdb, "temp", fdb_blob_make(&blob, &temp_data, sizeof(temp_data)));
    }

    { /* DELETE the KV by name */
        fdb_kv_del(&kvdb, "temp");
    }
}
