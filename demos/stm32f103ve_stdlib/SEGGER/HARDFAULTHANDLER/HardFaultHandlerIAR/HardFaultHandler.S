/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2017 SEGGER Microcontroller GmbH & Co. KG         *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: 4.32                                             *
*                                                                    *
**********************************************************************
;
;----------------------------------------------------------------------
;File    : HardFaultHandler.S
;Purpose : HardFault exception handler for IAR, Keil and GNU assembler.
;          Evaluates used stack (MSP, PSP) and passes appropiate stack
;          pointer to the HardFaultHandler "C"-routine.
;------------- END-OF-HEADER ------------------------------------------
;*/

#ifndef __IAR_SYSTEMS_ASM__
  #ifndef __CC_ARM
    #ifndef __GNUC__
      #error "Unsupported assembler!"
    #endif
  #endif
#endif

;/*********************************************************************
;*
;*     Forward declarations of segments used
;*
;**********************************************************************
;*/

#ifdef __IAR_SYSTEMS_ASM__

        SECTION CODE:CODE:NOROOT(2)
        SECTION CSTACK:DATA:NOROOT(3)

#elif defined __CC_ARM

        AREA    OSKERNEL, CODE, READONLY, ALIGN=2
        PRESERVE8

#endif

;/*********************************************************************
;*
;*     Publics
;*
;**********************************************************************
;*/

#ifdef __IAR_SYSTEMS_ASM__

        SECTION .text:CODE:NOROOT(2)
        PUBLIC  HardFault_Handler

#elif defined __CC_ARM

        EXPORT  HardFault_Handler

#elif defined __GNUC__

        .global HardFault_Handler
        .type   HardFault_Handler, function

#endif

;/*********************************************************************
;*
;*     Externals, code
;*
;**********************************************************************
;*/

#ifdef __IAR_SYSTEMS_ASM__

        EXTERN  HardFaultHandler

#elif defined __CC_ARM

        IMPORT  HardFaultHandler

#elif defined __GNUC__

        .extern HardFaultHandler

#endif

;/*********************************************************************
;*
;*     CODE segment
;*
;**********************************************************************
;*/

#ifdef __GNUC__

        .syntax unified
        .thumb
        .balign 4
        .text

#else

        THUMB

#endif

;/*********************************************************************
;*
;*       Global functions
;*
;**********************************************************************
;*/

;/*********************************************************************
;*
;*      HardFault_Handler()
;*
;*  Function description
;*    Evaluates the used stack (MSP, PSP) and passes the appropiate
;*    stack pointer to the HardFaultHandler "C"-routine.
;*
;*  Notes
;*    (1) Ensure that HardFault_Handler is part of the exception table
;*/
#ifdef __GNUC__
HardFault_Handler:
#else
HardFault_Handler
#endif
#if (defined (__IAR_SYSTEMS_ASM__) && (__ARM6M__) && (__CORE__ == __ARM6M__)) || \
    (defined (__CC_ARM) && (__TARGET_ARCH_6S_M))                              || \
    (defined (__GNUC__) && (__ARM_ARCH_6M__))
        ;// This version is for Cortex M0
        movs   R0, #4
        mov    R1, LR
        tst    R0, R1            ;// Check EXC_RETURN in Link register bit 2.
        bne    Uses_PSP
        mrs    R0, MSP           ;// Stacking was using MSP.
        b      Pass_StackPtr
#ifdef __GNUC__
Uses_PSP:
#else
Uses_PSP
#endif
        mrs    R0, PSP           ;// Stacking was using PSP.
#ifdef __GNUC__
Pass_StackPtr:
#else
Pass_StackPtr
#endif
#ifdef __CC_ARM
        ALIGN
#endif
        ldr    R2,=HardFaultHandler
        bx     R2                ;// Stack pointer passed through R0.

#else
        ;// This version is for Cortex M3, Cortex M4 and Cortex M4F
        tst    LR, #4            ;// Check EXC_RETURN in Link register bit 2.
        ite    EQ
        mrseq  R0, MSP           ;// Stacking was using MSP.
        mrsne  R0, PSP           ;// Stacking was using PSP.
        b      HardFaultHandler  ;// Stack pointer passed through R0.
#endif

#ifdef __GNUC__
        .end
#else
        END
#endif

;/****** End Of File *************************************************/
