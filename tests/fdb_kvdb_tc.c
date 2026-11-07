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
#include <fdb_low_lvl.h>
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

/* ------------------------------------------------------------------
 * Dynamically compute TEST_KV_VALUE_LEN so that exactly 3 KVs fit
 * per TEST_KVDB_SECTOR_SIZE-byte sector for ANY FDB_WRITE_GRAN value,
 * AND so that the test_fdb_gc2 GC path works correctly.
 *
 * Let:
 *   B       = _TKV_BASE  = KV_HDR_SZ + aligned_name
 *   V       = TEST_KV_VALUE_LEN  (aligned to W)
 *   kv_size = B + V
 *   kv5_size= B + 2V  (kv5 value = 2×V)
 *   kv4_size= B + 3V  (kv4 value = 3×V)
 *   th      = _TKV_THRESHOLD = KV_HDR_SZ + FDB_KV_NAME_MAX
 *   usable  = TEST_KVDB_SECTOR_SIZE - _TKV_SEC_HDR_SZ
 *
 * Three constraints, from LEAST to MOST strict:
 *
 *  [A] 4th KV does NOT fit (exactly 3 per sector):
 *      V >= ceil((usable - 5B - 64) / 4)
 *
 *  [B] kv4 (3×V) occupies a full sector (no normal KV fits after it):
 *      usable - (B+3V) <= (B+V) + th
 *      V >= ceil((usable - 3B - 64) / 4)
 *
 *  [C] GC does NOT stop early when kv1+kv2 are moved into the initially
 *      empty sector (sector3).  do_gc() stops when remain > free_size,
 *      where free_size = kv5_size = B+2V.  We need:
 *        usable - 2*(B+V)  <=  B + 2V
 *        3984 - 3B  <=  4V
 *      V >= ceil((usable - 3B) / 4)   ← STRICTEST BOUND (no -64 term)
 *
 * All three constraints reduce to the same form; [C] dominates.
 * The minimum V satisfying [C] (ceiling integer division: (N+3)/4):
 *
 *   V_min = FDB_WG_ALIGN( (usable - 3B + 3) / 4 )
 * ------------------------------------------------------------------*/

/* write-gran alignment unit in bytes */
#define _TKV_W                    ((FDB_WRITE_GRAN + 7) / 8)

/* KV status table size */
#define _TKV_KV_STATUS_SZ         FDB_STATUS_TABLE_SIZE(FDB_KV_STATUS_NUM)

/* Sector header raw size (store_status + dirty_status + magic + combined + reserved) */
#define _TKV_SEC_HDR_RAW_SZ       (FDB_STORE_STATUS_TABLE_SIZE + FDB_DIRTY_STATUS_TABLE_SIZE \
                                   + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))
#if (FDB_WRITE_GRAN == 64) || (FDB_WRITE_GRAN == 128)
#define _TKV_SEC_HDR_STRUCT_SZ     (_TKV_SEC_HDR_RAW_SZ + 4)
#elif (FDB_WRITE_GRAN == 256)
#define _TKV_SEC_HDR_STRUCT_SZ     (_TKV_SEC_HDR_RAW_SZ + 20)
#else
#define _TKV_SEC_HDR_STRUCT_SZ     FDB_ALIGN(_TKV_SEC_HDR_RAW_SZ, sizeof(uint32_t))
#endif
#define _TKV_SEC_HDR_SZ           FDB_WG_ALIGN(_TKV_SEC_HDR_STRUCT_SZ)

/* KV header raw size (status + magic + len + crc32 + name_len + value_len) */
#define _TKV_KV_HDR_RAW_SZ        (_TKV_KV_STATUS_SZ + sizeof(uint32_t) + sizeof(uint32_t) \
                                   + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t))
#define _TKV_KV_HDR_SZ            FDB_WG_ALIGN(_TKV_KV_HDR_RAW_SZ)

/* FDB_SEC_REMAIN_THRESHOLD equivalent: KV_HDR_DATA_SIZE + FDB_KV_NAME_MAX */
#define _TKV_THRESHOLD            (_TKV_KV_HDR_SZ + FDB_KV_NAME_MAX)

/* name length of "kv0".."kv5", aligned */
#define _TKV_NAME_ALIGNED         FDB_WG_ALIGN(3)

/* usable data space per sector */
#define _TKV_USABLE               (TEST_KVDB_SECTOR_SIZE - _TKV_SEC_HDR_SZ)

/* per-KV base overhead: header + aligned name */
#define _TKV_BASE                 (_TKV_KV_HDR_SZ + _TKV_NAME_ALIGNED)

