/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs.
 */

#ifndef _FLASHDB_H_
#define _FLASHDB_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <fdb_cfg.h>

#ifdef FDB_USING_FAL_MODE
#include <fal.h>
#endif

#include <fdb_def.h>


#ifdef __cplusplus
extern "C" {
#endif

/* FlashDB database API */
fdb_err_t fdb_kvdb_init   (fdb_kvdb_t db, const char *name, const char *path, struct fdb_default_kv *default_kv,
        void *user_data);
void      fdb_kvdb_control(fdb_kvdb_t db, int cmd, void *arg);
fdb_err_t fdb_kvdb_check(fdb_kvdb_t db);
fdb_err_t fdb_kvdb_deinit(fdb_kvdb_t db);
fdb_err_t fdb_tsdb_init   (fdb_tsdb_t db, const char *name, const char *path, fdb_get_time get_time, size_t max_len,
        void *user_data);
void      fdb_tsdb_control(fdb_tsdb_t db, int cmd, void *arg);
fdb_err_t fdb_tsdb_deinit(fdb_tsdb_t db);

/* blob API */
fdb_blob_t fdb_blob_make     (fdb_blob_t blob, const void *value_buf, size_t buf_len);
size_t     fdb_blob_read     (fdb_db_t db, fdb_blob_t blob);

/* Key-Value API like a KV DB */
fdb_err_t         fdb_kv_set          (fdb_kvdb_t db, const char *key, const char *value);
char             *fdb_kv_get          (fdb_kvdb_t db, const char *key);
fdb_err_t         fdb_kv_set_blob     (fdb_kvdb_t db, const char *key, fdb_blob_t blob);
size_t            fdb_kv_get_blob     (fdb_kvdb_t db, const char *key, fdb_blob_t blob);
fdb_err_t         fdb_kv_del          (fdb_kvdb_t db, const char *key);
fdb_kv_t          fdb_kv_get_obj      (fdb_kvdb_t db, const char *key, fdb_kv_t kv);
fdb_blob_t        fdb_kv_to_blob      (fdb_kv_t   kv, fdb_blob_t blob);
fdb_err_t         fdb_kv_set_default  (fdb_kvdb_t db);
void              fdb_kv_print        (fdb_kvdb_t db);
fdb_kv_iterator_t fdb_kv_iterator_init(fdb_kvdb_t db, fdb_kv_iterator_t itr);
bool              fdb_kv_iterate      (fdb_kvdb_t db, fdb_kv_iterator_t itr);

/* Time series log API like a TSDB */
fdb_err_t  fdb_tsl_append      (fdb_tsdb_t db, fdb_blob_t blob);
fdb_err_t  fdb_tsl_append_with_ts(fdb_tsdb_t db, fdb_blob_t blob, fdb_time_t timestamp);
void       fdb_tsl_iter        (fdb_tsdb_t db, fdb_tsl_cb cb, void *cb_arg);
void       fdb_tsl_iter_reverse(fdb_tsdb_t db, fdb_tsl_cb cb, void *cb_arg);
void       fdb_tsl_iter_by_time(fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_cb cb, void *cb_arg);
size_t     fdb_tsl_query_count (fdb_tsdb_t db, fdb_time_t from, fdb_time_t to, fdb_tsl_status_t status);
fdb_err_t  fdb_tsl_set_status  (fdb_tsdb_t db, fdb_tsl_t tsl, fdb_tsl_status_t status);
void       fdb_tsl_clean       (fdb_tsdb_t db);
fdb_blob_t fdb_tsl_to_blob     (fdb_tsl_t tsl, fdb_blob_t blob);

/* fdb_utils.c */
uint32_t   fdb_calc_crc32(uint32_t crc, const void *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _FLASHDB_H_ */
