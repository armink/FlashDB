/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TSDB testcases.
 *
 * This testcases is be used in RT-Thread Utest framework.
 * If you want run it, please add it to RT-Thread project.
 */

#include "utest.h"
#include <flashdb.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(RT_USING_UTEST) && defined(FDB_USING_TSDB)

#define TEST_TS_PART_NAME             "fdb_tsdb1"
#define TEST_TS_COUNT                 256
#define TEST_TS_USER_STATUS1_COUNT    (TEST_TS_COUNT/2)
#define TEST_TS_DELETED_COUNT         (TEST_TS_COUNT - TEST_TS_USER_STATUS1_COUNT)

static char log[10];

static struct fdb_tsdb test_tsdb;
static int cur_times = 0;

static fdb_time_t get_time(void)
{
    return cur_times ++;
}

static void test_fdb_tsdb_init_ex(void)
{
    uassert_true(fdb_tsdb_init(&test_tsdb, "test_ts", TEST_TS_PART_NAME, get_time, 128, NULL) == FDB_NO_ERR);
}

static void test_fdb_tsl_append(void)
{
    struct fdb_blob blob;
    int i;

    for (i = 0; i < TEST_TS_COUNT; ++i) {
        rt_snprintf(log, sizeof(log), "%d", i);
        uassert_true(fdb_tsl_append(&test_tsdb, fdb_blob_make(&blob, log, rt_strnlen(log, sizeof(log)))) == FDB_NO_ERR);
    }
}

static bool test_fdb_tsl_iter_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    char data[sizeof(log)];
    size_t read_len;

    fdb_blob_make(&blob, data, tsl->log_len);
    read_len = fdb_blob_read((fdb_db_t) &test_tsdb, fdb_tsl_to_blob(tsl, &blob));

    data[read_len] = '\0';

    if (arg == NULL) {
        uassert_true(tsl->time == atoi(data));
    } else {
        uassert_true(tsl->time == *((fdb_time_t *)arg));
    }

    return false;
}

static void test_fdb_tsl_iter(void)
{
    fdb_tsl_iter(&test_tsdb, test_fdb_tsl_iter_cb, NULL);
}

static void test_fdb_tsl_iter_by_time(void)
{
    fdb_time_t from = 0, to = TEST_TS_COUNT -1;

    for (fdb_time_t cur = from; cur <= to; cur ++) {
        fdb_tsl_iter_by_time(&test_tsdb, cur, cur, test_fdb_tsl_iter_cb, &cur);
    }
    fdb_tsl_iter_by_time(&test_tsdb, from, to, test_fdb_tsl_iter_cb, NULL);
}

static void test_fdb_tsl_query_count(void)
{
    fdb_time_t from = 0, to = TEST_TS_COUNT -1;

    uassert_true(fdb_tsl_query_count(&test_tsdb, from, to, FDB_TSL_WRITE) == TEST_TS_COUNT);
}

static bool est_fdb_tsl_set_status_cb(fdb_tsl_t tsl, void *arg)
{
	fdb_tsdb_t db = arg;

    if (tsl->time >= 0 && tsl->time < TEST_TS_USER_STATUS1_COUNT) {
        uassert_true(fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1) == FDB_NO_ERR);
    } else {
        uassert_true(fdb_tsl_set_status(db, tsl, FDB_TSL_DELETED) == FDB_NO_ERR);
    }

    return false;
}

static void test_fdb_tsl_set_status(void)
{
    fdb_time_t from = 0, to = TEST_TS_COUNT -1;

    fdb_tsl_iter_by_time(&test_tsdb, from, to, est_fdb_tsl_set_status_cb, &test_tsdb);

    uassert_true(fdb_tsl_query_count(&test_tsdb, from, to, FDB_TSL_USER_STATUS1) == TEST_TS_USER_STATUS1_COUNT);
    uassert_true(fdb_tsl_query_count(&test_tsdb, from, to, FDB_TSL_DELETED) == TEST_TS_DELETED_COUNT);
}

static bool test_fdb_tsl_clean_cb(fdb_tsl_t tsl, void *arg)
{
    size_t *count = arg;

    (*count) ++;

    return false;
}

static void test_fdb_tsl_clean(void)
{
    size_t count = 0;

    fdb_tsl_clean(&test_tsdb);

    fdb_tsl_iter(&test_tsdb, test_fdb_tsl_clean_cb, &count);

    uassert_true(count == 0);
}

static rt_err_t utest_tc_init(void)
{
    cur_times = 0;
    rt_memset(&test_tsdb, 0, sizeof(struct fdb_tsdb));

    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_fdb_tsdb_init_ex);
    UTEST_UNIT_RUN(test_fdb_tsl_clean);
    UTEST_UNIT_RUN(test_fdb_tsl_append);
    UTEST_UNIT_RUN(test_fdb_tsl_iter);
    UTEST_UNIT_RUN(test_fdb_tsl_iter_by_time);
    UTEST_UNIT_RUN(test_fdb_tsl_query_count);
    UTEST_UNIT_RUN(test_fdb_tsl_set_status);
    UTEST_UNIT_RUN(test_fdb_tsl_clean);
}
UTEST_TC_EXPORT(testcase, "packages.tools.flashdb.tsdb", utest_tc_init, utest_tc_cleanup, 20);

#endif /* defined(RT_USING_UTEST) && defined(FDB_USING_TSDB) */
