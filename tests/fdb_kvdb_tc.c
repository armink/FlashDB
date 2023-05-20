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
#include <inttypes.h>

#define RT_USING_UTEST

#if RT_VER_NUM <= 0x40003
#include <dfs_posix.h>
#else
#include <dfs_file.h>
#include <unistd.h>
#endif

#define TEST_TS_PART_NAME             "fdb_kvdb1"
#define TEST_KV_BLOB_NAME             "kv_blob_test"
#define TEST_KV_NAME                  "kv_test"
#define TEST_KV_VALUE_LEN              1200 /* only save 3 KVs in a 4096 sector */
#define TEST_KV_MAX_NUM                8
#define TEST_KVDB_SECTOR_SIZE          4096
#define TEST_KVDB_SECTOR_NUM           4

#define FDB_ARRAY_SIZE(array)          (sizeof(array) / sizeof(array[0]))

#if defined(RT_USING_UTEST) && defined(FDB_USING_KVDB)

struct test_kv{
    char name[32];
    uint8_t value[TEST_KV_VALUE_LEN];
    uint32_t addr;
    uint32_t saved_data_size;
    bool is_changed;
};

static struct fdb_kvdb test_kvdb;

static void test_fdb_kvdb_deinit(void);

static rt_err_t dir_delete(const char* path)
{
    DIR* dir = NULL;
    struct dirent* dirent = NULL;
    char* full_path;
    rt_err_t res = RT_EOK;

    if (path == RT_NULL || path[0] == '\0')
        return -ENOENT;

    full_path = (char*)rt_malloc(DFS_PATH_MAX);
    if (full_path == RT_NULL)
        return -ENOMEM;

    dir = opendir(path);
    if (dir == RT_NULL)
    {
        rt_free(full_path);
        return -ENOENT;
    }

    while (1)
    {
        dirent = readdir(dir);
        if (dirent == RT_NULL)
            break;
        if (rt_strcmp(".", dirent->d_name) != 0 &&
            rt_strcmp("..", dirent->d_name) != 0)
        {
            rt_snprintf(full_path, DFS_PATH_MAX, "%s/%s", path, dirent->d_name);
            if (dirent->d_type == DT_REG)
            {
                if (unlink(full_path) != 0)
                {
                    LOG_W("cannot remove '%s'", full_path);
                    res = -RT_ERROR;
                }
            }
            else if (dirent->d_type == DT_DIR)
            {
                if (dir_delete(full_path) != RT_EOK)
                {
                    res = -RT_ERROR;
                }
            }
        }
    }
    closedir(dir);
    rt_free(full_path);

    if (path[rt_strlen(path) - 1] != '/')
    {
        if (unlink(path) != 0)
        {
            LOG_W("cannot remove '%s'", path);
            res = -RT_ERROR;
        }
    }

    return res;
}

static void test_fdb_kvdb_init_by_sector_num(size_t sector_num)
{
    if (access(TEST_TS_PART_NAME, 0) < 0)
    {
        mkdir(TEST_TS_PART_NAME, 0);
    }

    uint32_t sec_size = TEST_KVDB_SECTOR_SIZE, db_size = sec_size * sector_num;
    rt_bool_t file_mode = true;

    fdb_kvdb_control(&(test_kvdb), FDB_KVDB_CTRL_SET_SEC_SIZE, &sec_size);
    fdb_kvdb_control(&(test_kvdb), FDB_KVDB_CTRL_SET_FILE_MODE, &file_mode);
    fdb_kvdb_control(&(test_kvdb), FDB_KVDB_CTRL_SET_MAX_SIZE, &db_size);

    uassert_true(fdb_kvdb_init(&test_kvdb, "test_kv", TEST_TS_PART_NAME, NULL, NULL) == FDB_NO_ERR);
}

static void test_fdb_kvdb_init(void)
{
    test_fdb_kvdb_init_by_sector_num(TEST_KVDB_SECTOR_NUM);
}

static void test_fdb_kvdb_init_by_8_sectors(void)
{
    test_fdb_kvdb_init_by_sector_num(8);
}

static void test_fdb_kvdb_init_check(void)
{
    /* check the oldest address */
    uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
}

static void test_fdb_kvdb_deinit(void)
{
    uassert_true(fdb_kvdb_deinit(&test_kvdb) == FDB_NO_ERR);
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

    snprintf(value_buf, sizeof(value_buf), "%" PRIu32, tick);
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

    snprintf(value_buf, sizeof(value_buf), "%" PRIu32, tick);
    result = fdb_kv_set(&test_kvdb, TEST_KV_NAME, value_buf);
    uassert_true(result == FDB_NO_ERR);

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    uassert_int_equal(tick, read_tick);
}

