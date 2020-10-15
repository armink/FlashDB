/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-04-20     chenyaxing   first version
 */

#include <stm32f4xx.h>

extern int rt_hw_usart_init(void);
extern void system_clock_config(int);
extern int clock_information(void);

void clk_init(char *clk_source, int source_freq, int target_freq)
{
    HAL_Init();

    /* disbale interrupt */
    __set_PRIMASK(1);
    /* system clock config */
    system_clock_config(target_freq);
    /* init system tick to 1KHz */
#if defined (SOC_SERIES_STM32H7)
    HAL_SYSTICK_Config((HAL_RCCEx_GetD1SysClockFreq()) / 1000);
#else
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
#endif
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
    /* enable interrupt */
    __set_PRIMASK(0);
}

int uart_init()
{
    rt_hw_usart_init();
    clock_information();
    return 0;
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void _error_handler(char *s, int num)
{
    /* USER CODE BEGIN Error_Handler */
    /* User can add his own implementation to report the HAL error return state */
    while (1)
    {
    }
    /* USER CODE END Error_Handler */
}
