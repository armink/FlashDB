/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KVDB testcases.
 *
 * This testcases is be used in RT-Thread Utest framework.
 * If you want run it, please add it to RT-Thread project.
 */

#include "utest.h"
#include <flashdb.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_TS_PART_NAME             "fdb_kvdb1"
#define TEST_KV_BLOB_NAME             "kv_blob_test"
#define TEST_KV_NAME                  "kv_test"

#if defined(RT_USING_UTEST) && defined(FDB_USING_KVDB)

static struct fdb_default_kv_node default_kv_set[] = {
        {"iap_need_copy_app", "0"},
        {"iap_need_crc32_check", "0"},
        {"iap_copy_app_size", "0"},
        {"stop_in_bootloader", "0"},
};

static struct fdb_kvdb test_kvdb;

static void test_fdb_kvdb_init(void)
{
    struct fdb_default_kv default_kv;

    default_kv.kvs = default_kv_set;
    default_kv.num = sizeof(default_kv_set) / sizeof(default_kv_set[0]);
    uassert_true(fdb_kvdb_init(&test_kvdb, "test_kv", "fdb_kvdb1", &default_kv, NULL) == FDB_NO_ERR);
}

static void test_fdb_create_kv_blob(void)
{
    fdb_err_t result = FDB_NO_ERR;
    rt_tick_t tick = rt_tick_get(), read_tick;
    size_t read_len;
    struct fdb_kv kv_obj;
    struct fdb_blob blob;
    uint8_t value_buf[sizeof(tick)];

    result = fdb_kv_set_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob, &tick, sizeof(tick)));
    uassert_true(result == FDB_NO_ERR);

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob.saved.len, sizeof(read_tick));
    uassert_int_equal(blob.saved.len, read_len);
    uassert_int_equal(tick, read_tick);

    uassert_true(fdb_kv_get_obj(&test_kvdb, TEST_KV_BLOB_NAME, &kv_obj) != NULL);

    fdb_blob_make(&blob, value_buf, sizeof(value_buf));
    read_len = fdb_blob_read((fdb_db_t)&test_kvdb, fdb_kv_to_blob(&kv_obj, &blob));
    uassert_int_equal(read_len, sizeof(value_buf));
    uassert_buf_equal(&tick, value_buf, sizeof(value_buf));
}

static void test_fdb_change_kv_blob(void)
{
    fdb_err_t result = FDB_NO_ERR;
    rt_tick_t tick = rt_tick_get(), read_tick;
    size_t read_len;
    struct fdb_blob blob_obj, *blob = &blob_obj;

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob_obj, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob->saved.len, sizeof(read_tick));
    uassert_int_equal(blob->saved.len, read_len);
    uassert_int_not_equal(tick, read_tick);

    result = fdb_kv_set_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob_obj, &tick, sizeof(tick)));
    uassert_true(result == FDB_NO_ERR);

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob_obj, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob->saved.len, sizeof(read_tick));
    uassert_int_equal(blob->saved.len, read_len);
    uassert_int_equal(tick, read_tick);
}

static void test_fdb_del_kv_blob(void)
{
    fdb_err_t result = FDB_NO_ERR;
    rt_tick_t tick = rt_tick_get(), read_tick;
    size_t read_len;
    struct fdb_blob blob;

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob.saved.len, sizeof(read_tick));
    uassert_int_equal(blob.saved.len, read_len);
    uassert_int_not_equal(tick, read_tick);

    result = fdb_kv_set_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob, NULL, 0));
    uassert_true(result == FDB_NO_ERR);

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob.saved.len, 0);
    uassert_int_equal(read_len, 0);
}

static void test_fdb_create_kv(void)
{
    fdb_err_t result = FDB_NO_ERR;
    rt_tick_t tick = rt_tick_get(), read_tick;
    char value_buf[14], *read_value;

    snprintf(value_buf, sizeof(value_buf), "%d", tick);
    result = fdb_kv_set(&test_kvdb, TEST_KV_NAME, value_buf);
    uassert_true(result == FDB_NO_ERR);

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    uassert_int_equal(tick, read_tick);
}

static void test_fdb_change_kv(void)
{
    fdb_err_t result = FDB_NO_ERR;
    rt_tick_t tick = rt_tick_get(), read_tick;
    char value_buf[14], *read_value;

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    uassert_int_not_equal(tick, read_tick);

    snprintf(value_buf, sizeof(value_buf), "%d", tick);
    result = fdb_kv_set(&test_kvdb, TEST_KV_NAME, value_buf);
    uassert_true(result == FDB_NO_ERR);

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    uassert_int_equal(tick, read_tick);
}

static void test_fdb_del_kv(void)
{
    fdb_err_t result = FDB_NO_ERR;
    rt_tick_t tick = rt_tick_get(), read_tick;
    char *read_value;

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    uassert_int_not_equal(tick, read_tick);

    result = fdb_kv_del(&test_kvdb, TEST_KV_NAME);
    uassert_true(result == FDB_NO_ERR);

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_null(read_value);
}

static rt_err_t utest_tc_init(void)
{
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_fdb_kvdb_init);
    UTEST_UNIT_RUN(test_fdb_create_kv_blob);
    UTEST_UNIT_RUN(test_fdb_change_kv_blob);
    UTEST_UNIT_RUN(test_fdb_del_kv_blob);
    UTEST_UNIT_RUN(test_fdb_create_kv);
    UTEST_UNIT_RUN(test_fdb_change_kv);
    UTEST_UNIT_RUN(test_fdb_del_kv);
}
UTEST_TC_EXPORT(testcase, "packages.tools.flashdb.kvdb", utest_tc_init, utest_tc_cleanup, 20);

#endif /* defined(RT_USING_UTEST) && defined(FDB_USING_KVDB) */
