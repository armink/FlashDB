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

/* Using FAL storage mode */
#define FDB_USING_FAL_MODE

#ifdef FDB_USING_FAL_MODE
/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f2/f4)/ 32(stm32f1)/ 64(stm32f7)/ 128(stm32h5)/ 256(stm32h7) */
#define FDB_WRITE_GRAN                /* @note you must define it for a value */
#endif

/* Using file storage mode by LIBC file API, like fopen/fread/fwrte/fclose */
/* #define FDB_USING_FILE_LIBC_MODE */

/* Using file storage mode by POSIX file API, like open/read/write/close */
/* #define FDB_USING_FILE_POSIX_MODE */

/* Using a transient sector read cache.
 * FlashDB keeps no persisted record of how much of a sector is used, so a KV
 * search has to read the sector chunk by chunk up to its end. This cache turns
 * that scan into a single read of the sector, see fdb_read_cache.c.
 *
 * The cache lives inside the initialization and inside an application driven
 * iteration, and is given back at the end of each of those two phases, so a single
 * key read or write at run time never holds a sector sized buffer in RAM.
 *
 * Define it to 1 to enable the cache. It carries a value instead of being defined
 * or not, so a build can override it without editing this file. It always takes a
 * value: a bare -DFDB_USING_READ_CACHE would leave the expressions without one.
 */
#ifndef FDB_USING_READ_CACHE
#define FDB_USING_READ_CACHE 0
#endif

#if FDB_USING_READ_CACHE
/* The number of bytes the read cache copies into RAM at once. It is both the RAM
 * window and the granularity of every copy: the window covering the address of a
 * read is copied on demand, when the read lands outside the one currently held.
 *
 * 0 (the default) makes the window the whole sector, which is the right value
 * for a normal build: the scan this cache exists for walks to the end of the
 * sector anyway, so copying the sector in one piece is the fewest accesses the
 * loading phase can take.
 *
 * Lower it only to cap the RAM of that copy, for example on a device where a
 * transient whole-sector allocation is a problem, or if the sector size is raised
 * and the window would grow with it. A value which is not smaller than the sector
 * size has no effect.
 */
#ifndef FDB_READ_CACHE_WINDOW
#define FDB_READ_CACHE_WINDOW 0
#endif
#endif /* FDB_USING_READ_CACHE */
/* MCU Endian Configuration, default is Little Endian Order. */
/* #define FDB_BIG_ENDIAN */ 

/* log print macro. default EF_PRINT macro is printf() */
/* #define FDB_PRINT(...)              my_printf(__VA_ARGS__) */

/* print debug information */
#define FDB_DEBUG_ENABLE

#endif /* _FDB_CFG_H_ */
