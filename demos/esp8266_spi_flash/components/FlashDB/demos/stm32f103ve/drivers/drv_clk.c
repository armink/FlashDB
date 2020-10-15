/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-10-26     ChenYong     first version
 * 2020-01-08     xiangxistu   add HSI configuration
 * 2020-04-20     chenyaxing   add system_clock_config frequency parameter
 */

#include <stm32f1xx.h>
#include <stdio.h>

void _error_handler(char *s, int num);

#ifndef Error_Handler
#define Error_Handler() _error_handler(__FILE__, __LINE__)
#endif

void system_clock_config(int target_freq_Mhz)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
#if defined(STM32F100xB) || defined(STM32F100xE)
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
#endif
#if defined(STM32F101x6) || defined(STM32F101xB) || defined(STM32F101xE) || defined(STM32F101xG)
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
#endif
#if defined(STM32F102x6) || defined(STM32F102xB)
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
#endif
#if defined(STM32F103x6) || defined(STM32F103xB) || defined(STM32F103xE) || defined(STM32F103xG)
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
#endif
#if defined(STM32F105xC) || defined(STM32F107xC)
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
#endif
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

#if defined(STM32F100xB) || defined(STM32F100xE)
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
#endif
#if defined(STM32F101x6) || defined(STM32F101xB) || defined(STM32F101xE) || defined(STM32F101xG)
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
#endif
#if defined(STM32F102x6) || defined(STM32F102xB)
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
#endif
#if defined(STM32F103x6) || defined(STM32F103xB) || defined(STM32F103xE) || defined(STM32F103xG)
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
#endif
#if defined(STM32F105xC) || defined(STM32F107xC)
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
#endif
    {
        Error_Handler();
    }
}

int clock_information(void)
{
    printf("System Clock information\n");
    printf("SYSCLK_Frequency = %d\n", (int) HAL_RCC_GetSysClockFreq());
    printf("HCLK_Frequency   = %d\n", (int) HAL_RCC_GetHCLKFreq());
    printf("PCLK1_Frequency  = %d\n", (int) HAL_RCC_GetPCLK1Freq());
    printf("PCLK2_Frequency  = %d\n", (int) HAL_RCC_GetPCLK2Freq());

    return 0;
}

void wait_ms(unsigned long ms_time)
{
    HAL_Delay(ms_time);
}
