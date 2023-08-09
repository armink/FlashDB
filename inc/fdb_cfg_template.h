/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief configuration file
 */

#ifndef _FDB_CFG_H_
#define _FDB_CFG_H_

#include "SEGGER_RTT.h"

/* using KVDB feature */
#define FDB_USING_KVDB

#ifdef FDB_USING_KVDB
/* Auto update KV to latest default when current KVDB version number is changed. @see fdb_kvdb.ver_num */
/* #define FDB_KV_AUTO_UPDATE */
#endif

/* using TSDB (Time series database) feature */
//#define FDB_USING_TSDB

/* Using file storage mode by LIBC file API, like fopen/fread/fwrte/fclose */
/* #define FDB_USING_FILE_LIBC_MODE */

/* Using file storage mode by LIBC file API, like fopen/fread/fwrte/fclose */
#define FDB_USING_FILE_LITTLEFS_MODE

/* Using file storage mode by POSIX file API, like open/read/write/close */
/* #define FDB_USING_FILE_POSIX_MODE */

/* MCU Endian Configuration, default is Little Endian Order. */
/* #define FDB_BIG_ENDIAN */ 

/* log print macro. default EF_PRINT macro is printf() */
#if DEBUG_
#define FDB_PRINT(...)              SEGGER_RTT_printf(0U, __VA_ARGS__)
#else
#define FDB_PRINT(...)
#endif

/* print debug information */
#define FDB_DEBUG_ENABLE

#endif /* _FDB_CFG_H_ */