/* Minimum V satisfying constraint [C] (the strictest).
 * Uses ceiling integer division: (N + 3) / 4. */
#define _TKV_MAX_VAL_ALIGNED      FDB_WG_ALIGN((_TKV_USABLE - 3 * _TKV_BASE + 3) / 4)

/* TEST_KV_VALUE_LEN: use aligned size directly (already a multiple of W) */
#define TEST_KV_VALUE_LEN         _TKV_MAX_VAL_ALIGNED

#define TEST_KV_MAX_NUM                8
#define TEST_KVDB_SECTOR_SIZE          4096
#define TEST_KVDB_SECTOR_NUM           4

#define FDB_ARRAY_SIZE(array)          (sizeof(array) / sizeof(array[0]))

#if defined(RT_USING_UTEST) && defined(FDB_USING_KVDB)

struct test_kv{
    char name[32];
    char *value;
    size_t value_len;
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
    rt_tick_t tick = 0, read_tick;
    size_t read_len;
    struct fdb_blob blob_obj, *blob = &blob_obj;

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob_obj, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob->saved.len, sizeof(read_tick));
    uassert_int_equal(blob->saved.len, read_len);
    rt_thread_mdelay(1);
    tick = rt_tick_get();
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
    rt_tick_t tick = 0, read_tick;
    size_t read_len;
    struct fdb_blob blob;

    read_len = fdb_kv_get_blob(&test_kvdb, TEST_KV_BLOB_NAME, fdb_blob_make(&blob, &read_tick, sizeof(read_tick)));
    uassert_int_equal(blob.saved.len, sizeof(read_tick));
    uassert_int_equal(blob.saved.len, read_len);
    rt_thread_mdelay(1);
    tick = rt_tick_get();
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
    rt_tick_t tick = 0, read_tick;
    char value_buf[14], *read_value;

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    rt_thread_mdelay(1);
    tick = rt_tick_get();
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
    rt_tick_t tick = 0, read_tick;
    char *read_value;

    read_value = fdb_kv_get(&test_kvdb, TEST_KV_NAME);
    uassert_not_null(read_value);
    read_tick = atoi(read_value);
    rt_thread_mdelay(1);
    tick = rt_tick_get();
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
        kv_tbl[index].value = (char *)rt_malloc(data_size);
        if (kv_tbl[index].value == NULL)
            RT_ASSERT(0 && "no memory for value");
        /* read data */
        fdb_blob_read((fdb_db_t)db, fdb_kv_to_blob(cur_kv, fdb_blob_make(&fdb_blob, kv_tbl[index].value, data_size)));
        index++;
    }

    return index;
}

static void test_save_fdb_by_kvs(const struct test_kv *kv_tbl, size_t len)
{
    struct fdb_blob blob_obj, *blob = &blob_obj;

    for (size_t i = 0; i < len; i++)
    {
        if (kv_tbl[i].is_changed)
        {
            int result = fdb_kv_set_blob(&test_kvdb, kv_tbl[i].name, fdb_blob_make(blob, kv_tbl[i].value, kv_tbl[i].value_len));
            uassert_true(result == FDB_NO_ERR);
        }
    }
}

