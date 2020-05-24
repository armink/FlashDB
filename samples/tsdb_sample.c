/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TSDB samples.
 *
 * Time series log (like TSDB) feature samples source file.
 *
 * TSL is time series log, the TSDB saved many TSLs.
 */

#include <flashdb.h>
#include <string.h>

struct env_status {
    int temp;
    int humi;
};

/* TSDB object */
static struct fdb_tsdb tsdb = { 0 };

static bool query_cb(fdb_tsl_t tsl, void *arg);
static bool set_status_cb(fdb_tsl_t tsl, void *arg);

static void lock(fdb_db_t db)
{
    /* YOUR CODE HERE */
}

static void unlock(fdb_db_t db)
{
    /* YOUR CODE HERE */
}
static fdb_time_t get_time(void)
{
    return time(NULL);
}

void tsdb_sample(void)
{
    fdb_err_t result;
    struct fdb_blob blob;

    { /* database initialization */
        /* set the lock and unlock function if you want */
        fdb_lock_set((fdb_db_t)&tsdb, lock, unlock);
        /* Time series database initialization
         *
         *       &tsdb: database object
         *       "log": database name
         *  "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         *    get_time: The get current timestamp function.
         *         128: maximum length of each log
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);

        if (result != FDB_NO_ERR) {
            return;
        }
    }

    { /* APPEND new TSL (time series log) */
        struct env_status status;

        /* append new log to TSDB */
        status.temp = 36;
        status.humi = 85;
        fdb_tsl_append(&tsdb, fdb_blob_make(&blob, &status, sizeof(status)));

        status.temp = 38;
        status.humi = 90;
        fdb_tsl_append(&tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
    }

    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(&tsdb, query_cb, &tsdb);
    }

    { /* QUERY the TSDB by time */
        /* prepare query time (from 1970-01-01 00:00:00 to 2020-05-05 00:00:00) */
        struct tm tm_from = { .tm_year = 1970 - 1900, .tm_mon = 0, .tm_mday = 1, .tm_hour = 0, .tm_min = 0, .tm_sec = 0 };
        struct tm tm_to = { .tm_year = 2020 - 1900, .tm_mon = 4, .tm_mday = 5, .tm_hour = 0, .tm_min = 0, .tm_sec = 0 };
        time_t from_time = mktime(&tm_from), to_time = mktime(&tm_to);
        size_t count;
        /* query all TSL in TSDB by time */
        fdb_tsl_iter_by_time(&tsdb, from_time, to_time, query_cb, &tsdb);
        /* query all FDB_TSL_WRITE status TSL's count in TSDB by time */
        count = fdb_tsl_query_count(&tsdb, from_time, to_time, FDB_TSL_WRITE);
        FDB_PRINT("query count: %lu\n", count);
    }

    { /* SET the TSL status */
        /* Change the TSL status by iterator or time iterator
         * set_status_cb: the change operation will in this callback
         *
         * NOTE: The actions to modify the state must be in order.
         *       FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2
         */
        fdb_tsl_iter(&tsdb, set_status_cb, &tsdb);
    }
}

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
    FDB_PRINT("time: %d, temp: %d, humi: %d\n", tsl->time, status.temp, status.humi);

    return false;
}

static bool set_status_cb(fdb_tsl_t tsl, void *arg)
{
    fdb_tsdb_t db = arg;

    fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1);

    return false;
}
