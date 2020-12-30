/*******************************************************************************
 *   Filename:       bsp_uart_fifo.h
 *   Revised:        All copyrights reserved to Roger-WY.
 *   Date:           2016-07-11
 *   Revision:       v1.0
 *   Writer:	     Roger-WY.
 *
 *   Description:    采用串口中断+FIFO模式实现多个串口的同时访问   头文件
 *******************************************************************************/

#ifndef __BSP_USART_FIFO_H__
#define __BSP_USART_FIFO_H__

#include <stm32f10x.h>
/*
	如果需要更改串口对应的管脚，请自行修改 bsp_uart_fifo.c文件中的 static void InitHardUart(void)函数
*/

/* 定义使能的串口, 0 表示不使能（不增加代码大小）， 1表示使能 */
#define	UART1_FIFO_EN	1
#define	UART2_FIFO_EN	0
#define	UART3_FIFO_EN	0
#define	UART4_FIFO_EN	0
#define	UART5_FIFO_EN	0

//#define BSP_RS485_TX_FUNCTOIN_ENABLE     /* RS485发送接收时是否使用单独的IO控制 */
#ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
    /* RS485芯片发送使能GPIO, PB2 */
    #define RCC_RS485_TXEN 	 RCC_APB2Periph_GPIOB
    #define PORT_RS485_TXEN  GPIOB
    #define PIN_RS485_TXEN	 GPIO_Pin_2

    #define RS485_RX_EN()	PORT_RS485_TXEN->BRR  = PIN_RS485_TXEN
    #define RS485_TX_EN()	PORT_RS485_TXEN->BSRR = PIN_RS485_TXEN
#endif

/* 定义端口号 */
typedef enum
{
	COM1 = 0,	/* USART1  PA9, PA10 */
	COM2 = 1,	/* USART2, PA2, PA3 */
	COM3 = 2,	/* USART3, PB10, PB11 */
	COM4 = 3,	/* UART4, PC10, PC11 */
	COM5 = 4,	/* UART5, PC12, PD2 */
}COM_PORT_E;

/* 定义串口波特率和FIFO缓冲区大小，分为发送缓冲区和接收缓冲区, 支持全双工 */
#if UART1_FIFO_EN == 1
	#define UART1_BAUD			115200
	#define UART1_TX_BUF_SIZE	1*512
	#define UART1_RX_BUF_SIZE	1*128
#endif

#if UART2_FIFO_EN == 1
	#define UART2_BAUD			9600
	#define UART2_TX_BUF_SIZE	1*64
	#define UART2_RX_BUF_SIZE	1*64
#endif

#if UART3_FIFO_EN == 1
	#define UART3_BAUD			115200
	#define UART3_TX_BUF_SIZE	1*128
	#define UART3_RX_BUF_SIZE	1*128
#endif

#if UART4_FIFO_EN == 1
	#define UART4_BAUD			9600
	#define UART4_TX_BUF_SIZE	1*64
	#define UART4_RX_BUF_SIZE	1*64
#endif

#if UART5_FIFO_EN == 1
	#define UART5_BAUD			115200
	#define UART5_TX_BUF_SIZE	1*1024
	#define UART5_RX_BUF_SIZE	1*1024
#endif

/* 串口设备结构体 */
typedef struct
{
	USART_TypeDef *uart;		/* STM32内部串口设备指针 */
	uint8_t *pTxBuf;			/* 发送缓冲区 */
	uint8_t *pRxBuf;			/* 接收缓冲区 */
	uint16_t usTxBufSize;		/* 发送缓冲区大小 */
	uint16_t usRxBufSize;		/* 接收缓冲区大小 */
	__IO uint16_t usTxWrite;	/* 发送缓冲区写指针 */
	__IO uint16_t usTxRead;		/* 发送缓冲区读指针 */
	__IO uint16_t usTxCount;	/* 等待发送的数据个数 */

	__IO uint16_t usRxWrite;	/* 接收缓冲区写指针 */
	__IO uint16_t usRxRead;		/* 接收缓冲区读指针 */
	__IO uint16_t usRxCount;	/* 还未读取的新数据个数 */

	void (*SendBefor)(void); 	/* 开始发送之前的回调函数指针（主要用于RS485切换到发送模式） */
	void (*SendOver)(void); 	/* 发送完毕的回调函数指针（主要用于RS485将发送模式切换为接收模式） */
	void (*ReciveNew)(uint8_t _byte);	/* 串口收到数据的回调函数指针 */
}UART_T;

void bsp_UartInit(void);
void ComSendBuf(COM_PORT_E _ucPort, uint8_t *_ucaBuf, uint16_t _usLen);
void ComSendChar(COM_PORT_E _ucPort, uint8_t _ucByte);
uint8_t ComGetChar(COM_PORT_E _ucPort, uint8_t *_pByte);
uint16_t ComGetString(COM_PORT_E _ucPort, uint8_t *_pBuf);


void ComClearTxFifo(COM_PORT_E _ucPort);
void ComClearRxFifo(COM_PORT_E _ucPort);

void RS485_SendBuf(uint8_t *_ucaBuf, uint16_t _usLen);
void RS485_SendStr(char *_pBuf);

void bsp_Set485Baud(uint32_t _baud);
void bsp_SetUart1Baud(uint32_t _baud);
void bsp_SetUart2Baud(uint32_t _baud);

#endif

/*****************************  (END OF FILE) *********************************/
