/*******************************************************************************
 *   Filename:       bsp.h
 *   Revised:        All copyrights reserved to Roger-WY.
 *   Revision:       v1.0
 *   Writer:	     Roger-WY.
 *
 *   Description:    板级支持包
 *
 *   Notes:
 *     				E-mail:wangyao19920204@msn.cn
 *
 *******************************************************************************/

#ifndef __BSP_H__
#define __BSP_H__

/* 开关全局中断的宏 */
#define ENABLE_INT()	__set_PRIMASK(0)	/* 使能全局中断 */
#define DISABLE_INT()	__set_PRIMASK(1)	/* 禁止全局中断 */

/***********************************************
* 描述： 常用外设
*/

#include "bsp_timer.h"

#include "bsp_uart_fifo.h"




/***********************************************
* 描述： I2C总线 及 所有I2C接口的器件
*/

/***********************************************
* 描述： SPI总线 及 所有SPI接口的器件
*/




/***********************************************
* 描述： 供外部调用的函数声明
*/
void bsp_Idle(void);
void bsp_Init(void);
static void NVIC_Configuration(void);

#endif

/*****************************  (END OF FILE) *********************************/
