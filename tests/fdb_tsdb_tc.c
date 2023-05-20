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
#define TEST_SECTOR_SIZE              4096
#define TEST_TIME_STEP                2

struct test_tls_data {
    int data;
    fdb_time_t time;
    uint32_t addr;
    rt_slist_t list;
};

struct test_tls_sector {
    uint32_t addr;                               /**< sector start address */
    fdb_time_t start_time;                       /**< the first start node's timestamp, 0x7FFFFFFF: unused */
    fdb_time_t end_time;
};

static char logbuf[10];
static struct fdb_tsdb test_tsdb;
static int cur_times = 0;
static struct test_tls_sector test_secs_info[10];
static fdb_time_t test_db_start_time = 0x7FFFFFFF, test_db_end_time = 0;

static fdb_time_t get_time(void)
{
    cur_times += TEST_TIME_STEP;
    return cur_times;
}

static void test_fdb_tsdb_init_ex(void)
{
    if (access(TEST_TS_PART_NAME, 0) < 0)
    {
        mkdir(TEST_TS_PART_NAME, 0);
    }

    uint32_t sec_size = TEST_SECTOR_SIZE, db_size = sec_size * 16;
    rt_bool_t file_mode = true;
    fdb_kvdb_control((fdb_kvdb_t)&(test_tsdb), FDB_TSDB_CTRL_SET_SEC_SIZE, &sec_size);
    fdb_kvdb_control((fdb_kvdb_t)&(test_tsdb), FDB_TSDB_CTRL_SET_FILE_MODE, &file_mode);
    fdb_kvdb_control((fdb_kvdb_t)&(test_tsdb), FDB_TSDB_CTRL_SET_MAX_SIZE, &db_size);

    uassert_true(fdb_tsdb_init(&test_tsdb, "test_ts", TEST_TS_PART_NAME, get_time, 128, NULL) == FDB_NO_ERR);
}

static void test_fdb_tsl_append(void)
{
    struct fdb_blob blob;
    int i;

    for (i = 0; i < TEST_TS_COUNT * TEST_TIME_STEP; ) {
        i += TEST_TIME_STEP;
        rt_snprintf(logbuf, sizeof(logbuf), "%d", i);
        uassert_true(fdb_tsl_append(&test_tsdb, fdb_blob_make(&blob, logbuf, rt_strnlen(logbuf, sizeof(logbuf)))) == FDB_NO_ERR);
    }
}

static bool test_fdb_tsl_iter_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    char data[sizeof(logbuf)];
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
    fdb_time_t from = 0, to = TEST_TS_COUNT * TEST_TIME_STEP - 1;

    for (fdb_time_t cur = from; cur <= to; cur += TEST_TIME_STEP) {
        fdb_tsl_iter_by_time(&test_tsdb, cur, cur, test_fdb_tsl_iter_cb, &cur);
    }
    fdb_tsl_iter_by_time(&test_tsdb, from, to, test_fdb_tsl_iter_cb, NULL);
}

static void test_fdb_tsl_query_count(void)
{
    fdb_time_t from = 0, to = TEST_TS_COUNT * TEST_TIME_STEP;

    uassert_true(fdb_tsl_query_count(&test_tsdb, from, to, FDB_TSL_WRITE) == TEST_TS_COUNT);
}

static bool est_fdb_tsl_set_status_cb(fdb_tsl_t tsl, void *arg)
{
	fdb_tsdb_t db = arg;

    if (tsl->time >= 0 && tsl->time <= TEST_TS_USER_STATUS1_COUNT * TEST_TIME_STEP) {
        uassert_true(fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1) == FDB_NO_ERR);
    } else {
        uassert_true(fdb_tsl_set_status(db, tsl, FDB_TSL_DELETED) == FDB_NO_ERR);
    }

    return false;
}

static void test_fdb_tsl_set_status(void)
{
    fdb_time_t from = 0, to = TEST_TS_COUNT * TEST_TIME_STEP;

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

    cur_times = 0;
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

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    int data;
    struct test_tls_data *node;
    rt_slist_t *tsl_list = (rt_slist_t *)arg;
    fdb_blob_read((fdb_db_t) &test_tsdb, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &data, sizeof(data))));
    node = rt_malloc(sizeof(struct test_tls_data));
    RT_ASSERT(node != RT_NULL);

    node->data = data;
    node->time = tsl->time;
    rt_slist_append(tsl_list,&node->list);
    return false;
}

