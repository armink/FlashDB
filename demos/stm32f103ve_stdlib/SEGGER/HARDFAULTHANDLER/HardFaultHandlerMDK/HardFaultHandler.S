;
;----------------------------------------------------------------------
;File    : HardFaultHandler.S
;Purpose : HardFault exception handler for IAR, Keil and GNU assembler.
;          Evaluates used stack (MSP, PSP) and passes appropiate stack
;          pointer to the HardFaultHandler "C"-routine.
;------------- END-OF-HEADER ------------------------------------------

;
;/*********************************************************************
;*
;*     Forward declarations of segments used
;*
;**********************************************************************
;*/
	AREA    OSKERNEL, CODE, READONLY, ALIGN=2
	PRESERVE8

	EXPORT  HardFault_Handler
		
	IMPORT  HardFaultHandler

	THUMB


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

HardFault_Handler

        ;// This version is for Cortex M3, Cortex M4 and Cortex M4F
        tst    LR, #4            ;// Check EXC_RETURN in Link register bit 2.
        ite    EQ
        mrseq  R0, MSP           ;// Stacking was using MSP.
        mrsne  R0, PSP           ;// Stacking was using PSP.
        b      HardFaultHandler  ;// Stack pointer passed through R0.

        END

;/****** End Of File *************************************************/
