/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-09     xiangxistu   first version
 * 2020-05-18     chenyaxing   modify stm32_uart_config struct
 */

#include <string.h>
#include <stdlib.h>
#include <stm32f4xx.h>
#include "uart_config.h"
#include <board.h>

static UART_HandleTypeDef handle;

/* stm32 config class */
struct stm32_uart_config
{
    const char *name;
    USART_TypeDef *Instance;
    IRQn_Type irq_type;

    const char *tx_pin_name;
    const char *rx_pin_name;
};

static struct stm32_uart_config *_uart_config = NULL;

struct stm32_uart_config uart_config[] =
{
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
#ifdef BSP_USING_UART2
    UART2_CONFIG,
#endif
#ifdef BSP_USING_UART3
    UART3_CONFIG,
#endif
#ifdef BSP_USING_UART4
    UART4_CONFIG,
#endif
#ifdef BSP_USING_UART5
    UART5_CONFIG,
#endif
#ifdef BSP_USING_UART6
    UART6_CONFIG,
#endif
#ifdef BSP_USING_UART7
    UART7_CONFIG,
#endif
#ifdef BSP_USING_UART8
    UART8_CONFIG,
#endif
#ifdef BSP_USING_LPUART1
    LPUART1_CONFIG,
#endif
};

static long stm32_uart_clk_enable(struct stm32_uart_config *config)
{
    /* check the parameters */
    assert_param(IS_UART_INSTANCE(config->Instance));

    /* uart clock enable */
    switch ((uint32_t)config->Instance)
    {
#ifdef BSP_USING_UART1
    case (uint32_t)USART1:
        __HAL_RCC_USART1_CLK_ENABLE();
        break;
#endif /* BSP_USING_UART1 */
#ifdef BSP_USING_UART2
    case (uint32_t)USART2:
        __HAL_RCC_USART2_CLK_ENABLE();
        break;
#endif /* BSP_USING_UART2 */
#ifdef BSP_USING_UART3
    case (uint32_t)USART3:
        __HAL_RCC_USART3_CLK_ENABLE();
        break;
#endif /* BSP_USING_UART3 */
#ifdef BSP_USING_UART4
#if defined(SOC_SERIES_STM32F0) || defined(SOC_SERIES_STM32L0) || \
   defined(SOC_SERIES_STM32G0)
    case (uint32_t)USART4:
        __HAL_RCC_USART4_CLK_ENABLE();
#else
    case (uint32_t)UART4:
        __HAL_RCC_UART4_CLK_ENABLE();
#endif
        break;
#endif /* BSP_USING_UART4 */
#ifdef BSP_USING_UART5
#if defined(SOC_SERIES_STM32F0) || defined(SOC_SERIES_STM32L0) || \
   defined(SOC_SERIES_STM32G0)
    case (uint32_t)USART5:
        __HAL_RCC_USART5_CLK_ENABLE();
#else
    case (uint32_t)UART5:
        __HAL_RCC_UART5_CLK_ENABLE();
#endif
        break;
#endif /* BSP_USING_UART5 */
#ifdef BSP_USING_UART6
    case (uint32_t)USART6:
        __HAL_RCC_USART6_CLK_ENABLE();
        break;
#endif /* BSP_USING_UART6 */
#ifdef BSP_USING_UART7
#if defined(SOC_SERIES_STM32F0)
    case (uint32_t)USART7:
        __HAL_RCC_USART7_CLK_ENABLE();
#else
    case (uint32_t)UART7:
        __HAL_RCC_UART7_CLK_ENABLE();
#endif
        break;
#endif /* BSP_USING_UART7 */
#ifdef BSP_USING_UART8
#if defined(SOC_SERIES_STM32F0)
    case (uint32_t)USART8:
        __HAL_RCC_USART8_CLK_ENABLE();
#else
    case (uint32_t)UART8:
        __HAL_RCC_UART8_CLK_ENABLE();
#endif
        break;
#endif /* BSP_USING_UART8 */
#ifdef BSP_USING_LPUART1
    case (uint32_t)LPUART1:
       __HAL_RCC_LPUART1_CLK_ENABLE();
       break;
#endif /* BSP_USING_LPUART1 */
    default:
        return -1;
    }

    return 0;
}