static void test_check_fdb_by_kvs(const struct test_kv *kv_tbl, size_t len)
{
    static struct test_kv saved_kv_tbl[TEST_KV_MAX_NUM] = { 0 };

    RT_ASSERT(len <= FDB_ARRAY_SIZE(saved_kv_tbl));

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
            {"kv0", "0", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv1", "1", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv2", "2", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv3", "3", TEST_KV_VALUE_LEN, 1, 0, 1},
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
            {"kv1", "1", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv2", "2", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv0", "00", TEST_KV_VALUE_LEN, 1, 0, 1},
            {"kv3", "33", TEST_KV_VALUE_LEN, 1, 0, 1},
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
            {"kv0", "000", TEST_KV_VALUE_LEN, 2, 0, 1},
            {"kv1", "111", TEST_KV_VALUE_LEN, 2, 0, 1},
            {"kv2", "222", TEST_KV_VALUE_LEN, 2, 0, 1},
            {"kv3", "333", TEST_KV_VALUE_LEN, 3, 0, 1},
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
            {"kv0", "0000", TEST_KV_VALUE_LEN, 3, 0, 1},
            {"kv1", "1111", TEST_KV_VALUE_LEN, 3, 0, 1},
            {"kv2", "2222", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv3", "3333", TEST_KV_VALUE_LEN, 0, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 2);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 2);

    }
}

static void test_fdb_gc2(void)
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
            {"kv0", "0", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv1", "1", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv2", "2", TEST_KV_VALUE_LEN, 0, 0, 1},
            {"kv3", "3", TEST_KV_VALUE_LEN, 1, 0, 1},
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
            {"kv1", "1", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv2", "2", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv0", "00", TEST_KV_VALUE_LEN, 1, 0, 1},
            {"kv3", "33", TEST_KV_VALUE_LEN, 1, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
    }

    {
        /*
         * prepare3: add big kv4
         *
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    using    |    using     |    empty    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |    kv0       |     kv3     |              |             |
         * |    delete    |     delete  |              |             |
         * +----------------------------+     kv4      |             |
         * |              |             |     new      |             |
         * |    kv1       |     kv0     |              |             |
         * |    new       |     new     |              |             |
         * +----------------------------+              |             |
         * |              |             |              |             |
         * |    kv2       |     kv3     |--------------|             |
         * |    new       |     new     |              |             |
         * +----------------------------+              |             |
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         */
        static const struct test_kv kv_tbl[] = {
            {"kv1", "1", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv2", "2", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv0", "00", TEST_KV_VALUE_LEN, 1, 0, 0},
            {"kv3", "33", TEST_KV_VALUE_LEN, 1, 0, 0},
            {"kv4", "4", TEST_KV_VALUE_LEN * 3, 2, 0, 1},
        };

        test_fdb_by_kvs(kv_tbl, FDB_ARRAY_SIZE(kv_tbl));
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
        fdb_reboot();
        uassert_true(RT_ALIGN_DOWN(test_kvdb.parent.oldest_addr, TEST_KVDB_SECTOR_SIZE) == TEST_KVDB_SECTOR_SIZE * 0);
    }

    {
        /*
         * add kv5, trigger GC

         * step1: move kv1 and kv2
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    empty     |    using    |    using     |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |              |     kv3     |              |    kv1      |
         * |              |     delete  |              |    new      |
         * |              +-------------+     kv4      |-------------|
         * |              |             |     new      |             |
         * |              |     kv0     |              |    kv2      |
         * |              |     new     |              |    new      |
         * |              +-------------+              |-------------|
         * |              |             |              |             |
         * |              |     kv3     |--------------|             |
         * |              |     new     |              |             |
         * |              +-------------+              |             |
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         * 
         * step2: move kv0 and kv3
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    empty    |    using     |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |     kv3      |             |              |    kv1      |
         * |     new      |             |              |    new      |
         * +--------------+             |     kv4      |-------------|
         * |              |             |     new      |             |
         * |              |             |              |    kv2      |
         * |              |             |              |    new      |
         * |              |             |              |-------------|
         * |              |             |              |             |
         * |              |             |--------------|    kv0      |
         * |              |             |              |    new      |
         * |              |             |              |-------------|
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         * 
         * step3: add kv5
         * +--------------+-------------+--------------+-------------+
         * |   sector0    |   sector1   |   sector2    |   sector3   |
         * |    using     |    empty    |    using     |    using    |
         * +---------------------------------------------------------+
         * |              |             |              |             |
         * |     kv3      |             |              |    kv1      |
         * |     new      |             |              |    new      |
         * +--------------+             |              |-------------|
         * |              |             |      kv4     |             |
         * |              |             |      new     |    kv2      |
         * |     kv5      |             |              |    new      |
         * |     new      |             |              |-------------|
         * |              |             |              |             |
         * |              |             |--------------|    kv0      |
         * |--------------|             |              |    new      |
         * |              |             |              |-------------|
         * |              |             |              |             |
         * +--------------+-------------+--------------+-------------+
         * 
         */
        static const struct test_kv kv_tbl[] = {
            {"kv3", "33", TEST_KV_VALUE_LEN, 0, 0, 0},
            {"kv5", "5", TEST_KV_VALUE_LEN * 2, 0, 0, 1},
            {"kv4", "4", TEST_KV_VALUE_LEN * 3, 2, 0, 0},
            {"kv1", "1", TEST_KV_VALUE_LEN, 3, 0, 0},
            {"kv2", "2", TEST_KV_VALUE_LEN, 3, 0, 0},
            {"kv0", "00", TEST_KV_VALUE_LEN, 3, 0, 0},
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
        {"kv0", "0", TEST_KV_VALUE_LEN, 0, 0, 1},
        {"kv1", "1", TEST_KV_VALUE_LEN, 0, 0, 1},
        {"kv2", "2", TEST_KV_VALUE_LEN, 0, 0, 1},
        {"kv3", "3", TEST_KV_VALUE_LEN, 1, 0, 1},
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
        {"kv4", "4", TEST_KV_VALUE_LEN, 4, 0, 1},
        {"kv5", "5", TEST_KV_VALUE_LEN, 4, 0, 1},
        {"kv6", "6", TEST_KV_VALUE_LEN, 4, 0, 1},
        {"kv7", "7", TEST_KV_VALUE_LEN, 5, 0, 1},
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

#if FDB_USING_READ_CACHE
/*
 * The read cache is borrowed, not owned: it is allocated while the database is
 * loaded and given back when that loading phase ends, so a running database never
 * keeps a sector sized buffer in RAM. This case pins down that contract, and that
 * a run which goes through the cache reads back what was written.
 */
static void test_fdb_read_cache(void)
{
    struct fdb_kv_iterator itr;
    char key[16], value[16], *read_value;
    fdb_err_t result = FDB_NO_ERR;
    uint32_t i, loaded = 0;

    /* no phase is open here, so no window may be held */
    uassert_null(test_kvdb.parent.read_cache);

    for (i = 0; i < TEST_KV_MAX_NUM; i++) {
        snprintf(key, sizeof(key), "rc_%u", i);
        snprintf(value, sizeof(value), "%u", i);
        result = fdb_kv_set(&test_kvdb, key, value);
        uassert_true(result == FDB_NO_ERR);
        /* a single key write at run time must not allocate the window */
        uassert_null(test_kvdb.parent.read_cache);
    }

    /* a full iteration loads the database, so it may borrow the window ... */
    fdb_kv_iterator_init(&test_kvdb, &itr);
    while (fdb_kv_iterate(&test_kvdb, &itr)) {
        loaded++;
    }
    uassert_int_equal(loaded, TEST_KV_MAX_NUM);
    /* ... and it is given back as soon as the iteration ends */
    uassert_null(test_kvdb.parent.read_cache);

    /* what the cached run reads back must be what was written */
    for (i = 0; i < TEST_KV_MAX_NUM; i++) {
        snprintf(key, sizeof(key), "rc_%u", i);
        snprintf(value, sizeof(value), "%u", i);
        read_value = fdb_kv_get(&test_kvdb, key);
        uassert_not_null(read_value);
        uassert_str_equal(read_value, value);
    }

    /* leave the database as the following cases expect it */
    for (i = 0; i < TEST_KV_MAX_NUM; i++) {
        snprintf(key, sizeof(key), "rc_%u", i);
        uassert_true(fdb_kv_del(&test_kvdb, key) == FDB_NO_ERR);
    }
}
#endif /* FDB_USING_READ_CACHE */


#ifdef FDB_KV_USING_CACHE
static size_t issue_59_cache_index(uint32_t addr)
{
    size_t i;

    for (i = 0; i < FDB_KV_CACHE_TABLE_SIZE; i++) {
        if (test_kvdb.kv_cache_table[i].addr == addr) {
            return i;
        }
    }
    return FDB_KV_CACHE_TABLE_SIZE;
}

#ifdef FDB_UTEST_READ_FAULTS
enum issue_59_read_fault_mode {
    ISSUE_59_READ_OK,
    ISSUE_59_CACHED_NAME_ERROR,
    ISSUE_59_HEADER_ERROR,
    ISSUE_59_CRC_DATA_ERROR,
    ISSUE_59_RECORD_NAME_ERROR,
    ISSUE_59_GC_HEADER_ERROR
};

static struct {
    enum issue_59_read_fault_mode mode;
    uint32_t start, name_addr;
    uint32_t gc_addr;
    size_t header_size, name_size, cache_index;
    unsigned int hits, name_reads, header_skips;
    bool capture_header, in_record, cache_was_evicted;
} issue_59_read_fault;

static fdb_err_t issue_59_read_fault_hook(fdb_db_t db, uint32_t addr, void *buf,
        size_t size, fdb_err_t result)
{
    bool fail = false;

    if (result != FDB_NO_ERR || db != (fdb_db_t)&test_kvdb)
    {
        return result;
    }
    if (issue_59_read_fault.capture_header && addr == issue_59_read_fault.start)
    {
        issue_59_read_fault.header_size = size;
    }
    if (issue_59_read_fault.mode == ISSUE_59_READ_OK)
    {
        return result;
    }
    if (issue_59_read_fault.mode == ISSUE_59_GC_HEADER_ERROR
            && test_kvdb.gc_request
            && addr == issue_59_read_fault.gc_addr + _TKV_SEC_HDR_SZ
            && size == issue_59_read_fault.header_size
            && issue_59_read_fault.hits == 0)
    {
        fail = true;
    }
    else if (addr == issue_59_read_fault.start && size == issue_59_read_fault.header_size)
    {
        if (issue_59_read_fault.header_skips)
        {
            issue_59_read_fault.header_skips--;
            return result;
        }
        issue_59_read_fault.in_record = true;
        issue_59_read_fault.name_reads = 0;
        if (issue_59_read_fault.mode == ISSUE_59_CACHED_NAME_ERROR && issue_59_read_fault.hits)
        {
            issue_59_read_fault.cache_was_evicted =
                    test_kvdb.kv_cache_table[issue_59_read_fault.cache_index].addr == FDB_DATA_UNUSED;
        }
        fail = issue_59_read_fault.mode == ISSUE_59_HEADER_ERROR;
    }
    else if (addr == issue_59_read_fault.name_addr)
    {
        if (issue_59_read_fault.mode == ISSUE_59_CACHED_NAME_ERROR
                && !issue_59_read_fault.in_record && !issue_59_read_fault.hits
                && size == FDB_KV_NAME_MAX)
        {
            fail = true;
        }
        else if (issue_59_read_fault.in_record)
        {
            if (issue_59_read_fault.mode == ISSUE_59_CRC_DATA_ERROR && size == FDB_WG_ALIGN(32))
            {
                fail = true;
            }
            if (issue_59_read_fault.mode == ISSUE_59_RECORD_NAME_ERROR
                    && size == issue_59_read_fault.name_size)
            {
                /* A wide write unit makes the CRC and final name reads identical in size. */
                unsigned int name_read = issue_59_read_fault.name_size == FDB_WG_ALIGN(32) ? 2 : 1;
                issue_59_read_fault.name_reads++;
                fail = issue_59_read_fault.name_reads == name_read;
            }
        }
    }
    if (fail)
    {
        /* Valid bytes must still be rejected when the read reports failure. */
        issue_59_read_fault.hits++;
        issue_59_read_fault.in_record = false;
        return FDB_READ_ERR;
    }
    return result;
}

static bool issue_59_read_fault_target(const char *key, fdb_kv_t kv)
{
    memset(&issue_59_read_fault, 0, sizeof(issue_59_read_fault));
    if (fdb_kv_get_obj(&test_kvdb, key, kv) == NULL)
    {
        uassert_true(false);
        return false;
    }
    issue_59_read_fault.start = kv->addr.start;
    issue_59_read_fault.name_addr = kv->addr.value - FDB_WG_ALIGN(kv->name_len);
    issue_59_read_fault.name_size = FDB_WG_ALIGN(kv->name_len);
    issue_59_read_fault.cache_index = issue_59_cache_index(kv->addr.start);
    uassert_true(issue_59_read_fault.cache_index < FDB_KV_CACHE_TABLE_SIZE);
    if (issue_59_read_fault.cache_index >= FDB_KV_CACHE_TABLE_SIZE)
    {
        return false;
    }
    /* Observe the real header read instead of duplicating its private C layout. */
    issue_59_read_fault.capture_header = true;
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, kv));
    issue_59_read_fault.capture_header = false;
    uassert_true(issue_59_read_fault.header_size > 0);
    return issue_59_read_fault.header_size > 0;
}

static bool issue_59_read_fault_prepare(const char *key, fdb_kv_t kv)
{
    struct fdb_blob blob;
    uint8_t value[2 * _TKV_W + 33];

    memset(&issue_59_read_fault, 0, sizeof(issue_59_read_fault));
    _fdb_utest_read_fault_hook = issue_59_read_fault_hook;
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
    memset(value, 0x59, sizeof(value));
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);
    return issue_59_read_fault_target(key, kv);
}

