/*
 * Copyright (c) 2022, kaans, <https://github.com/kaans>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include <esp_partition.h>

#include <string.h>
#include <fal.h>

#define FLASH_ERASE_MIN_SIZE (4 * 1024)

#define LOCKER_ENABLE
#ifdef LOCKER_ENABLE
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_lock = NULL;

const static esp_partition_t *partition;

#define LOCK()                                 \
    do                                         \
    {                                          \
        xSemaphoreTake(s_lock, portMAX_DELAY); \
    } while (0)

#define UNLOCK()                \
    do                          \
    {                           \
        xSemaphoreGive(s_lock); \
    } while (0)
#else
#define LOCK()
#define UNLOCK()
#endif

static int init(void)
{
#ifdef LOCKER_ENABLE
    if (s_lock == NULL)
    {
        s_lock = xSemaphoreCreateCounting(1, 1);
        assert(s_lock != NULL);
    }
#endif

    // the values passed to esp_partition_find_first() must correspond to the
    // values set in partitions.csv for the partition named "flashdb".
    partition = esp_partition_find_first(0x40, 0x00, "flashdb");

    assert(partition != NULL);

    return 1;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    esp_err_t ret;

    LOCK();
    ret = esp_partition_read(partition, offset, buf, size);
    UNLOCK();

    return ret;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    esp_err_t ret;

    LOCK();
    ret = esp_partition_write(partition, offset, buf, size);
    UNLOCK();

    return ret;
}

static int erase(long offset, size_t size)
{
    esp_err_t ret;
    int32_t erase_size = ((size - 1) / FLASH_ERASE_MIN_SIZE) + 1;

    LOCK();
    ret = esp_partition_erase_range(partition, offset, erase_size * FLASH_ERASE_MIN_SIZE);
    UNLOCK();

    return ret;
}

const struct fal_flash_dev nor_flash0 =
    {
        .name = NOR_FLASH_DEV_NAME,
        .addr = 0x0,                      // address is relative to beginning of partition; 0x0 is start of the partition
        .len = 32 * 1024,                 // size of the partition as specified in partitions.csv
        .blk_size = FLASH_ERASE_MIN_SIZE, // must be 4096 bytes
        .ops = {init, read, write, erase},
        .write_gran = 1, // 1 byte write granularity
};
