/*******************************************************************************
 *   Filename:       bsp_uart_fifo.c
 *   Revised:        All copyrights reserved to Roger-WY.
 *   Date:           2016-07-11
 *   Revision:       v1.0
 *   Writer:	     Roger-WY.
 *
 *   Description:    采用串口中断+FIFO模式实现多个串口的同时访问
 *
 *
 *   Notes:
 *
 *******************************************************************************/

#include "bsp_uart_fifo.h"
#include "bsp.h"
#include <string.h>


/* 定义每个串口结构体变量 */
#if UART1_FIFO_EN == 1
	static UART_T  g_tUart1;
	static uint8_t g_TxBuf1[UART1_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf1[UART1_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if UART2_FIFO_EN == 1
	static UART_T  g_tUart2;
	static uint8_t g_TxBuf2[UART2_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf2[UART2_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if UART3_FIFO_EN == 1
	static UART_T  g_tUart3;
	static uint8_t g_TxBuf3[UART3_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf3[UART3_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if UART4_FIFO_EN == 1
	static UART_T  g_tUart4;
	static uint8_t g_TxBuf4[UART4_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf4[UART4_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if UART5_FIFO_EN == 1
	static UART_T  g_tUart5;
	static uint8_t g_TxBuf5[UART5_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf5[UART5_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

//----------------------------------------------------------------------------//
static void UartVarInit(void);

static void InitHardUart(void);
static void UartSend(UART_T *_pUart, uint8_t *_ucaBuf, uint16_t _usLen);
static uint8_t UartGetChar(UART_T *_pUart, uint8_t *_pByte);
static void UartIRQ(UART_T *_pUart);
static void ConfigUartNVIC(void);

#ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
void RS485_InitTXE(void);
#endif

//============================================================================//

/*******************************************************************************
 * 名    称： bsp_UartInit
 * 功    能： 初始化串口硬件，并对全局变量赋初值.
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void bsp_UartInit(void)
{
	UartVarInit();		/* 必须先初始化全局变量,再配置硬件 */

	InitHardUart();		/* 配置串口的硬件参数(波特率等) */

#ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
	RS485_InitTXE();	/* 配置RS485芯片的发送使能硬件，配置为推挽输出 */
#endif

	ConfigUartNVIC();	/* 配置串口中断 */
}

/*******************************************************************************
 * 名    称： ComToUart
 * 功    能： 将COM端口号转换为UART指针
 * 入口参数： UART端口号
 * 出口参数： uart指针
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
UART_T *ComToUart(COM_PORT_E _ucPort)
{
	if (_ucPort == COM1) {
		#if UART1_FIFO_EN == 1
			return &g_tUart1;
		#else
			return 0;
		#endif
	} else if (_ucPort == COM2) {
		#if UART2_FIFO_EN == 1
			return &g_tUart2;
		#else
			return 0;
		#endif
	} else if (_ucPort == COM3) {
		#if UART3_FIFO_EN == 1
			return &g_tUart3;
		#else
			return 0;
		#endif
	} else if (_ucPort == COM4) {
		#if UART4_FIFO_EN == 1
			return &g_tUart4;
		#else
			return 0;
		#endif
	} else if (_ucPort == COM5) {
		#if UART5_FIFO_EN == 1
			return &g_tUart5;
		#else
			return 0;
		#endif
	} else {
		/* 不做任何处理 */
		return 0;
	}
}

/*******************************************************************************
 * 名    称： ComToUart
 * 功    能： 将COM端口号转换为UART指针
 * 入口参数： _ucPort: 端口号(COM1 - COM5)
 * 出口参数： uart指针 USART1, USART2, USART3, UART4, UART5
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void ComSendBuf(COM_PORT_E _ucPort, uint8_t *_ucaBuf, uint16_t _usLen)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0) {
		return;
	}

	if (pUart->SendBefor != 0) {
		pUart->SendBefor();		/* 如果是RS485通信，可以在这个函数中将RS485设置为发送模式 */
	}

	UartSend(pUart, _ucaBuf, _usLen);
}

/*******************************************************************************
 * 名    称： ComSendChar
 * 功    能： 向串口发送1个字节。数据放到发送缓冲区后立即返回，由中断服务程序在后台完成发送
 * 入口参数： _ucPort: 端口号(COM1 - COM5)
 *			  _ucByte: 待发送的数据
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void ComSendChar(COM_PORT_E _ucPort, uint8_t _ucByte)
{
	ComSendBuf(_ucPort, &_ucByte, 1);
}

/*******************************************************************************
 * 名    称： ComGetChar
 * 功    能： 从接收缓冲区读取1字节，非阻塞。无论有无数据均立即返回。
 * 入口参数： _ucPort: 端口号(COM1 - COM5)
 *			  _pByte: 接收到的数据存放在这个地址
 * 出口参数： 0 表示无数据, 1 表示读取到有效字节
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
uint8_t ComGetChar(COM_PORT_E _ucPort, uint8_t *_pByte)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0) {
		return 0;
	}

	return UartGetChar(pUart, _pByte);
}

uint16_t ComGetString(COM_PORT_E _ucPort, uint8_t *_pBuf)
{
    uint16_t  ucCnt = 0;

    while(ComGetChar(_ucPort,_pBuf)) {
            _pBuf ++;
            ucCnt ++;
    }

    return ucCnt;
}

/*******************************************************************************
 * 名    称： ComClearTxFifo
 * 功    能： 清零串口发送缓冲区
 * 入口参数： _ucPort: 端口号(COM1 - COM5)
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void ComClearTxFifo(COM_PORT_E _ucPort)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0) {
		return;
	}

	pUart->usTxWrite = 0;
	pUart->usTxRead  = 0;
	pUart->usTxCount = 0;
}

/*******************************************************************************
 * 名    称： ComClearRxFifo
 * 功    能： 清零串口接收缓冲区
 * 入口参数： _ucPort: 端口号(COM1 - COM5)
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void ComClearRxFifo(COM_PORT_E _ucPort)
{
	UART_T *pUart;

	pUart = ComToUart(_ucPort);
	if (pUart == 0) {
		return;
	}

	pUart->usRxWrite = 0;
	pUart->usRxRead  = 0;
	pUart->usRxCount = 0;
}

/*******************************************************************************
 * 名    称： bsp_SetUart1Baud
 * 功    能： 修改UART1波特率
 * 入口参数： _baud : 波特率.0-4500000
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void bsp_SetUart1Baud(uint32_t _baud)
{
	USART_InitTypeDef USART_InitStructure;

	/* 第2步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate   = _baud;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits   = USART_StopBits_1;
	USART_InitStructure.USART_Parity     = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
}

/*******************************************************************************
 * 名    称： bsp_SetUart2Baud
 * 功    能： 修改UART2波特率
 * 入口参数： _baud : 波特率.0-4500000
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void bsp_SetUart2Baud(uint32_t _baud)
{
	USART_InitTypeDef USART_InitStructure;

	/* 第2步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate = _baud;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
}


/* 如果是RS485通信，请按如下格式编写函数， 我们仅举了 USART3作为RS485的例子 */
#ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
/*******************************************************************************
 * 名    称： RS485_InitTXE
 * 功    能： 配置RS485发送使能口线 TXE
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：如果是RS485通信，请按如下格式编写函数， 我们仅举了 USART3作为RS485的例子
 *******************************************************************************/
void RS485_InitTXE(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_RS485_TXEN, ENABLE);	/* 打开GPIO时钟 */

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	/* 推挽输出模式 */
	GPIO_InitStructure.GPIO_Pin = PIN_RS485_TXEN;
	GPIO_Init(PORT_RS485_TXEN, &GPIO_InitStructure);
}
/*******************************************************************************
 * 名    称： RS485_SendBefor
 * 功    能： 发送数据前的准备工作。对于RS485通信，请设置RS485芯片为发送状态，
 *			  并修改 UartVarInit()中的函数指针等于本函数名，比如 g_tUart2.SendBefor = RS485_SendBefor
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void RS485_SendBefor(void)
{
	RS485_TX_EN();	/* 切换RS485收发芯片为发送模式 */
}

/*******************************************************************************
 * 名    称： RS485_SendOver
 * 功    能： 发送一串数据结束后的善后处理。对于RS485通信，请设置RS485芯片为接收状态，
*			  并修改 UartVarInit()中的函数指针等于本函数名，比如 g_tUart2.SendOver = RS485_SendOver
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void RS485_SendOver(void)
{
	RS485_RX_EN();	/* 切换RS485收发芯片为接收模式 */
}

#endif

/*******************************************************************************
 * 名    称： bsp_Set485Baud
 * 功    能： 修改485串口(USART3)的波特率。
 * 入口参数： _baud : 波特率.0-4500000
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void bsp_Set485Baud(uint32_t _baud)
{
	USART_InitTypeDef USART_InitStructure;

	/* 第2步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate   = _baud;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits   = USART_StopBits_1;
	USART_InitStructure.USART_Parity     = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
}



/*******************************************************************************
 * 名    称： RS485_SendBuf
 * 功    能： 通过RS485芯片发送一串数据。注意，本函数不等待发送完毕。
 * 入口参数： _ucaBuf : 数据缓冲区
 *			  _usLen : 数据长度
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void RS485_SendBuf(uint8_t *_ucaBuf, uint16_t _usLen)
{
	ComSendBuf(COM1, _ucaBuf, _usLen);
}

/*******************************************************************************
 * 名    称： RS485_SendStr
 * 功    能： 通过RS485芯片发送一串字符串
 * 入口参数： _ucaBuf : 数据缓冲区
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
void RS485_SendStr(char *_pBuf)
{
	RS485_SendBuf((uint8_t *)_pBuf, strlen(_pBuf));
}

/*******************************************************************************
 * 名    称： ModbusSlave_Uart_ReciveNew
 * 功    能： Modbus从机接收到新的数据
 * 入口参数： _byte 接收到的新数据
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
#if  (MODBUS_CFG_SLAVE_EN == 1)
extern void MODS_ReciveNew(uint8_t _byte);
void ModbusSlave_Uart_ReciveNew(uint8_t _byte)
{
	MODS_ReciveNew(_byte);
}
#endif
/*******************************************************************************
 * 名    称： ModbusHost_Uart_ReciveNew
 * 功    能： Modbus主机接收到新的数据
 * 入口参数： _byte 接收到的新数据
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
#if  (MODBUS_CFG_HOST_EN == 1)
extern void MODH_ReciveNew(uint8_t _data);
void ModbusHost_Uart_ReciveNew(uint8_t _byte)
{
    MODH_ReciveNew(_byte);
}
#endif

/*******************************************************************************
 * 名    称： UartVarInit
 * 功    能： 初始化串口相关的变量
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
static void UartVarInit(void)
{
#if UART1_FIFO_EN == 1
	g_tUart1.uart        = USART1;				/* STM32 串口设备 */
	g_tUart1.pTxBuf      = g_TxBuf1;			/* 发送缓冲区指针 */
	g_tUart1.pRxBuf      = g_RxBuf1;			/* 接收缓冲区指针 */
	g_tUart1.usTxBufSize = UART1_TX_BUF_SIZE;	/* 发送缓冲区大小 */
	g_tUart1.usRxBufSize = UART1_RX_BUF_SIZE;	/* 接收缓冲区大小 */
	g_tUart1.usTxWrite = 0;						/* 发送FIFO写索引 */
	g_tUart1.usTxRead  = 0;						/* 发送FIFO读索引 */
	g_tUart1.usRxWrite = 0;						/* 接收FIFO写索引 */
	g_tUart1.usRxRead  = 0;						/* 接收FIFO读索引 */
	g_tUart1.usRxCount = 0;						/* 接收到的新数据个数 */
	g_tUart1.usTxCount = 0;						/* 待发送的数据个数 */
    #ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
	g_tUart1.SendBefor = RS485_SendBefor;		/* 发送数据前的回调函数 */
	g_tUart1.SendOver  = RS485_SendOver;		/* 发送完毕后的回调函数 */
    #else
    g_tUart1.SendBefor = 0;		                /* 发送数据前的回调函数 */
	g_tUart1.SendOver  = 0;		                /* 发送完毕后的回调函数 */
    #endif
    g_tUart1.ReciveNew = 0;		/* 接收到新数据后的回调函数 */
#endif

#if UART2_FIFO_EN == 1
	g_tUart2.uart        = USART2;				/* STM32 串口设备 */
	g_tUart2.pTxBuf      = g_TxBuf2;			/* 发送缓冲区指针 */
	g_tUart2.pRxBuf      = g_RxBuf2;			/* 接收缓冲区指针 */
	g_tUart2.usTxBufSize = UART2_TX_BUF_SIZE;	/* 发送缓冲区大小 */
	g_tUart2.usRxBufSize = UART2_RX_BUF_SIZE;	/* 接收缓冲区大小 */
	g_tUart2.usTxWrite = 0;						/* 发送FIFO写索引 */
	g_tUart2.usTxRead  = 0;						/* 发送FIFO读索引 */
	g_tUart2.usRxWrite = 0;						/* 接收FIFO写索引 */
	g_tUart2.usRxRead  = 0;						/* 接收FIFO读索引 */
	g_tUart2.usRxCount = 0;						/* 接收到的新数据个数 */
	g_tUart2.usTxCount = 0;						/* 待发送的数据个数 */
    #ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
	g_tUart2.SendBefor = RS485_SendBefor;		/* 发送数据前的回调函数 */
	g_tUart2.SendOver  = RS485_SendOver;		/* 发送完毕后的回调函数 */
    #else
    g_tUart2.SendBefor = 0;		                /* 发送数据前的回调函数 */
	g_tUart2.SendOver  = 0;		                /* 发送完毕后的回调函数 */
    #endif
    //g_tUart2.ReciveNew = 0;		/* 接收到新数据后的回调函数 */
    g_tUart2.ReciveNew = ModbusHost_Uart_ReciveNew;
#endif

#if UART3_FIFO_EN == 1
	g_tUart3.uart        = USART3;				/* STM32 串口设备 */
	g_tUart3.pTxBuf      = g_TxBuf3;			/* 发送缓冲区指针 */
	g_tUart3.pRxBuf      = g_RxBuf3;			/* 接收缓冲区指针 */
	g_tUart3.usTxBufSize = UART3_TX_BUF_SIZE;	/* 发送缓冲区大小 */
	g_tUart3.usRxBufSize = UART3_RX_BUF_SIZE;	/* 接收缓冲区大小 */
	g_tUart3.usTxWrite = 0;						/* 发送FIFO写索引 */
	g_tUart3.usTxRead  = 0;						/* 发送FIFO读索引 */
	g_tUart3.usRxWrite = 0;						/* 接收FIFO写索引 */
	g_tUart3.usRxRead  = 0;						/* 接收FIFO读索引 */
	g_tUart3.usRxCount = 0;						/* 接收到的新数据个数 */
	g_tUart3.usTxCount = 0;						/* 待发送的数据个数 */
    #ifdef  BSP_RS485_TX_FUNCTOIN_ENABLE
	g_tUart3.SendBefor = RS485_SendBefor;		/* 发送数据前的回调函数 */
	g_tUart3.SendOver  = RS485_SendOver;		/* 发送完毕后的回调函数 */
    #else
    g_tUart3.SendBefor = 0;		                /* 发送数据前的回调函数 */
	g_tUart3.SendOver  = 0;		                /* 发送完毕后的回调函数 */
    #endif
	//g_tUart3.ReciveNew = RS485_ReciveNew;		/* 接收到新数据后的回调函数 */
    //g_tUart3.ReciveNew = ModbusHost_Uart_ReciveNew;
    g_tUart3.ReciveNew = 0;		/* 接收到新数据后的回调函数 */
#endif

#if UART4_FIFO_EN == 1
	g_tUart4.uart        = UART4;				/* STM32 串口设备 */
	g_tUart4.pTxBuf      = g_TxBuf4;			/* 发送缓冲区指针 */
	g_tUart4.pRxBuf      = g_RxBuf4;			/* 接收缓冲区指针 */
	g_tUart4.usTxBufSize = UART4_TX_BUF_SIZE;	/* 发送缓冲区大小 */
	g_tUart4.usRxBufSize = UART4_RX_BUF_SIZE;	/* 接收缓冲区大小 */
	g_tUart4.usTxWrite = 0;						/* 发送FIFO写索引 */
	g_tUart4.usTxRead  = 0;						/* 发送FIFO读索引 */
	g_tUart4.usRxWrite = 0;						/* 接收FIFO写索引 */
	g_tUart4.usRxRead  = 0;						/* 接收FIFO读索引 */
	g_tUart4.usRxCount = 0;						/* 接收到的新数据个数 */
	g_tUart4.usTxCount = 0;						/* 待发送的数据个数 */
	g_tUart4.SendBefor = 0;						/* 发送数据前的回调函数 */
	g_tUart4.SendOver  = 0;						/* 发送完毕后的回调函数 */
	g_tUart4.ReciveNew = 0;						/* 接收到新数据后的回调函数 */
#endif

#if UART5_FIFO_EN == 1
	g_tUart5.uart        = UART5;				/* STM32 串口设备 */
	g_tUart5.pTxBuf      = g_TxBuf5;			/* 发送缓冲区指针 */
	g_tUart5.pRxBuf      = g_RxBuf5;			/* 接收缓冲区指针 */
	g_tUart5.usTxBufSize = UART5_TX_BUF_SIZE;	/* 发送缓冲区大小 */
	g_tUart5.usRxBufSize = UART5_RX_BUF_SIZE;	/* 接收缓冲区大小 */
	g_tUart5.usTxWrite = 0;						/* 发送FIFO写索引 */
	g_tUart5.usTxRead  = 0;						/* 发送FIFO读索引 */
	g_tUart5.usRxWrite = 0;						/* 接收FIFO写索引 */
	g_tUart5.usRxRead  = 0;						/* 接收FIFO读索引 */
	g_tUart5.usRxCount = 0;						/* 接收到的新数据个数 */
	g_tUart5.usTxCount = 0;						/* 待发送的数据个数 */
	g_tUart5.SendBefor = 0;						/* 发送数据前的回调函数 */
	g_tUart5.SendOver  = 0;						/* 发送完毕后的回调函数 */
	g_tUart5.ReciveNew = 0;						/* 接收到新数据后的回调函数 */
#endif

}

/*******************************************************************************
 * 名    称： InitHardUart
 * 功    能： 配置串口的硬件参数（波特率，数据位，停止位，起始位，校验位，中断使能）
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
static void InitHardUart(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

#if UART1_FIFO_EN == 1		/* 串口1 TX = PA9   RX = PA10 或 TX = PB6   RX = PB7*/

	/* 第1步：打开GPIO和USART部件的时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* 第2步：将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第3步：将USART Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第4步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate = UART1_BAUD;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	/* 使能接收中断 */
	/*
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
		注意: 不要在此处打开发送中断
		发送中断使能在SendUart()函数打开
	*/
	USART_Cmd(USART1, ENABLE);		/* 使能串口 */

	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(USART1, USART_FLAG_TC);     /* 清发送完成标志，Transmission Complete flag */
#endif

#if UART2_FIFO_EN == 1		/* 串口2 TX = PA2， RX = PA3  */
	/* 第1步：打开GPIO和USART部件的时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* 第2步：将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第3步：将USART Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/*  第3步已经做了，因此这步可以不做
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	*/
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第4步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate   = UART2_BAUD;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits   = USART_StopBits_1;
	USART_InitStructure.USART_Parity     = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;		/* 仅选择接收模式 */
	USART_Init(USART2, &USART_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);	/* 使能接收中断 */
	/*
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
		注意: 不要在此处打开发送中断
		发送中断使能在SendUart()函数打开
	*/
	USART_Cmd(USART2, ENABLE);		/* 使能串口 */

	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(USART2, USART_FLAG_TC);     /* 清发送完成标志，Transmission Complete flag */
#endif

#if UART3_FIFO_EN == 1			/* 串口3 TX = PB10   RX = PB11 */
	/* 第1步： 开启GPIO和UART时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	/* 第2步：将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 第3步：将USART Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/*  第3步已经做了，因此这步可以不做
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	*/
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 第4步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate   = UART3_BAUD;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits   = USART_StopBits_1;
	USART_InitStructure.USART_Parity     = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);	/* 使能接收中断 */
	/*
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
		注意: 不要在此处打开发送中断
		发送中断使能在SendUart()函数打开
	*/
	USART_Cmd(USART3, ENABLE);		/* 使能串口 */

	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(USART3, USART_FLAG_TC);     /* 清发送完成标志，Transmission Complete flag */