/* check the oldest address is already right when kvdb reinit */
static void fdb_reboot(void)
{
    test_fdb_kvdb_deinit();
    test_fdb_kvdb_init();
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

    {
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
    }
}

static int iter_all_kv(fdb_kvdb_t db, struct test_kv *kv_tbl, size_t len)
{
    struct fdb_kv_iterator iterator;
    fdb_kv_t cur_kv;
    size_t data_size = 0;
    int index = 0;
    struct fdb_blob fdb_blob;

    fdb_kv_iterator_init(db, &iterator);
    while (fdb_kv_iterate(db, &iterator) == RT_TRUE && index < len)
    {
        /* get data len */
        cur_kv = &(iterator.curr_kv);
        data_size = (size_t)cur_kv->value_len;

        rt_strncpy(kv_tbl[index].name, cur_kv->name, 32);
        kv_tbl[index].saved_data_size = data_size;
        kv_tbl[index].addr = cur_kv->addr.start;
        /* read data */
        fdb_blob_read((fdb_db_t)db, fdb_kv_to_blob(cur_kv, fdb_blob_make(&fdb_blob, &kv_tbl[index].value, data_size)));
        uassert_true(kv_tbl[index].saved_data_size <= TEST_KV_VALUE_LEN);

        index++;
    }

    return index;
}

static void test_save_fdb_by_kvs(const struct test_kv *kv_tbl, size_t len)
{
    struct fdb_blob blob_obj, * blob = &blob_obj;

    for (size_t i = 0; i < len; i++)
    {
        if (kv_tbl[i].is_changed)
        {
            fdb_kv_set_blob(&test_kvdb, kv_tbl[i].name, fdb_blob_make(blob, kv_tbl[i].value, TEST_KV_VALUE_LEN));
        }
    }
}

