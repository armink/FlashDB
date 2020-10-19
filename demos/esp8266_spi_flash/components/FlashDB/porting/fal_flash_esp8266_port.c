/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_spi_flash.h"

#include <string.h>
#include <fal.h>

#define FLASH_ERASE_MIN_SIZE    (4 * 1024)

#define LOCKER_ENABLE
#ifdef LOCKER_ENABLE
#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t s_lock = NULL;

#define LOCK()                                  \
    do {                                        \
        xSemaphoreTake(s_lock, portMAX_DELAY);  \
    } while(0)

#define UNLOCK()                                \
    do {                                        \
        xSemaphoreGive(s_lock);                 \
    } while(0)
#else
#define LOCK()
#define UNLOCK()
#endif

static int init(void)
{
#ifdef LOCKER_ENABLE
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateCounting(1, 1);
        assert(s_lock != NULL);
    }
#endif
    return 1;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    //Xassert(size % 4 == 0);

    /* You can add your code under here. */
    int32_t ret;
    uint32_t addr = nor_flash0.addr + offset;

    
    LOCK();
    ret = spi_flash_read(addr, buf, size);
    UNLOCK();

    return ret;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    int32_t ret;
    uint32_t addr = nor_flash0.addr + offset;
    
    LOCK();
    ret = spi_flash_write(addr, buf, size);
    UNLOCK();
    
    return ret;
}

static int erase(long offset, size_t size)
{
    int32_t ret;
    uint32_t addr = nor_flash0.addr + offset;

    int32_t erase_size = ((size - 1) / FLASH_ERASE_MIN_SIZE) + 1;

    LOCK();
    ret = spi_flash_erase_range(addr, erase_size * FLASH_ERASE_MIN_SIZE);
    UNLOCK();
    
    return ret;
}

/*
  "esp8266_onchip" : Flash 设备的名字。
  0x08000000: 对 Flash 操作的起始地址。
  1024*1024：Flash 的总大小（1MB）。
  128*1024：Flash 块/扇区大小（因为 STM32F2 各块大小不均匀，所以擦除粒度为最大块的大小：128K）。
  {init, read, write, erase} ：Flash 的操作函数。 如果没有 init 初始化过程，第一个操作函数位置可以置空。
  8 : 设置写粒度，单位 bit， 0 表示未生效（默认值为 0 ），该成员是 fal 版本大于 0.4.0 的新增成员。各个 flash 写入粒度不尽相同，可通过该成员进行设置，以下列举几种常见 Flash 写粒度：
  nor flash:  1 bit
  stm32f2/f4: 8 bit
  stm32f1:    32 bit
  stm32l4:    64 bit
 */

//1.定义 flash 设备

const struct fal_flash_dev nor_flash0 =
{
    .name       = "norflash0",
    .addr       = 0x100000,
    .len        = 50*1024,
    .blk_size   = FLASH_ERASE_MIN_SIZE,
    .ops        = {init, read, write, erase},
    .write_gran = 1
};