static void issue_59_read_fault_check(enum issue_59_read_fault_mode mode)
{
    const char *key = "io59";
    struct fdb_kv kv;
    struct fdb_blob blob;
    uint8_t value[2 * _TKV_W + 33], expected[sizeof(value)];

    if (!issue_59_read_fault_prepare(key, &kv))
    {
        return;
    }
    issue_59_read_fault.mode = mode;
    if (mode == ISSUE_59_CACHED_NAME_ERROR)
    {
        /* A failed cache hint may recover through a successful full scan. */
        uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
        uassert_true(issue_59_read_fault.cache_was_evicted);
    }
    else
    {
        uassert_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    }
    issue_59_read_fault.mode = ISSUE_59_READ_OK;
    uassert_true(issue_59_read_fault.hits > 0);
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    memset(expected, 0x59, sizeof(expected));
    memset(value, 0, sizeof(value));
    uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
            fdb_blob_make(&blob, value, sizeof(value))), sizeof(value));
    uassert_buf_equal(value, expected, sizeof(value));
    _fdb_utest_read_fault_hook = NULL;
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}

static void test_fdb_github_issue_59_cached_name_read_error(void)
{
    issue_59_read_fault_check(ISSUE_59_CACHED_NAME_ERROR);
}

static void test_fdb_github_issue_59_header_read_error(void)
{
    issue_59_read_fault_check(ISSUE_59_HEADER_ERROR);
}

