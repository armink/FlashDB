/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-30     SummerGift   first version
 * 2019-01-03     zylx         modify dma support
 * 2020-05-18     chenyaxing   modify uart_config struct
 */

#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__
#include <board.h>
#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_USING_UART1)
#ifndef UART1_CONFIG
#define UART1_CONFIG                                                \
    {                                                               \
        .name = "uart1",                                            \
        .Instance = USART1,                                         \
        .irq_type = USART1_IRQn,                                    \
        .tx_pin_name = BSP_UART1_TX_PIN,                            \
        .rx_pin_name = BSP_UART1_RX_PIN,                            \
    }
#endif /* UART1_CONFIG */

#if defined(BSP_UART1_RX_USING_DMA)
#ifndef UART1_DMA_RX_CONFIG
#define UART1_DMA_RX_CONFIG                                         \
    {                                                               \
        .Instance = UART1_RX_DMA_INSTANCE,                          \
        .dma_rcc = UART1_RX_DMA_RCC,                                \
        .dma_irq = UART1_RX_DMA_IRQ,                                \
    }
#endif /* UART1_DMA_RX_CONFIG */
#endif /* BSP_UART1_RX_USING_DMA */

#if defined(BSP_UART1_TX_USING_DMA)
#ifndef UART1_DMA_TX_CONFIG
#define UART1_DMA_TX_CONFIG                                         \
    {                                                               \
        .Instance = UART1_TX_DMA_INSTANCE,                          \
        .dma_rcc = UART1_TX_DMA_RCC,                                \
        .dma_irq = UART1_TX_DMA_IRQ,                                \
    }
#endif /* UART1_DMA_TX_CONFIG */
#endif /* BSP_UART1_TX_USING_DMA */
#endif /* BSP_USING_UART1 */

#if defined(BSP_USING_UART2)
#ifndef UART2_CONFIG
#define UART2_CONFIG                                                \
    {                                                               \
        .name = "uart2",                                            \
        .Instance = USART2,                                         \
        .irq_type = USART2_IRQn,                                    \
        .tx_pin_name = BSP_UART2_TX_PIN,                            \
        .rx_pin_name = BSP_UART2_RX_PIN,                            \
    }
#endif /* UART2_CONFIG */

#if defined(BSP_UART2_RX_USING_DMA)
#ifndef UART2_DMA_RX_CONFIG
#define UART2_DMA_RX_CONFIG                                         \
    {                                                               \
        .Instance = UART2_RX_DMA_INSTANCE,                          \
        .dma_rcc = UART2_RX_DMA_RCC,                                \
        .dma_irq = UART2_RX_DMA_IRQ,                                \
    }
#endif /* UART2_DMA_RX_CONFIG */
#endif /* BSP_UART2_RX_USING_DMA */

#if defined(BSP_UART2_TX_USING_DMA)
#ifndef UART2_DMA_TX_CONFIG
#define UART2_DMA_TX_CONFIG                                         \
    {                                                               \
        .Instance = UART2_TX_DMA_INSTANCE,                          \
        .dma_rcc = UART2_TX_DMA_RCC,                                \
        .dma_irq = UART2_TX_DMA_IRQ,                                \
    }
#endif /* UART2_DMA_TX_CONFIG */
#endif /* BSP_UART2_TX_USING_DMA */
#endif /* BSP_USING_UART2 */

#if defined(BSP_USING_UART3)
#ifndef UART3_CONFIG
#define UART3_CONFIG                                                \
    {                                                               \
        .name = "uart3",                                            \
        .Instance = USART3,                                         \
        .irq_type = USART3_IRQn,                                    \
        .tx_pin_name = BSP_UART3_TX_PIN,                            \
        .rx_pin_name = BSP_UART3_RX_PIN,                            \
    }
#endif /* UART3_CONFIG */

#if defined(BSP_UART3_RX_USING_DMA)
#ifndef UART3_DMA_RX_CONFIG
#define UART3_DMA_RX_CONFIG                                         \
    {                                                               \
        .Instance = UART3_RX_DMA_INSTANCE,                          \
        .dma_rcc = UART3_RX_DMA_RCC,                                \
        .dma_irq = UART3_RX_DMA_IRQ,                                \
    }
