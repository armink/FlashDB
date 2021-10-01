/*
 * Copyright (c) 2021, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <flashdb.h>
#include <time.h>

#define FDB_LOG_TAG "[main]"

#ifdef FDB_KV_CACHE_HASH_ENHANCEMENT
static struct fdb_cache_hash_enhancement_ops hash_ops;
#endif

static pthread_mutex_t kv_locker, ts_locker;
static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
    {"username", "armink", 0},                       /* string KV */
    {"password", "123456", 0},                       /* string KV */
    {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
    {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};
/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };
/* TSDB object */
struct fdb_tsdb tsdb = { 0 };
/* counts for simulated timestamp */
static int counts = 0;

extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_string_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);
extern void kvdb_bench_sample(fdb_kvdb_t kvdb);
extern void tsdb_sample(fdb_tsdb_t tsdb);

static void lock(fdb_db_t db)
{
    pthread_mutex_lock((pthread_mutex_t *)db->user_data);
}

static void unlock(fdb_db_t db)
{
    pthread_mutex_unlock((pthread_mutex_t *)db->user_data);
}

static fdb_time_t get_time(void)
{
    return time(NULL);
}

#ifdef FDB_KV_CACHE_HASH_ENHANCEMENT
#include <stdlib.h>
#include <string.h>
static uint32_t *hash_memory;

/* return the size of the memory block in byte, 
   the memory block is composed of uint32_t
   The block size MUST BE a multiple of four.*/
static int hash_init(uint32_t default_value)
{
    size_t elements_cnt = 4096; 
    size_t size = 4 * elements_cnt;
    
    hash_memory = (uint32_t *)malloc(size);
    memset(hash_memory, default_value, size);

    return size;
}

/* return an element of the memory block */
static uint32_t hash_read(long offset)
{
    return hash_memory[offset];
}

/* write an element to the memory block */
static int hash_write(long offset, const uint32_t addr)
{
    hash_memory[offset] = addr;
    return 1;
}
#endif


int main(void)
{
    fdb_err_t result;
    bool file_mode = true;
    uint32_t sec_size = 4096, db_size = sec_size * 1024;

#ifdef FDB_KV_CACHE_HASH_ENHANCEMENT
    hash_ops.init = hash_init;
    hash_ops.read = hash_read;
    hash_ops.write = hash_write;
#endif

#ifdef FDB_USING_KVDB
    { /* KVDB Sample */
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        /* set the lock and unlock function if you want */
        pthread_mutex_init(&kv_locker, NULL);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);
        /* set the sector and database max size */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_SEC_SIZE, &sec_size);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_MAX_SIZE, &db_size);
        /* enable file mode */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_FILE_MODE, &file_mode);
        /* create database directory */
        mkdir("fdb_kvdb1", 0777);
#ifdef FDB_KV_CACHE_HASH_ENHANCEMENT
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_HASH_OPS, &hash_ops);
#endif
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *  &kv_locker: The locker object.
         */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, &kv_locker);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
        /* run string KV samples */
        kvdb_type_string_sample(&kvdb);
        /* run blob KV samples */
        kvdb_type_blob_sample(&kvdb);

        /* run bench test */
        {
            clock_t stick =  clock();
            kvdb_bench_sample(&kvdb);
            FDB_INFO("\nBench time: %d\n\n",clock() - stick);
        }
    }
#endif /* FDB_USING_KVDB */

#ifdef FDB_USING_TSDB
    { /* TSDB Sample */
        /* set the lock and unlock function if you want */
        pthread_mutex_init(&ts_locker, NULL);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, (void *)lock);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, (void *)unlock);
        /* set the sector and database max size */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_SEC_SIZE, &sec_size);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_MAX_SIZE, &db_size);
        /* enable file mode */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_FILE_MODE, &file_mode);
        /* create database directory */
        mkdir("fdb_tsdb1", 0777);
        /* Time series database initialization
         *
         *       &tsdb: database object
         *       "log": database name
         * "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         *    get_time: The get current timestamp function.
         *         128: maximum length of each log
         *   ts_locker: The locker object.
         */
        result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, &ts_locker);
        /* read last saved time for simulated timestamp */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run TSDB sample */
        tsdb_sample(&tsdb);
    }
#endif /* FDB_USING_TSDB */

    return 0;
}