static bool get_sector_info_cb(fdb_tsl_t tsl, void *arg)
{
    int i = tsl->addr.log / TEST_SECTOR_SIZE;

    if (i < sizeof(test_secs_info) / sizeof(test_secs_info[0])) {
        test_secs_info[i].addr = RT_ALIGN_DOWN(tsl->addr.log, TEST_SECTOR_SIZE);
        if (test_secs_info[i].start_time > tsl->time) {
            test_secs_info[i].start_time = tsl->time;
        }
        if (test_secs_info[i].end_time < tsl->time) {
            test_secs_info[i].end_time = tsl->time;
        }
        if (test_db_start_time > tsl->time) {
            test_db_start_time = tsl->time;
        }
        if (test_db_end_time < tsl->time) {
            test_db_end_time = tsl->time;
        }
        return false;
    } else {
        return true;
    }
}

static void test_tsdb_data_by_time(fdb_time_t from, fdb_time_t to)
{
    rt_slist_t tsl_list;
    rt_slist_init(&tsl_list);
    fdb_time_t i, cur_time = from, valid_to = to, last_tsl_time;
    uint32_t tsl_num, j;

    if (from <= to) {
        if (from < test_db_start_time) {
            cur_time = test_db_start_time;
        }
        if (to > test_db_end_time) {
            valid_to = test_db_end_time;
        }
    } else {
        if (from > test_db_end_time) {
            cur_time = test_db_end_time;
        }
        if (to < test_db_start_time) {
            valid_to = test_db_start_time;
        }
    }

    fdb_tsl_iter_by_time(&test_tsdb, from, to, query_cb, &tsl_list);

    struct test_tls_data *tls;
    rt_slist_t* node = RT_NULL;

    tsl_num = rt_slist_len(&tsl_list);
    /* check the tsl number */
    if (from <= to) {
        for (i = cur_time, j = 0; i <= valid_to; i++) {
            if (i % TEST_TIME_STEP == 0) {
                j++;
            }
        }
    } else {
        for (i = cur_time, j = 0; i >= valid_to; i--) {
            if (i % TEST_TIME_STEP == 0) {
                j++;
            }
        }
    }
    uassert_true(tsl_num == j);
    /* check the tsl time */
    for (node = (&tsl_list)->next; node != RT_NULL; node = node->next, rt_free(tls))
    {
        tls = rt_slist_entry(node, struct test_tls_data, list);
        if (from <= to) {
            uassert_true(tls->time == RT_ALIGN(cur_time, TEST_TIME_STEP));
            cur_time += TEST_TIME_STEP;
        } else {
            uassert_true(tls->time == RT_ALIGN_DOWN(cur_time, TEST_TIME_STEP));
            cur_time -= TEST_TIME_STEP;
        }
        last_tsl_time = tls->time;
    }

    if (tsl_num > 0) {
        /* check the last tsl */
        if (from <= to) {
            uassert_true(last_tsl_time == RT_ALIGN_DOWN(valid_to, TEST_TIME_STEP));
        } else {
            uassert_true(last_tsl_time == RT_ALIGN(valid_to, TEST_TIME_STEP));
        }
    }
}