static void test_fdb_github_issue_59_crc_data_read_error(void)
{
    issue_59_read_fault_check(ISSUE_59_CRC_DATA_ERROR);
}

static void test_fdb_github_issue_59_record_name_read_error(void)
{
    issue_59_read_fault_check(ISSUE_59_RECORD_NAME_ERROR);
}

static void test_fdb_github_issue_59_max_key_read_error(void)
{
    char key[FDB_KV_NAME_MAX + 1];
    struct fdb_kv kv;
    struct fdb_blob blob;
    uint8_t value[_TKV_W + 1], read_value[sizeof(value)];

    memset(key, 'm', FDB_KV_NAME_MAX);
    key[FDB_KV_NAME_MAX] = '\0';
    memset(value, 0x5A, sizeof(value));
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);
    if (!issue_59_read_fault_target(key, &kv))
    {
        return;
    }

    issue_59_read_fault.mode = ISSUE_59_CACHED_NAME_ERROR;
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    uassert_true(issue_59_read_fault.hits > 0);
    uassert_true(issue_59_read_fault.cache_was_evicted);
    issue_59_read_fault.mode = ISSUE_59_READ_OK;
    _fdb_utest_read_fault_hook = NULL;

    memset(read_value, 0, sizeof(read_value));
    uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
            fdb_blob_make(&blob, read_value, sizeof(read_value))), sizeof(value));
    uassert_buf_equal(read_value, value, sizeof(value));
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}