static long stm32_gpio_clk_enable(GPIO_TypeDef *gpiox)
{
    /* check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(gpiox));

    /* gpio ports clock enable */
    switch ((uint32_t)gpiox)
    {
#if defined(__HAL_RCC_GPIOA_CLK_ENABLE)
    case (uint32_t)GPIOA:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOB_CLK_ENABLE)
    case (uint32_t)GPIOB:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOC_CLK_ENABLE)
    case (uint32_t)GPIOC:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOD_CLK_ENABLE)
    case (uint32_t)GPIOD:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOE_CLK_ENABLE)
    case (uint32_t)GPIOE:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOF_CLK_ENABLE)
    case (uint32_t)GPIOF:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOG_CLK_ENABLE)
    case (uint32_t)GPIOG:
        __HAL_RCC_GPIOG_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOH_CLK_ENABLE)
    case (uint32_t)GPIOH:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOI_CLK_ENABLE)
    case (uint32_t)GPIOI:
        __HAL_RCC_GPIOI_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOJ_CLK_ENABLE)
    case (uint32_t)GPIOJ:
        __HAL_RCC_GPIOJ_CLK_ENABLE();
        break;
#endif
#if defined(__HAL_RCC_GPIOK_CLK_ENABLE)
    case (uint32_t)GPIOK:
        __HAL_RCC_GPIOK_CLK_ENABLE();
        break;
#endif
    default:
        return -1;
    }

    return 0;
}

static int up_char(char * c)
{
    if ((*c >= 'a') && (*c <= 'z'))
    {
        *c = *c - 32;
    }
    return 0;
}

static void get_pin_by_name(const char* pin_name, GPIO_TypeDef **port, uint16_t *pin)
{
    int pin_num = atoi((char*) &pin_name[2]);
    char port_name = pin_name[1];
    up_char(&port_name);
    up_char(&port_name);
    *port = ((GPIO_TypeDef *) ((uint32_t) GPIOA
            + (uint32_t) (port_name - 'A') * ((uint32_t) GPIOB - (uint32_t) GPIOA)));
    *pin = (GPIO_PIN_0 << pin_num);
}

static long stm32_gpio_configure(struct stm32_uart_config *config)
{
    int uart_num = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_TypeDef *tx_port;
    GPIO_TypeDef *rx_port;
    uint16_t tx_pin;
    uint16_t rx_pin;
    uart_num = config->name[4] - '0';
    get_pin_by_name(config->rx_pin_name, &rx_port, &rx_pin);
    get_pin_by_name(config->tx_pin_name, &tx_port, &tx_pin);

    /* gpio ports clock enable */
    stm32_gpio_clk_enable(tx_port);
    if (tx_port != rx_port)
    {
        stm32_gpio_clk_enable(rx_port);
    }
    
    /* rx pin initialize */
    GPIO_InitStruct.Pin = tx_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(SOC_SERIES_STM32F2) || defined(SOC_SERIES_STM32F4) || \
    defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32G4) || \
    defined(SOC_SERIES_STM32L1) || defined(SOC_SERIES_STM32L4)
#define GPIO_AF7   ((uint8_t)0x07)
#define GPIO_AF8   ((uint8_t)0x08)
    /* uart1-3 -> AF7, uart4-8 -> AF8 */
    if (uart_num <= 3)
    {
        GPIO_InitStruct.Alternate = GPIO_AF7;
    }
    else
    {
        GPIO_InitStruct.Alternate = GPIO_AF8;
    }
#endif
    HAL_GPIO_Init(tx_port, &GPIO_InitStruct);

    /* rx pin initialize */
    GPIO_InitStruct.Pin = rx_pin;
    HAL_GPIO_Init(rx_port, &GPIO_InitStruct);

    return 0;
}

static long stm32_configure(struct stm32_uart_config *config)
{
    stm32_uart_clk_enable(config);

    handle.Instance          = config->Instance;
    handle.Init.BaudRate     = 115200;
    handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    handle.Init.Mode         = UART_MODE_TX_RX;
    handle.Init.OverSampling = UART_OVERSAMPLING_16;
    handle.Init.WordLength   = UART_WORDLENGTH_8B;
    handle.Init.StopBits     = UART_STOPBITS_1;
    handle.Init.Parity       = UART_PARITY_NONE;

    if (HAL_UART_Init(&handle) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

int rt_hw_usart_init(void)
{
    _uart_config = &uart_config[0];
    stm32_gpio_configure(_uart_config);
    stm32_configure(_uart_config);

    return 0;
}

void print_char(char c)
{
    HAL_UART_Transmit(&handle, (uint8_t *) (&c), 1, 1);
}
