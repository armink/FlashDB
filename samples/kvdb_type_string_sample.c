/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief string KV samples.
 *
 * Key-Value Database string type KV feature samples source file.
 */

#include <flashdb.h>
#include <string.h>

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

void kvdb_type_string_sample(void)
{
    fdb_err_t result;

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
        char temp_data[10] = "36";

        /* It will create new KV node when "temp" KV not in database. */
        fdb_kv_set(&kvdb, "temp", temp_data);
    }

    { /* GET the KV value */
        char *return_value, temp_data[10] = { 0 };

        /* Get the "temp" KV value.
         * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
         */
        return_value = fdb_kv_get(&kvdb, "temp");
        /* the return value is NULL when get the value failed */
        if (return_value != NULL) {
            strncpy(temp_data, return_value, sizeof(temp_data));
            FDB_PRINT("temp_data: %s\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        char temp_data[10] = "38";

        /* change the "temp" KV's value to "38.1" */
        fdb_kv_set(&kvdb, "temp", temp_data);
    }

    { /* DELETE the KV by name */
        fdb_kv_del(&kvdb, "temp");
    }
}
