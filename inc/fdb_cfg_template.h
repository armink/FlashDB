/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief configuration template file. You need to rename the file to "fbd_cfg.h" and modify the configuration items in it to suit your use.
 */

#ifndef _FDB_CFG_H_
#define _FDB_CFG_H_

/* using KVDB feature */
#define FDB_USING_KVDB

#ifdef FDB_USING_KVDB
/* Auto update KV to latest default when current KVDB version number is changed. @see fdb_kvdb.ver_num */
/* #define FDB_KV_AUTO_UPDATE */
#endif

/* using TSDB (Time series database) feature */
#define FDB_USING_TSDB

/* Use fixed-size blobs in TSDB to save flash overhead (8 bytes per entry).
 * Define this to the fixed blob size in bytes when all TSL entries are the same size.
 * Ideal for logging fixed-size sensor data (e.g., float + timestamp).
 * Warning: If defined will be incompatible with variable blob flash store or if fixed blob size is later changed */
/* #define FDB_TSDB_FIXED_BLOB_SIZE 4 */


/* Use sequential mode - eliminates per-entry timestamp storage by calculating timestamps
 * on-the-fly, saving sizeof(fdb_time_t). In this mode timestamps are monotonically increasing by 1 on each log
 * and don't require application to provide a time function.
 * Warning: If defined will be incompatible with time flash store */
/* #define FDB_TSDB_USING_SEQ_MODE */

/* Using FAL storage mode */
#define FDB_USING_FAL_MODE

#ifdef FDB_USING_FAL_MODE
/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f2/f4)/ 32(stm32f1)/ 64(stm32f7)/ 128(stm32h5) */
#define FDB_WRITE_GRAN                /* @note you must define it for a value */
#endif

/* Using file storage mode by LIBC file API, like fopen/fread/fwrte/fclose */
/* #define FDB_USING_FILE_LIBC_MODE */

/* Using file storage mode by POSIX file API, like open/read/write/close */
/* #define FDB_USING_FILE_POSIX_MODE */

/* MCU Endian Configuration, default is Little Endian Order. */
/* #define FDB_BIG_ENDIAN */ 

/* log print macro. default EF_PRINT macro is printf() */
/* #define FDB_PRINT(...)              my_printf(__VA_ARGS__) */

/* print debug information */
#define FDB_DEBUG_ENABLE

#endif /* _FDB_CFG_H_ */
