/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sfud.h>

sfud_flash sfud_norflash0 = {
        .name = "norflash0",
        .spi.name = "SPI2",
        .chip = { "W25Q64FV", SFUD_MF_ID_WINBOND, 0x40, 0x17, 8L * 1024L * 1024L, SFUD_WM_PAGE_256B, 4096, 0x20 } };

int spi_flash_init(void)
{
    /* SFUD initialize */
    if (sfud_device_init(&sfud_norflash0) == SFUD_SUCCESS) {
        return 0;
    } else {
        return -1;
    }
}
