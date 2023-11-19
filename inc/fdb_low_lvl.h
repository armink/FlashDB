/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief low level API and definition
 */

#ifndef _FDB_LOW_LVL_H_
#define _FDB_LOW_LVL_H_

#include <fdb_cfg.h>
#include <fdb_def.h>

#if (FDB_WRITE_GRAN == 1)
#define FDB_STATUS_TABLE_SIZE(status_number)       ((status_number * FDB_WRITE_GRAN + 7)/8)
#else
#define FDB_STATUS_TABLE_SIZE(status_number)       (((status_number - 1) * FDB_WRITE_GRAN + 7)/8)
#endif

/* the data is erased */
#define FDB_BYTE_ERASED                           0xFF
/* the data is written */
#define FDB_BYTE_WRITTEN                          0x00

/* Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define FDB_ALIGN(size, align)                    (((size) + (align) - 1) - (((size) + (align) -1) % (align)))
/* align by write granularity */
#define FDB_WG_ALIGN(size)                        (FDB_ALIGN(size, ((FDB_WRITE_GRAN + 7)/8)))
/**
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define FDB_ALIGN_DOWN(size, align)               (((size) / (align)) * (align))
/* align down by write granularity */
#define FDB_WG_ALIGN_DOWN(size)                   (FDB_ALIGN_DOWN(size, (FDB_WRITE_GRAN + 7)/8))

#define FDB_STORE_STATUS_TABLE_SIZE               FDB_STATUS_TABLE_SIZE(FDB_SECTOR_STORE_STATUS_NUM)
#define FDB_DIRTY_STATUS_TABLE_SIZE               FDB_STATUS_TABLE_SIZE(FDB_SECTOR_DIRTY_STATUS_NUM)

/* the data is unused */
#if (FDB_BYTE_ERASED  == 0xFF)
#define FDB_DATA_UNUSED                      0xFFFFFFFF
#else
#define FDB_DATA_UNUSED                      0x00000000
#endif

/* invalid address */
#define FDB_FAILED_ADDR                      0xFFFFFFFF

size_t _fdb_set_status(uint8_t status_table[], size_t status_num, size_t status_index);
size_t _fdb_get_status(uint8_t status_table[], size_t status_num);
uint32_t _fdb_continue_ff_addr(fdb_db_t db, uint32_t start, uint32_t end);
fdb_err_t _fdb_init_ex(fdb_db_t db, const char *name, const char *part_name, fdb_db_type type, void *user_data);
void _fdb_init_finish(fdb_db_t db, fdb_err_t result);
void _fdb_deinit(fdb_db_t db);
const char *_fdb_db_path(fdb_db_t db);
fdb_err_t _fdb_write_status(fdb_db_t db, uint32_t addr, uint8_t status_table[], size_t status_num, size_t status_index, bool sync);
size_t _fdb_read_status(fdb_db_t db, uint32_t addr, uint8_t status_table[], size_t total_num);
fdb_err_t _fdb_flash_read(fdb_db_t db, uint32_t addr, void *buf, size_t size);
fdb_err_t _fdb_flash_erase(fdb_db_t db, uint32_t addr, size_t size);
fdb_err_t _fdb_flash_write(fdb_db_t db, uint32_t addr, const void *buf, size_t size, bool sync);

#endif /* _FDB_LOW_LVL_H_ */