#endif

#if UART4_FIFO_EN == 1			/* 串口4 TX = PC10   RX = PC11 */
	/* 第1步： 开启GPIO和UART时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

	/* 第2步：将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 第3步：将USART Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 第4步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate   = UART4_BAUD;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits   = USART_StopBits_1;
	USART_InitStructure.USART_Parity     = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART4, &USART_InitStructure);

	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);	/* 使能接收中断 */
	/*
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
		注意: 不要在此处打开发送中断
		发送中断使能在SendUart()函数打开
	*/
	USART_Cmd(UART4, ENABLE);		/* 使能串口 */

	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(UART4, USART_FLAG_TC);     /* 清发送完成标志，Transmission Complete flag */
#endif

#if UART5_FIFO_EN == 1			/* 串口5 TX = PC12   RX = PD2 */
	/* 第1步： 开启GPIO和UART时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

	/* 第2步：将USART Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 第3步：将USART Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);


	/* 第4步： 配置串口硬件参数 */
	USART_InitStructure.USART_BaudRate   = UART5_BAUD;	/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits   = USART_StopBits_1;
	USART_InitStructure.USART_Parity     = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART5, &USART_InitStructure);

	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);	/* 使能接收中断 */
	/*
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
		注意: 不要在此处打开发送中断
		发送中断使能在SendUart()函数打开
	*/
	USART_Cmd(UART5, ENABLE);		/* 使能串口 */

	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(UART5, USART_FLAG_TC);     /* 清发送完成标志，Transmission Complete flag */