static void test_fdb_tsl_iter_by_time_1(void)
{
    struct fdb_blob blob;
    int data, i;

    fdb_tsl_clean(&test_tsdb);
    /* make test data for more than 2 sectors */
    for (data = 0; data < 800; data++) {
        fdb_tsl_append(&test_tsdb, fdb_blob_make(&blob, &data, sizeof(data)));
    }
    /* init all test sectors info */
    for (i = 0; i < sizeof(test_secs_info) / sizeof(test_secs_info[0]); i++) {
        test_secs_info[i].addr = TEST_SECTOR_SIZE * i;
        test_secs_info[i].start_time = 0x7FFFFFFF;
        test_secs_info[i].end_time = 0;
    }
    /* get the the sectors info by iterator */
    fdb_tsl_iter_by_time(&test_tsdb, 0, 0x7FFFFFFF, get_sector_info_cb, RT_NULL);
    /* must found more than 2 sectors */
    uassert_true(test_secs_info[2].start_time != 0x7FFFFFFF);
    /* check the database bound */
    test_tsdb_data_by_time(test_db_start_time - 1, test_db_end_time + 1);
    test_tsdb_data_by_time(test_db_start_time - 2, test_db_start_time - 1);
    test_tsdb_data_by_time(test_db_start_time - 1, test_db_start_time - 2);
    test_tsdb_data_by_time(test_db_end_time + 1, test_db_end_time + 2);
    test_tsdb_data_by_time(test_db_end_time + 2, test_db_end_time + 1);

    /* check 1st sector */
    test_tsdb_data_by_time(test_secs_info[0].start_time - 1, test_secs_info[0].end_time);
    test_tsdb_data_by_time(test_secs_info[0].start_time, test_secs_info[0].end_time);
    test_tsdb_data_by_time(test_secs_info[0].start_time, test_secs_info[0].end_time + 1);
    test_tsdb_data_by_time(test_secs_info[0].end_time + 1, test_secs_info[0].start_time);
    test_tsdb_data_by_time(test_secs_info[0].end_time, test_secs_info[0].start_time);
    test_tsdb_data_by_time(test_secs_info[0].end_time, test_secs_info[0].start_time - 1);

    /* check last sector */
    struct test_tls_sector *last_secs = NULL;
    for (i = 0; i < sizeof(test_secs_info) / sizeof(test_secs_info[0]); i++) {
        if (test_secs_info[i].end_time == 0) {
            last_secs = &test_secs_info[i];
            break;
        }
    }
    uassert_true(i >= 3);
    uassert_true(last_secs != NULL);
    test_tsdb_data_by_time(last_secs->start_time - 1, last_secs->end_time);
    test_tsdb_data_by_time(last_secs->start_time, last_secs->end_time);
    test_tsdb_data_by_time(last_secs->start_time, last_secs->end_time + 1);
    test_tsdb_data_by_time(last_secs->end_time + 1, last_secs->start_time);
    test_tsdb_data_by_time(last_secs->end_time, last_secs->start_time);
    test_tsdb_data_by_time(last_secs->end_time, last_secs->start_time - 1);

    /* check less then 1 sector */
    test_tsdb_data_by_time(test_secs_info[0].start_time + 1, test_secs_info[0].end_time - 1);
    test_tsdb_data_by_time(test_secs_info[0].end_time - 1, test_secs_info[0].start_time + 1);

    /* check equal 1 sector */
    test_tsdb_data_by_time(test_secs_info[0].start_time, test_secs_info[0].end_time);
    test_tsdb_data_by_time(test_secs_info[0].end_time, test_secs_info[0].start_time);

    /* check 1~2 sector */
    test_tsdb_data_by_time(test_secs_info[0].start_time - 1, test_secs_info[1].end_time);
    test_tsdb_data_by_time(test_secs_info[0].start_time + 1, test_secs_info[1].end_time);
    test_tsdb_data_by_time(test_secs_info[0].end_time + 1, test_secs_info[1].end_time);
    test_tsdb_data_by_time(test_secs_info[1].end_time + 1, test_secs_info[0].start_time);
    test_tsdb_data_by_time(test_secs_info[1].end_time - 1, test_secs_info[0].start_time);
    test_tsdb_data_by_time(test_secs_info[1].start_time - 1, test_secs_info[0].start_time);

    /* check more than 2 sectors */
    test_tsdb_data_by_time(test_secs_info[0].start_time - 1, test_secs_info[2].end_time);
    test_tsdb_data_by_time(test_secs_info[0].start_time + 1, test_secs_info[2].end_time);
    test_tsdb_data_by_time(test_secs_info[0].end_time + 1, test_secs_info[2].end_time);
    test_tsdb_data_by_time(test_secs_info[2].end_time + 1, test_secs_info[0].start_time);
    test_tsdb_data_by_time(test_secs_info[2].end_time - 1, test_secs_info[0].start_time);
    test_tsdb_data_by_time(test_secs_info[2].start_time - 1, test_secs_info[0].start_time);
}

static void test_fdb_tsdb_deinit(void)
{
    uassert_true(fdb_tsdb_deinit(&test_tsdb) == FDB_NO_ERR);
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
    UTEST_UNIT_RUN(test_fdb_tsl_iter_by_time_1);
    UTEST_UNIT_RUN(test_fdb_tsdb_deinit);
}

UTEST_TC_EXPORT(testcase, "packages.system.flashdb.tsdb", utest_tc_init, utest_tc_cleanup, 20);
#endif /* defined(RT_USING_UTEST) && defined(FDBTC_USING_TSDB) && defined(TC_USING_FDBTC_TSDB) */