static void test_fdb_github_issue_59_iterator_read_error(void)
{
    struct fdb_kv first, second;
    struct fdb_kv_iterator itr;
    struct fdb_blob blob;
    uint8_t value[_TKV_W + 1];
    size_t count = 0;

    if (!issue_59_read_fault_prepare("it59a", &first))
    {
        return;
    }
    memset(value, 0xA5, sizeof(value));
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, "it59b",
            fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);
    if (!issue_59_read_fault_target("it59b", &second))
    {
        return;
    }
    fdb_kv_iterator_init(&test_kvdb, &itr);
    uassert_true(fdb_kv_iterate(&test_kvdb, &itr));
    uassert_int_equal(itr.curr_kv.addr.start, first.addr.start);
    issue_59_read_fault.mode = ISSUE_59_HEADER_ERROR;
    /* get_next_kv_addr first probes the next magic word with a 32-byte read. */
    issue_59_read_fault.header_skips = issue_59_read_fault.header_size == 32 ? 1 : 0;
    uassert_true(!fdb_kv_iterate(&test_kvdb, &itr));
    issue_59_read_fault.mode = ISSUE_59_READ_OK;
    uassert_true(issue_59_read_fault.hits > 0);
    uassert_true(!itr.curr_kv.crc_is_ok);
    uassert_int_equal(itr.curr_kv.status, FDB_KV_ERR_HDR);
    uassert_int_equal(itr.curr_kv.name_len, 0);
    uassert_int_equal(itr.curr_kv.value_len, 0);
    uassert_int_equal(itr.curr_kv.addr.value, FDB_DATA_UNUSED);
    uassert_int_equal(itr.iterated_cnt, 1);
    uassert_int_equal(itr.iterated_obj_bytes, first.len);
    uassert_int_equal(itr.iterated_value_bytes, first.value_len);

    _fdb_utest_read_fault_hook = NULL;
    fdb_kv_iterator_init(&test_kvdb, &itr);
    while (fdb_kv_iterate(&test_kvdb, &itr))
    {
        count++;
    }
    uassert_int_equal(count, 2);
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}