#endif /* UART3_DMA_RX_CONFIG */
#endif /* BSP_UART3_RX_USING_DMA */

#if defined(BSP_UART3_TX_USING_DMA)
#ifndef UART3_DMA_TX_CONFIG
#define UART3_DMA_TX_CONFIG                                         \
    {                                                               \
        .Instance = UART3_TX_DMA_INSTANCE,                          \
        .dma_rcc = UART3_TX_DMA_RCC,                                \
        .dma_irq = UART3_TX_DMA_IRQ,                                \
    }
#endif /* UART3_DMA_TX_CONFIG */
#endif /* BSP_UART3_TX_USING_DMA */
#endif /* BSP_USING_UART3 */

#if defined(BSP_USING_UART4)
#ifndef UART4_CONFIG
#define UART4_CONFIG                                                \
    {                                                               \
        .name = "uart4",                                            \
        .Instance = UART4,                                          \
        .irq_type = UART4_IRQn,                                     \
        .tx_pin_name = BSP_UART4_TX_PIN,                            \
        .rx_pin_name = BSP_UART4_RX_PIN,                            \
    }
#endif /* UART4_CONFIG */

#if defined(BSP_UART4_RX_USING_DMA)
#ifndef UART4_DMA_RX_CONFIG
#define UART4_DMA_RX_CONFIG                                         \
    {                                                               \
        .Instance = UART4_RX_DMA_INSTANCE,                          \
        .dma_rcc = UART4_RX_DMA_RCC,                                \
        .dma_irq = UART4_RX_DMA_IRQ,                                \
    }
#endif /* UART4_DMA_RX_CONFIG */
#endif /* BSP_UART4_RX_USING_DMA */

#if defined(BSP_UART4_TX_USING_DMA)
#ifndef UART4_DMA_TX_CONFIG
#define UART4_DMA_TX_CONFIG                                         \
    {                                                               \
        .Instance = UART4_TX_DMA_INSTANCE,                          \
        .dma_rcc = UART4_TX_DMA_RCC,                                \
        .dma_irq = UART4_TX_DMA_IRQ,                                \
    }
#endif /* UART4_DMA_TX_CONFIG */
#endif /* BSP_UART4_RX_USING_DMA */
#endif /* BSP_USING_UART4 */

#if defined(BSP_USING_UART5)
#ifndef UART5_CONFIG
#define UART5_CONFIG                                                \
    {                                                               \
        .name = "uart5",                                            \
        .Instance = UART5,                                          \
        .irq_type = UART5_IRQn,                                     \
        .tx_pin_name = BSP_UART5_TX_PIN,                            \
        .rx_pin_name = BSP_UART5_RX_PIN,                            \
    }
#endif /* UART5_CONFIG */

#if defined(BSP_UART5_RX_USING_DMA)
#ifndef UART5_DMA_RX_CONFIG
#define UART5_DMA_RX_CONFIG                                         \
    {                                                               \
        .Instance = UART5_RX_DMA_INSTANCE,                          \
        .dma_rcc = UART5_RX_DMA_RCC,                                \
        .dma_irq = UART5_RX_DMA_IRQ,                                \
    }
#endif /* UART5_DMA_RX_CONFIG */
#endif /* BSP_UART5_RX_USING_DMA */

#if defined(BSP_UART5_TX_USING_DMA)
#ifndef UART5_DMA_TX_CONFIG
#define UART5_DMA_TX_CONFIG                                         \
    {                                                               \
        .Instance = UART5_TX_DMA_INSTANCE,                          \
        .dma_rcc = UART5_TX_DMA_RCC,                                \
        .dma_irq = UART5_TX_DMA_IRQ,                                \
    }
#endif /* UART5_DMA_TX_CONFIG */
#endif /* BSP_UART5_TX_USING_DMA */
#endif /* BSP_USING_UART5 */

#ifdef __cplusplus
}
#endif

#endif