#endif
}

/*******************************************************************************
 * 名    称： ConfigUartNVIC
 * 功    能： 配置串口硬件中断
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
static void ConfigUartNVIC(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure the NVIC Preemption Priority Bits */
	/*	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  --- 在 bsp.c 中 bsp_Init() 中配置中断优先级组 */

#if UART1_FIFO_EN == 1
	/* 使能串口1中断 */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#if UART2_FIFO_EN == 1
	/* 使能串口2中断 */
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#if UART3_FIFO_EN == 1
	/* 使能串口3中断t */
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#if UART4_FIFO_EN == 1
	/* 使能串口4中断t */
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#if UART5_FIFO_EN == 1
	/* 使能串口5中断t */
	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

#if UART6_FIFO_EN == 1
	/* 使能串口6中断t */
	NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif
}

/*******************************************************************************
 * 名    称： UartSend
 * 功    能： 填写数据到UART发送缓冲区,并启动发送中断。中断处理函数发送完毕后，自动关闭发送中断
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
static void UartSend(UART_T *_pUart, uint8_t *_ucaBuf, uint16_t _usLen)
{
    uint16_t i;

    for (i = 0; i < _usLen; i++)
    {
        /* 如果发送缓冲区已经满了，则等待缓冲区空 */
        while (1)
        {
            __IO uint16_t usCount;

            DISABLE_INT();
            usCount = _pUart->usTxCount;
            ENABLE_INT();

            if (usCount < _pUart->usTxBufSize)
            {
                break;
            }
            else if(usCount == _pUart->usTxBufSize)/* 数据已填满缓冲区 */
            {
                if((_pUart->uart->CR1 & USART_CR1_TXEIE) == 0)
                {
                    USART_ITConfig(_pUart->uart, USART_IT_TXE, ENABLE);
                }
            }
        }

        /* 将新数据填入发送缓冲区 */
        _pUart->pTxBuf[_pUart->usTxWrite] = _ucaBuf[i];

        DISABLE_INT();
        if (++_pUart->usTxWrite >= _pUart->usTxBufSize)
        {
            _pUart->usTxWrite = 0;
        }
        _pUart->usTxCount++;
        ENABLE_INT();
    }

    USART_ITConfig(_pUart->uart, USART_IT_TXE, ENABLE);
}

