/*
*********************************************************************************************************
*
*	模块名称 : 中断模块
*	文件名称 : stm32f10x_it.c
*	版    本 : V2.0
*	说    明 : 本文件存放所有的中断服务函数。为了便于他人了解程序用到的中断，我们不建议将中断函数移到其他
*			的文件。
*
*			我们只需要添加需要的中断函数即可。一般中断函数名是固定的，除非您修改了启动文件：
*				Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\arm\startup_stm32f10x_hd.s
*
*			启动文件是汇编语言文件，定了每个中断的服务函数，这些函数使用了WEAK 关键字，表示弱定义，因此如
*			果我们在c文件中重定义了该服务函数（必须和它同名），那么启动文件的中断函数将自动无效。这也就
*			函数重定义的概念，这和C++中的函数重载的意义类似。
*
*
*********************************************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"

//------------------------------------------------------------------------------
/*********************************中断服务函数*********************************/
/**
  * @brief  This function handles SysTick Handler.
  * @系统嘀嗒定时器中断服务程序。
  * @param  None
  * @retval None
  */
extern void SysTick_ISR(void);	/* 声明调用外部的函数 */
void SysTick_Handler(void)
{
    SysTick_ISR();
}



/**
  * @brief  This function handles RTC global interrupt request.
  * @param  None
  * @retval : None
  */
//extern void BSP_RTC_ISR_Handler (void);
void RTC_IRQHandler(void)
{
    //BSP_RTC_ISR_Handler();
}


/**
  * @brief  This function handles EXTI0 Handler.
  * @外部中断线（0线）中断服务函数
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0) != RESET) //确保是否产生了EXTI Line中断
    {
        EXTI_ClearITPendingBit(EXTI_Line0);     //清除中断标志位
        //用户任务

    }
}

/**
  * @brief  This function handles TIM2 Handler.
  * @定时器2中断服务函数   该中断服务函数在bsp_timer.c中实现
  * @param  None
  * @retval None
  */
//void TIM2_IRQHandler(void)
//{
//    if ( TIM_GetITStatus(TIM2 , TIM_IT_Update) != RESET )  //溢出中断标志位1时
//    {
//        TIM_ClearITPendingBit(TIM2 , TIM_FLAG_Update);      //清除溢出中断标志位
//        //用户任务
//
//    }
//}


/**
  * @brief  This function handles USART1 Handler.
  * @串口1中断服务函数
  * @param  None
  * @retval None
  */
//void USART1_IRQHandler(void)
//{
//
//}






//------------------------------------------------------------------------------
//不常用的中断服务函数

void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  /* 当硬件失效异常发生时进入死循环 */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}



/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