static void test_fdb_github_issue_59_gc_read_error(void)
{
    const char *key = "gc59_read";
    struct fdb_kv kv;
    struct fdb_blob blob;
    uint8_t value[TEST_KV_VALUE_LEN], expected[sizeof(value)], read_value[sizeof(value)];
    size_t i, count = 2 * test_kvdb.parent.max_size / sizeof(value);
    bool injected = false, have_expected = false;

    memset(value, 0xA6, sizeof(value));
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);
    memcpy(expected, value, sizeof(expected));
    have_expected = true;
    _fdb_utest_read_fault_hook = issue_59_read_fault_hook;
    if (!issue_59_read_fault_target(key, &kv))
    {
        return;
    }
    issue_59_read_fault.mode = ISSUE_59_GC_HEADER_ERROR;
    for (i = 1; i < count && !injected; i++)
    {
        value[0] = (uint8_t)i;
        issue_59_read_fault.gc_addr = test_kvdb.parent.oldest_addr;
        issue_59_read_fault.hits = 0;
        if (fdb_kv_set_blob(&test_kvdb, key,
                fdb_blob_make(&blob, value, sizeof(value))) == FDB_NO_ERR)
        {
            memcpy(expected, value, sizeof(expected));
            have_expected = true;
        }
        injected = issue_59_read_fault.hits != 0;
    }
    issue_59_read_fault.mode = ISSUE_59_READ_OK;
    _fdb_utest_read_fault_hook = NULL;
    uassert_true(injected);
    uassert_true(have_expected);
    if (!injected || !have_expected)
    {
        return;
    }

    /* A failed GC must leave the source value readable before recovery. */
    memset(read_value, 0, sizeof(read_value));
    uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
            fdb_blob_make(&blob, read_value, sizeof(read_value))), sizeof(read_value));
    uassert_buf_equal(read_value, expected, sizeof(expected));

    /* The dirty-GC marker must allow the next initialization to resume safely. */
    fdb_reboot();
    memset(read_value, 0, sizeof(read_value));
    uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
            fdb_blob_make(&blob, read_value, sizeof(read_value))), sizeof(read_value));
    uassert_buf_equal(read_value, expected, sizeof(expected));
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}
#endif /* FDB_UTEST_READ_FAULTS */

static void test_fdb_github_issue_59_max_key(void)
{
    char key[FDB_KV_NAME_MAX + 1];
    uint8_t value[_TKV_W + 1], read_value[sizeof(value)];
    struct fdb_blob blob;
    struct fdb_kv kv;
    size_t key_len, i;

    memset(value, 0x59, sizeof(value));
    for (key_len = FDB_KV_NAME_MAX - 1; key_len <= FDB_KV_NAME_MAX; key_len++)
    {
        memset(key, 'k', key_len);
        key[key_len] = '\0';
        uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
                fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);

        /* The full on-media name is valid even when kv.name is NUL-truncated. */
        for (i = 0; i < 2; i++)
        {
            memset(&kv, 0, sizeof(kv));
            uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
            uassert_true(kv.crc_is_ok);
            uassert_int_equal(kv.status, FDB_KV_WRITE);
            uassert_int_equal(kv.name_len, key_len);
            uassert_int_equal(kv.value_len, sizeof(value));
            uassert_true(issue_59_cache_index(kv.addr.start) < FDB_KV_CACHE_TABLE_SIZE);
            memset(read_value, 0, sizeof(read_value));
            uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
                    fdb_blob_make(&blob, read_value, sizeof(read_value))), sizeof(value));
            uassert_buf_equal(read_value, value, sizeof(value));
        }
        uassert_int_equal(fdb_kv_del(&test_kvdb, key), FDB_NO_ERR);
    }
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}

static void test_fdb_github_issue_59_crc(void)
{
    const char *key = "issue59_crc";
    struct fdb_blob blob;
    struct fdb_kv kv;
    uint8_t value[_TKV_W + 1], damaged[FDB_WG_ALIGN(1)];
    size_t index;

    uassert_true(test_kvdb.parent.file_mode);
    if (!test_kvdb.parent.file_mode) {
        return;
    }
    memset(value, 0xA5, sizeof(value));
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    index = issue_59_cache_index(kv.addr.start);
    uassert_true(index < FDB_KV_CACHE_TABLE_SIZE);

    /* Fault injection is restricted to the emulated file medium, not NOR programming. */
    uassert_int_equal(_fdb_flash_read((fdb_db_t)&test_kvdb, kv.addr.value,
            damaged, sizeof(damaged)), FDB_NO_ERR);
    damaged[0] ^= 1;
    uassert_int_equal(_fdb_flash_write((fdb_db_t)&test_kvdb, kv.addr.value,
            damaged, sizeof(damaged), true), FDB_NO_ERR);
    uassert_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    if (index < FDB_KV_CACHE_TABLE_SIZE) {
        uassert_int_equal(test_kvdb.kv_cache_table[index].addr, FDB_DATA_UNUSED);
    }
    uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
            fdb_blob_make(&blob, value, sizeof(value))), 0);
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}