/*******************************************************************************
 * 名    称： UartGetChar
 * 功    能： 从串口接收缓冲区读取1字节数据 （用于主程序调用）
 * 入口参数： _pUart : 串口设备
 *			  _pByte : 存放读取数据的指针
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
static uint8_t UartGetChar(UART_T *_pUart, uint8_t *_pByte)
{
	uint16_t usCount;

	/* usRxWrite 变量在中断函数中被改写，主程序读取该变量时，必须进行临界区保护 */
	DISABLE_INT();
	usCount = _pUart->usRxCount;
	ENABLE_INT();

	/* 如果读和写索引相同，则返回0 */
	//if (_pUart->usRxRead == usRxWrite)
	if (usCount == 0) {	/* 已经没有数据 */
		return 0;
	} else {
		*_pByte = _pUart->pRxBuf[_pUart->usRxRead];		/* 从串口接收FIFO取1个数据 */

		/* 改写FIFO读索引 */
		DISABLE_INT();
		if (++_pUart->usRxRead >= _pUart->usRxBufSize) {
			_pUart->usRxRead = 0;
		}
		_pUart->usRxCount--;
		ENABLE_INT();
		return 1;
	}
}

/*******************************************************************************
 * 名    称： UartIRQ
 * 功    能： 供中断服务程序调用，通用串口中断处理函数
 * 入口参数： _pUart : 串口设备
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
static void UartIRQ(UART_T *_pUart)
{
	/* 处理接收中断  */
	if (USART_GetITStatus(_pUart->uart, USART_IT_RXNE) != RESET) {
		/* 从串口接收数据寄存器读取数据存放到接收FIFO */
		uint8_t ch;

		ch = USART_ReceiveData(_pUart->uart);
		_pUart->pRxBuf[_pUart->usRxWrite] = ch;
		if (++_pUart->usRxWrite >= _pUart->usRxBufSize) {
			_pUart->usRxWrite = 0;
		}
		if (_pUart->usRxCount < _pUart->usRxBufSize) {
			_pUart->usRxCount++;
		}

		/* 回调函数,通知应用程序收到新数据,一般是发送1个消息或者设置一个标记 */
		//if (_pUart->usRxWrite == _pUart->usRxRead)
		//if (_pUart->usRxCount == 1)
		{
			if (_pUart->ReciveNew) {
				_pUart->ReciveNew(ch);
			}
		}
	}

	/* 处理发送缓冲区空中断 */
	if (USART_GetITStatus(_pUart->uart, USART_IT_TXE) != RESET) {
		//if (_pUart->usTxRead == _pUart->usTxWrite)
		if (_pUart->usTxCount == 0) {
			/* 发送缓冲区的数据已取完时， 禁止发送缓冲区空中断 （注意：此时最后1个数据还未真正发送完毕）*/
			USART_ITConfig(_pUart->uart, USART_IT_TXE, DISABLE);

			/* 使能数据发送完毕中断 */
			USART_ITConfig(_pUart->uart, USART_IT_TC, ENABLE);
		} else {
			/* 从发送FIFO取1个字节写入串口发送数据寄存器 */
			USART_SendData(_pUart->uart, _pUart->pTxBuf[_pUart->usTxRead]);
			if (++_pUart->usTxRead >= _pUart->usTxBufSize) {
				_pUart->usTxRead = 0;
			}
			_pUart->usTxCount--;
		}
    /* 数据bit位全部发送完毕的中断 */
	} else if (USART_GetITStatus(_pUart->uart, USART_IT_TC) != RESET) {
		//if (_pUart->usTxRead == _pUart->usTxWrite)
		if (_pUart->usTxCount == 0) {
			/* 如果发送FIFO的数据全部发送完毕，禁止数据发送完毕中断 */
			USART_ITConfig(_pUart->uart, USART_IT_TC, DISABLE);

			/* 回调函数, 一般用来处理RS485通信，将RS485芯片设置为接收模式，避免抢占总线 */
			if (_pUart->SendOver) {
				_pUart->SendOver();
			}
		} else {
			/* 正常情况下，不会进入此分支 */

			/* 如果发送FIFO的数据还未完毕，则从发送FIFO取1个数据写入发送数据寄存器 */
			USART_SendData(_pUart->uart, _pUart->pTxBuf[_pUart->usTxRead]);
			if (++_pUart->usTxRead >= _pUart->usTxBufSize) {
				_pUart->usTxRead = 0;
			}
			_pUart->usTxCount--;
		}
	}
}

