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
#include <stm32f1xx.h>
#include "uart_config.h"
#include <board.h>

#define __STM32_PORT(port)  GPIO##port##_BASE
#define GET_PIN(PORTx,PIN) (long)((16 * ( ((long)__STM32_PORT(PORTx) - (long)GPIOA_BASE)/(0x0400UL) )) + PIN)

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

const uint8_t __lowest_bit_bitmap[] =
{
    /* 00 */ 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 10 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 20 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 30 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 40 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 50 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 60 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 70 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 80 */ 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 90 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* A0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* B0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* C0 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* D0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* E0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* F0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
int __rt_ffs(int value)
{
    if (value == 0) return 0;

    if (value & 0xff)
        return __lowest_bit_bitmap[value & 0xff] + 1;

    if (value & 0xff00)
        return __lowest_bit_bitmap[(value & 0xff00) >> 8] + 9;

    if (value & 0xff0000)
        return __lowest_bit_bitmap[(value & 0xff0000) >> 16] + 17;

    return __lowest_bit_bitmap[(value & 0xff000000) >> 24] + 25;
}

static uint16_t stm32_get_pin(GPIO_TypeDef *pin_port, uint32_t pin_num)
{
    return (uint16_t)((16 * (((long)pin_port - (long)GPIOA_BASE)/(0x0400UL))) + (__rt_ffs(pin_num) - 1));
}
static long stm32_gpio_configure(struct stm32_uart_config *config)
{
#define UART_IS_TX        (1U<<7)
#define UART_IS_RX        (0U)

    uint16_t tx_pin_num = 0, rx_pin_num = 0;
    int rx_index = 0, tx_index = 0, index = 0;
    int uart_num = 0;
    int uart_is_remap = 0;
    GPIO_TypeDef *tx_port;
    GPIO_TypeDef *rx_port;
    uint16_t tx_pin;
    uint16_t rx_pin;
    get_pin_by_name(config->rx_pin_name, &rx_port, &rx_pin);
    get_pin_by_name(config->tx_pin_name, &tx_port, &tx_pin);

    struct gpio_uart_remap {
        /* index get by GET_PIN */
        uint16_t pin_index;
        /* -1: not uart, 1: uart1, 2: uart2 ... */
        int8_t normal_uart;
        /* -1: not uart, 1: uart1, 2: uart2 ... */
        int8_t remap_uart;
    };

   static const struct gpio_uart_remap uart_remaps[] =
       {
           /* usart1 configure */
           { .pin_index = GET_PIN(A, 9),  .normal_uart =  1, .remap_uart = -1 },
           { .pin_index = GET_PIN(A, 10), .normal_uart =  1, .remap_uart = -1 },
           { .pin_index = GET_PIN(B, 6),  .normal_uart = -1, .remap_uart =  1 },
           { .pin_index = GET_PIN(B, 7),  .normal_uart = -1, .remap_uart =  1 },

           /* usart2 configure */
           { .pin_index = GET_PIN(A, 2),  .normal_uart =  2, .remap_uart = -1 },
           { .pin_index = GET_PIN(A, 3),  .normal_uart =  2, .remap_uart = -1 },
           { .pin_index = GET_PIN(D, 5),  .normal_uart = -1, .remap_uart =  2 },
           { .pin_index = GET_PIN(D, 6),  .normal_uart = -1, .remap_uart =  2 },

           /* usart3 configure */
           { .pin_index = GET_PIN(B, 10), .normal_uart =  3, .remap_uart = -1 },
           { .pin_index = GET_PIN(B, 11), .normal_uart =  3, .remap_uart = -1 },
           { .pin_index = GET_PIN(D, 8),  .normal_uart = -1, .remap_uart =  3 },
           { .pin_index = GET_PIN(D, 9),  .normal_uart = -1, .remap_uart =  3 },
           { .pin_index = GET_PIN(C, 10), .normal_uart =  4, .remap_uart =  3 },
           { .pin_index = GET_PIN(C, 11), .normal_uart =  4, .remap_uart =  3 },
       };
   /* get tx/rx pin index */
   tx_pin_num = stm32_get_pin(tx_port, tx_pin);
   rx_pin_num = stm32_get_pin(rx_port, rx_pin);

   for (index = 0; index < sizeof(uart_remaps) / sizeof(struct gpio_uart_remap); index++)
   {
       if (uart_remaps[index].pin_index == tx_pin_num)
       {
           tx_index = index;
       }
       else if (uart_remaps[index].pin_index == rx_pin_num)
       {
           rx_index = index;
       }
   }

   /* check tx/rx pin remap information */
   assert_param(uart_remaps[tx_index].normal_uart == uart_remaps[rx_index].normal_uart);
   assert_param(uart_remaps[tx_index].remap_uart == uart_remaps[rx_index].remap_uart);

   uart_num = config->name[4] - '0';
   uart_is_remap = uart_remaps[tx_index].remap_uart == uart_num ? 1 : 0;

   {

       GPIO_InitTypeDef GPIO_InitStruct = {0};

       /* gpio ports clock enable */
       stm32_gpio_clk_enable(tx_port);
       if (tx_port != rx_port)
       {
           stm32_gpio_clk_enable(rx_port);
       }

       /* tx pin initialize */
       GPIO_InitStruct.Pin = tx_pin;
       GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
       GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
       HAL_GPIO_Init(tx_port, &GPIO_InitStruct);

       /* rx pin initialize */
       GPIO_InitStruct.Pin = rx_pin;
       GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
       GPIO_InitStruct.Pull = GPIO_NOPULL;
       HAL_GPIO_Init(rx_port, &GPIO_InitStruct);

       /* enable the remapping of usart alternate */
       if (uart_is_remap)
       {
           __HAL_RCC_AFIO_CLK_ENABLE();
           switch (uart_num)
           {
           case 1:
               __HAL_AFIO_REMAP_USART1_ENABLE();
               break;
           case 2:
               __HAL_AFIO_REMAP_USART2_ENABLE();
               break;
            case 3:
                if (uart_remaps[tx_index].normal_uart < 0)
                {
#ifdef AFIO_MAPR_USART3_REMAP_FULLREMAP
                    __HAL_AFIO_REMAP_USART3_ENABLE();
#endif
                }
                else
                {
#ifdef AFIO_MAPR_USART3_REMAP_PARTIALREMAP
                    __HAL_AFIO_REMAP_USART3_PARTIAL();
#endif
                    }
                break;
           default:
               assert_param(0);
           }
       }
   }

   return 0;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    /* if this uart is shell function */
    if(huart == &handle)
    {
        stm32_gpio_configure(_uart_config);
    }
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
    stm32_configure(_uart_config);

    return 0;
}


void print_char(char c)
{
    HAL_UART_Transmit(&handle, (uint8_t *) (&c), 1, 1);
}
