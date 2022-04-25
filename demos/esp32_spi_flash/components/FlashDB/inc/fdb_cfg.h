/*
 * Copyright (c) 2022, kaans, <https://github.com/kaans>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief configuration file
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

/* Using FAL storage mode */
#define FDB_USING_FAL_MODE

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f2/f4)/ 32(stm32f1) */
#define FDB_WRITE_GRAN 1 /* @note you must define it for a value */

/* MCU Endian Configuration, default is Little Endian Order. */
/* #define FDB_BIG_ENDIAN  */

/* log print macro using ESP_LOG.
 * Need to include esp_log.h when setting FDB_PRINT to ESP_LOGI().
 * default EF_PRINT macro is printf() */
//#include <esp_log.h>
//#define FDB_PRINT(...) ESP_LOGI("fdb", __VA_ARGS__)

/* print debug information */
#define FDB_DEBUG_ENABLE

#endif /* _FDB_CFG_H_ */