static void test_check_fdb_by_kvs(const struct test_kv *kv_tbl, size_t len)
{
    static struct test_kv saved_kv_tbl[TEST_KV_MAX_NUM] = { 0 };

    iter_all_kv(&test_kvdb, saved_kv_tbl, FDB_ARRAY_SIZE(saved_kv_tbl));

    for (size_t i = 0, j = 0; i < len; i++)
    {
        for (j = 0; j < FDB_ARRAY_SIZE(saved_kv_tbl); j++)
        {
            if (rt_strcmp(saved_kv_tbl[j].name, kv_tbl[i].name) == 0)
                break;
        }
        if (j < FDB_ARRAY_SIZE(saved_kv_tbl))
        {
            uassert_str_equal(saved_kv_tbl[j].name, kv_tbl[i].name);
            uassert_str_equal(saved_kv_tbl[j].value, kv_tbl[i].value);
            uassert_true(RT_ALIGN_DOWN(saved_kv_tbl[j].addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * kv_tbl[i].addr);
        }
        else
        {
            /* kv not found */
            uassert_true(0);
        }
    }
}

static void test_fdb_by_kvs(const struct test_kv *kv_tbl, size_t len)
{
    test_save_fdb_by_kvs(kv_tbl, len);
    test_check_fdb_by_kvs(kv_tbl, len);
}

static void test_fdb_gc(void)
{
    fdb_kv_set_default(&test_kvdb);

    {
        /*
         * prepare1: add 4 KVs
         *
         * +---------------------------------------------------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    using    |    empty     |    empty    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |    kv0       |    kv3      |              |             |
         * |    new       |    new      |              |             |
         * +----------------------------+              |             |
         * |              |             |              |             |
         * |    kv1       |             |              |             |
         * |    new       |             |              |             |
         * +--------------+             |              |             |
         * |              |             |              |             |
         * |    kv2       |             |              |             |
         * |    new       |             |              |             |
         * +--------------+             |              |             |
         * |              |             |              |             |
         * +---------------------------------------------------------+
         */
        static const struct test_kv kv_tbl[] = {
            {"kv0", "0", 0, 0, 1},
            {"kv1", "1", 0, 0, 1},
            {"kv2", "2", 0, 0, 1},
            {"kv3", "3", 1, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
    }

    {
        /*
         * prepare2: change kv0 and kv3
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    using    |    empty     |    empty    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |    kv0       |     kv3     |              |             |
         * |    delete    |     delete  |              |             |
         * +----------------------------+              |             |
         * |              |             |              |             |
         * |    kv1       |     kv0     |              |             |
         * |    new       |     new     |              |             |
         * +----------------------------+              |             |
         * |              |             |              |             |
         * |    kv2       |     kv3     |              |             |
         * |    new       |     new     |              |             |
         * +----------------------------+              |             |
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         */
        static const struct test_kv kv_tbl[] = {
            {"kv1", "1", 0, 0, 0},
            {"kv2", "2", 0, 0, 0},
            {"kv0", "00", 1, 0, 1},
            {"kv3", "33", 1, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
    }

    {
        /*
         * change kv0, kv1, kv2 and kv3, trigger GC
         *
         * step1: change kv0, kv1 and kv2
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    using    |     using    |    empty    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |    kv0       |     kv3     |     kv0      |             |
         * |    delete    |     delete  |     new      |             |
         * +-------------------------------------------+             |
         * |              |             |              |             |
         * |    kv1       |     kv0     |     kv1      |             |
         * |    delete    |     delete  |     new      |             |
         * +-------------------------------------------+             |
         * |              |             |              |             |
         * |    kv2       |     kv3     |     kv2      |             |
         * |    delete    |     new     |     new      |             |
         * +-------------------------------------------+             |
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         *
         * step2.1: change kv3, start GC, only GC 1 sector on FlashDB V1.2
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    empty     |    using    |     using    |    empty    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |              |     kv3     |     kv0      |             |
         * |              |     delete  |     new      |             |
         * |              +----------------------------+             |
         * |              |             |              |             |
         * |              |     kv0     |     kv1      |             |
         * |              |     delete  |     new      |             |
         * |              +----------------------------+             |
         * |              |             |              |             |
         * |              |     kv3     |     kv2      |             |
         * |              |     new     |     new      |             |
         * |              +----------------------------+             |
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         *
         * step2.2: change kv3
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    empty     |    using    |     using    |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |              |     kv3     |     kv0      |     kv3     |
         * |              |     delete  |     new      |     new     |
         * |              +------------------------------------------+
         * |              |             |              |             |
         * |              |     kv0     |     kv1      |             |
         * |              |     delete  |     new      |             |
         * |              +----------------------------+             |
         * |              |             |              |             |
         * |              |     kv3     |     kv2      |             |
         * |              |     delete  |     new      |             |
         * |              +----------------------------+             |
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         *
         */
        static const struct test_kv kv_tbl[] = {
            {"kv0", "000", 2, 0, 1},
            {"kv1", "111", 2, 0, 1},
            {"kv2", "222", 2, 0, 1},
            {"kv3", "333", 3, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 1);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 1);
    }

    {
        /*
         * change kv0, kv1, kv2 and kv3, trigger GC
         *
         * change kv0 and kv1
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    empty     |    using    |     using    |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |              |     kv3     |     kv0      |     kv3     |
         * |              |     delete  |     delete   |     new     |
         * |              +------------------------------------------+
         * |              |             |              |             |
         * |              |     kv0     |     kv1      |     kv0     |
         * |              |     delete  |     delete   |     new     |
         * |              +------------------------------------------+
         * |              |             |              |             |
         * |              |     kv3     |     kv2      |     kv1     |
         * |              |     delete  |     new      |     new     |
         * |              +------------------------------------------+
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         *
         * step2.1: change kv2, start GC, only GC 1 sector on FlashDB V1.2
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    empty     |    empty    |     using    |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |              |             |     kv0      |     kv3     |
         * |              |             |     delete   |     new     |
         * |              |             +----------------------------+
         * |              |             |              |             |
         * |              |             |     kv1      |     kv0     |
         * |              |             |     delete   |     new     |
         * |              |             +----------------------------+
         * |              |             |              |             |
         * |              |             |     kv2      |     kv1     |
         * |              |             |     new      |     new     |
         * |              |             +----------------------------+
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         *
         * step2.2: change kv2
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    uings     |    empty    |     using    |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |     kv2      |             |     kv0      |     kv3     |
         * |     new      |             |     delete   |     new     |
         * +--------------+             +----------------------------+
         * |              |             |              |             |
         * |              |             |     kv1      |     kv0     |
         * |              |             |     delete   |     new     |
         * |              |             +----------------------------+
         * |              |             |              |             |
         * |              |             |     kv2      |     kv1     |
         * |              |             |     delete   |     new     |
         * |              |             +----------------------------+
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         *
         * step3: change kv3
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    empty    |     using    |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |     kv2      |             |     kv0      |     kv3     |
         * |     new      |             |     delete   |     delete  |
         * +--------------+             +----------------------------+
         * |              |             |              |             |
         * |     kv3      |             |     kv1      |     kv0     |
         * |     new      |             |     delete   |     new     |
         * +--------------+             +----------------------------+
         * |              |             |              |             |
         * |              |             |     kv2      |     kv1     |
         * |              |             |     delete   |     new     |
         * |              |             +----------------------------+
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         */
        static const struct test_kv kv_tbl[] = {
            {"kv0", "0000", 3, 0, 1},
            {"kv1", "1111", 3, 0, 1},
            {"kv2", "2222", 0, 0, 1},
            {"kv3", "3333", 0, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 2);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 2);

    }
}

static void test_fdb_scale_up(void)
{
    fdb_kv_set_default(&test_kvdb);

    static const struct test_kv old_kv_tbl[] = {
        {"kv0", "0", 0, 0, 1},
        {"kv1", "1", 0, 0, 1},
        {"kv2", "2", 0, 0, 1},
        {"kv3", "3", 1, 0, 1},
    };
    /* save some data */
    test_save_fdb_by_kvs(old_kv_tbl, FDB_ARRAY_SIZE(old_kv_tbl));

     /* reboot, scale up from 4 sectors to 8 sectors */
    test_fdb_kvdb_deinit();
    test_fdb_kvdb_init_by_8_sectors();

    /* check old data */
    test_check_fdb_by_kvs(old_kv_tbl, FDB_ARRAY_SIZE(old_kv_tbl));

    /* save some new data */
    static const struct test_kv new_kv_tbl[] = {
        {"kv4", "4", 4, 0, 1},
        {"kv5", "5", 4, 0, 1},
        {"kv6", "6", 4, 0, 1},
        {"kv7", "7", 5, 0, 1},
    };
    /* kv4: sector1, kv5: sector1, kv6: sector2, kv7: sector2 */
    test_save_fdb_by_kvs(new_kv_tbl, FDB_ARRAY_SIZE(new_kv_tbl));
    /* kv4: sector2, kv5: sector3, kv6: sector3, kv7: sector3 */
    test_save_fdb_by_kvs(new_kv_tbl, FDB_ARRAY_SIZE(new_kv_tbl));
    /* kv4: sector4, kv5: sector4, kv6: sector4, kv7: sector5 */
    test_save_fdb_by_kvs(new_kv_tbl, FDB_ARRAY_SIZE(new_kv_tbl));
    /* check new data */
    test_check_fdb_by_kvs(new_kv_tbl, FDB_ARRAY_SIZE(new_kv_tbl));
    /* check old data */
    test_check_fdb_by_kvs(old_kv_tbl, FDB_ARRAY_SIZE(old_kv_tbl));
}

static void test_fdb_kvdb_set_default(void)
{
    uassert_true(fdb_kv_set_default(&test_kvdb) == FDB_NO_ERR);
}


static rt_err_t utest_tc_init(void)
{
    dir_delete(TEST_TS_PART_NAME);
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    if (test_kvdb.parent.init_ok)
    {
        fdb_kv_set_default(&test_kvdb);
        fdb_kvdb_deinit(&test_kvdb);
    }

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_fdb_kvdb_init);
    UTEST_UNIT_RUN(test_fdb_kvdb_init_check);
    UTEST_UNIT_RUN(test_fdb_create_kv_blob);
    UTEST_UNIT_RUN(test_fdb_change_kv_blob);
    UTEST_UNIT_RUN(test_fdb_del_kv_blob);
    UTEST_UNIT_RUN(test_fdb_create_kv);
    UTEST_UNIT_RUN(test_fdb_change_kv);
    UTEST_UNIT_RUN(test_fdb_del_kv);
    UTEST_UNIT_RUN(test_fdb_gc);
    UTEST_UNIT_RUN(test_fdb_scale_up);
    UTEST_UNIT_RUN(test_fdb_kvdb_set_default);
    UTEST_UNIT_RUN(test_fdb_kvdb_deinit);
}
UTEST_TC_EXPORT(testcase, "packages.system.flashdb.kvdb", utest_tc_init, utest_tc_cleanup, 20);

#endif /* defined(RT_USING_UTEST) && defined(FDB_USING_KVDB) */
