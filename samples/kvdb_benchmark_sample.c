/*
 * Copyright (c) 2020, LianYang, <lian.yang.cn@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief benchmark KV samples.
 *
 * write some KV entries and read it out
 */

#include <flashdb.h>
#include <string.h>

#ifdef FDB_USING_KVDB

#define FDB_LOG_TAG "[sample][kvdb][bench]"

#define ROUND (200)

void kvdb_bench_sample(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;
    char bench_key[FDB_KV_NAME_MAX];
    char bench_value[FDB_KV_NAME_MAX];

    FDB_INFO("==================== kvdb_bench_sample ====================\n");

    if (fdb_kv_get(kvdb, "1")==NULL)
    { /* SET the KV value */
        for (int i = 0; i<ROUND; i++) {
            snprintf(bench_key, FDB_KV_NAME_MAX-1, "%d", i);
            snprintf(bench_value, FDB_KV_NAME_MAX-1, "VALUE%d", i);

            fdb_kv_set(kvdb, bench_key, bench_value);
            FDB_INFO("set the '%8s' value to %12s\n", bench_key, bench_value);
        }
    }
    
    { /* GET the KV value */
        for (int i = 0; i<ROUND; i++ ) {
            snprintf(bench_key, FDB_KV_NAME_MAX-1, "%d", i);

            char* read_value = fdb_kv_get(kvdb, bench_key);
            FDB_INFO("get the '%8s' value is %12s\n", bench_key, read_value);
        }
    }

    FDB_INFO("===========================================================\n");
}

#endif /* FDB_USING_KVDB */
