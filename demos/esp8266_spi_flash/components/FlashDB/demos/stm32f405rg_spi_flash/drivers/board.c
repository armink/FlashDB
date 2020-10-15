/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-05-31     yaxing.chen  the first version
 */

#include <stdio.h>
#include <stdint.h>
#include <board.h>

static void rtthread_startup(void)
{
    extern int main(void);
    extern int $Super$$main(void);
    extern void clk_init(char *clk_source, int source_freq, int target_freq);
    extern int uart_init();
    extern int spi_flash_init(void);

    clk_init(BSP_CLOCK_SOURCE, BSP_CLOCK_SOURCE_FREQ_MHZ, BSP_CLOCK_SYSTEM_FREQ_MHZ);
    uart_init();
    spi_flash_init();

    setbuf(stdout, NULL);

    /* invoke system main function */
#if defined(__CC_ARM) || defined(__CLANG_ARM)
    $Super$$main(); /* for ARMCC. */
#elif defined(__ICCARM__) || defined(__GNUC__)
    main();
#endif

    while(1);
}

#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern int $Super$$main(void);
/* re-define main function */
int $Sub$$main(void)
{
    rtthread_startup();
    return 0;
}
#elif defined(__ICCARM__)
extern int main(void);
/* __low_level_init will auto called by IAR cstartup */
extern void __iar_data_init3(void);
int __low_level_init(void)
{
    // call IAR table copy function.
    __iar_data_init3();
    rtthread_startup();
    return 0;
}
#elif defined(__GNUC__)
/* Add -eentry to arm-none-eabi-gcc argument */
int entry(void)
{
    rtthread_startup();
    return 0;
}
#endif
