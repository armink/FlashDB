/*
 * Copyright (c) 2022, Spider, <spider@spider.vc>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fal.h>
#include <stm32l4xx.h>

static int init(void)
{
    /* do nothing now */
    return 1;
}


static int ef_err_port_cnt = 0;
int on_ic_read_cnt  = 0;
int on_ic_write_cnt = 0;

void feed_dog(void)
{

}

static int read(long offset, uint8_t *buf, size_t size)
{

    size_t i;
    uint32_t addr = stm32_onchip_flash.addr + offset;

    if( addr%4 != 0)
        ef_err_port_cnt++;

    for (i = 0; i < size; i++, addr++, buf++)
    {
        *buf = *(uint8_t *) addr;
    }
    on_ic_read_cnt++;
    return size;
}


static int write(long offset, const uint8_t *buf, size_t size)
{
    size_t   i;
    uint32_t addr = stm32_onchip_flash.addr + offset;

    __ALIGN_BEGIN uint64_t write_data __ALIGN_END;
    __ALIGN_BEGIN uint64_t read_data  __ALIGN_END;  

    if(addr%4 != 0)
        ef_err_port_cnt++;

/*
    if((int)buf%8 != 0)
        ef_err_port_cnt++;
*/

    HAL_FLASH_Unlock();
    for (i = 0; i < size; i += 8, buf+=8, addr += 8) {
        memcpy(&write_data, buf, 8); //The third parameter used to ensure HAL_FLASH_Program is the alignment of the first address in memory
        if (write_data != 0xFFFFFFFFFFFFFFFF)
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, write_data);
        read_data = *(uint64_t *)addr;
        /* You can add your code under here. */
        if (read_data != write_data) {
            HAL_FLASH_Lock(); 
            return -1;
        }
        else{
			//FLash操作可能非常耗时，如果有看门狗需要喂狗，以下代码由用户实现
           feed_dog();
        }
    }
    HAL_FLASH_Lock();

    on_ic_write_cnt++;
    return size;
}


static int erase(long offset, size_t size)
{
    // uint32_t addr = stm32_onchip_flash.addr + offset;

    HAL_StatusTypeDef flash_status;
    size_t erase_pages, i;
    uint32_t PAGEError = 0;

    erase_pages = size / FLASH_PAGE_SIZE;
    if (size % FLASH_PAGE_SIZE != 0) {
        erase_pages++;
    }

    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.NbPages     = 1;  //Wipe out one sector at a time to perform one feeding of the dog to prevent timeout
    HAL_FLASH_Unlock();
    
    for (i = 0; i < erase_pages; i++) {
        EraseInitStruct.Page = (offset/FLASH_PAGE_SIZE) + i;
        flash_status = HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
        if (flash_status != HAL_OK) {
            HAL_FLASH_Lock(); 
            return -1;
        }
        else{
			//FLash operation can be very time-consuming, if there is a watchdog that needs to feed the dog, the following code is implemented by the user
            feed_dog();
        }
    }
    HAL_FLASH_Lock(); 

    return size;
}


/*
  "stm32_onchip" : Flash 设备的名字。
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

const struct fal_flash_dev stm32_onchip_flash =
{
    .name       = "stm32_onchip",
    .addr       = 0x08000000,
    .len        = 256*1024,
    .blk_size   = FLASH_PAGE_SIZE,
    .ops        = {init, read, write, erase},
    .write_gran = 64
};