/*******************************************************************************
 * 名    称： USART1_IRQHandler  USART2_IRQHandler USART3_IRQHandler UART4_IRQHandler UART5_IRQHandler
 * 功    能： USART中断服务程序
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
#if UART1_FIFO_EN == 1
void USART1_IRQHandler(void)
{
	UartIRQ(&g_tUart1);
}
#endif

#if UART2_FIFO_EN == 1
void USART2_IRQHandler(void)
{
	UartIRQ(&g_tUart2);
}
#endif

#if UART3_FIFO_EN == 1
void USART3_IRQHandler(void)
{
	UartIRQ(&g_tUart3);
}
#endif

#if UART4_FIFO_EN == 1
void UART4_IRQHandler(void)
{
	UartIRQ(&g_tUart4);
}
#endif

#if UART5_FIFO_EN == 1
void UART5_IRQHandler(void)
{
	UartIRQ(&g_tUart5);
}
#endif

#if UART6_FIFO_EN == 1
void USART6_IRQHandler(void)
{
	UartIRQ(&g_tUart6);
}
#endif

/*******************************************************************************
 * 名    称： fputc
 * 功    能： 重定义putc函数，这样可以使用printf函数从串口1打印输出
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
int fputc(int ch, FILE *f)
{
#if 0	/* 将需要printf的字符通过串口中断FIFO发送出去，printf函数会立即返回 */
	ComSendChar(COM1, ch);

	return ch;
#else	/* 采用阻塞方式发送每个字符,等待数据发送完毕 */
	/* 写一个字节到USART1 */
	USART_SendData(USART1, (uint8_t) ch);

	/* 等待发送结束 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{}

	return ch;
#endif
}

/*******************************************************************************
 * 名    称： fgetc
 * 功    能： 重定义getc函数，这样可以使用getchar函数从串口1输入数据
 * 入口参数： 无
 * 出口参数： 无
 * 作　　者： Roger-WY.
 * 创建日期： 2016-07-19
 * 修    改：
 * 修改日期：
 * 备    注：
 *******************************************************************************/
int fgetc(FILE *f)
{
#if 1	/* 从串口接收FIFO中取1个数据, 只有取到数据才返回 */
	uint8_t ucData;

	while(ComGetChar(COM1, &ucData) == 0);

	return ucData;
#else
	/* 等待串口1输入数据 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

	return (int)USART_ReceiveData(USART1);
#endif
}

/*****************************  (END OF FILE) *********************************/