static void test_fdb_github_issue_59_stale_cache(void)
{
    const char *key = "issue59_stale";
    struct fdb_blob blob;
    struct fdb_kv kv;
    uint8_t old_value[_TKV_W + 1], new_value[_TKV_W + 2], read_value[sizeof(new_value)];
    uint32_t old_addr, new_addr;
    size_t index;

    memset(old_value, 0x11, sizeof(old_value));
    memset(new_value, 0x22, sizeof(new_value));
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, old_value, sizeof(old_value))), FDB_NO_ERR);
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    old_addr = kv.addr.start;
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, new_value, sizeof(new_value))), FDB_NO_ERR);
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    new_addr = kv.addr.start;
    index = issue_59_cache_index(new_addr);
    uassert_true(index < FDB_KV_CACHE_TABLE_SIZE);
    if (index >= FDB_KV_CACHE_TABLE_SIZE) {
        return;
    }

    /* Restore an obsolete address deliberately; normal API calls need not create it. */
    test_kvdb.kv_cache_table[index].addr = old_addr;
    uassert_int_equal(fdb_kv_get_blob(&test_kvdb, key,
            fdb_blob_make(&blob, read_value, sizeof(read_value))), sizeof(new_value));
    uassert_buf_equal(read_value, new_value, sizeof(new_value));
    uassert_int_equal(test_kvdb.kv_cache_table[index].addr, new_addr);

    uassert_int_equal(fdb_kv_del(&test_kvdb, key), FDB_NO_ERR);
    test_kvdb.kv_cache_table[index].addr = new_addr;
    uassert_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    uassert_int_equal(test_kvdb.kv_cache_table[index].addr, FDB_DATA_UNUSED);

    /* A committed empty blob must remain distinguishable from a failed lookup. */
    uassert_int_equal(fdb_kv_set_blob(&test_kvdb, key,
            fdb_blob_make(&blob, new_value, 0)), FDB_NO_ERR);
    uassert_not_null(fdb_kv_get_obj(&test_kvdb, key, &kv));
    uassert_int_equal(kv.value_len, 0);
    uassert_int_equal(fdb_kv_del(&test_kvdb, key), FDB_NO_ERR);
}

static void test_fdb_github_issue_59_gc(void)
{
    struct fdb_blob blob;
    uint8_t value[TEST_KV_VALUE_LEN];
    uint32_t previous_oldest = test_kvdb.parent.oldest_addr;
    size_t i, count = 2 * test_kvdb.parent.max_size / sizeof(value);
    bool gc_ran = false;

    memset(value, 0x59, sizeof(value));
    for (i = 0; i < count; i++) {
        value[0] = (uint8_t)i;
        uassert_int_equal(fdb_kv_set_blob(&test_kvdb, "issue59_gc",
                fdb_blob_make(&blob, value, sizeof(value))), FDB_NO_ERR);
        if (test_kvdb.parent.oldest_addr != previous_oldest) {
            gc_ran = true;
        }
        previous_oldest = test_kvdb.parent.oldest_addr;
    }
    uassert_true(gc_ran);
    test_fdb_github_issue_59_stale_cache();
    uassert_int_equal(fdb_kv_set_default(&test_kvdb), FDB_NO_ERR);
}

#endif /* FDB_KV_USING_CACHE */

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
    UTEST_UNIT_RUN(test_fdb_gc2);
    UTEST_UNIT_RUN(test_fdb_scale_up);
    UTEST_UNIT_RUN(test_fdb_kvdb_set_default);
#ifdef FDB_KV_USING_CACHE
    UTEST_UNIT_RUN(test_fdb_github_issue_59_max_key);
#ifdef FDB_UTEST_READ_FAULTS
    UTEST_UNIT_RUN(test_fdb_github_issue_59_cached_name_read_error);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_header_read_error);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_crc_data_read_error);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_record_name_read_error);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_max_key_read_error);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_iterator_read_error);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_gc_read_error);
#endif
    UTEST_UNIT_RUN(test_fdb_github_issue_59_crc);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_stale_cache);
    UTEST_UNIT_RUN(test_fdb_github_issue_59_gc);
#endif
#if FDB_USING_READ_CACHE
    UTEST_UNIT_RUN(test_fdb_read_cache);
#endif
    UTEST_UNIT_RUN(test_fdb_kvdb_deinit);
}
UTEST_TC_EXPORT(testcase, "packages.system.flashdb.kvdb", utest_tc_init, utest_tc_cleanup, 20);

#endif /* defined(RT_USING_UTEST) && defined(FDB_USING_KVDB) */
