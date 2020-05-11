/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                              (c) Copyright 2016; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            STARTUP CODE
*
*                                      Texas Instruments MSP432
*                                               on the
*
*                                        TI MSP-EXP432P401R
*                                     LaunchPad Development Kit
*
* Filename      : msp432_startup.c
* Version       : V1.00
* Programmer(s) : YS
*********************************************************************************************************
*/

#include  <lib_def.h>
#include  <bsp_int.h>
#include  <bsp_sys.h>

#include  <os.h>
#include  <stdint.h>


#include  <msp432.h>


static  void  App_Reset_ISR       (void);

static  void  App_NMI_ISR         (void);

static  void  App_Fault_ISR       (void);

static  void  App_BusFault_ISR    (void);

static  void  App_UsageFault_ISR  (void);

static  void  App_Spurious_ISR    (void);
                                                                /* External declaration for the reset handler that...   */
                                                                /* ...is to be called when the processor is started     */
extern  void  _c_int00(void);
extern  void  SystemInit();

                                                                /* Linker variable that marks the top of the stack.     */
extern  unsigned  long  __STACK_END;



                                                                /* Intrrupt vector table. Note that the proper...       */
                                                                /*...constructs must be placed on this to ensure that...*/
                                                                /*...ends up at physical addr 0x0000.0000 or at the...  */
                                                                /*...start of the program if located at start addr...   */
                                                                /*...other than 0.                                      */
#pragma RETAIN(interruptVectors)
#pragma DATA_SECTION(interruptVectors, ".intvecs")
void (* const interruptVectors[])(void) =
{
    (void (*)(void))((uint32_t)&__STACK_END),
                                                                /* 000 Initial stack pointer.                           */
    App_Reset_ISR,                                              /* 001 Initial program counter.                         */
    App_NMI_ISR,                                                /* 002 Non-maskable interrupt.                          */
    App_Fault_ISR,                                              /* 003 Hard fault exception.                            */
    App_Spurious_ISR,                                           /* 004 Reserved interrupt 4.                            */
    App_BusFault_ISR,                                           /* 005 Bus fault exception.                             */
    App_UsageFault_ISR,                                         /* 006 Usage fault exception.                           */
    App_Spurious_ISR,                                           /* 007 Reserved interrupt 7.                            */
    App_Spurious_ISR,                                           /* 008 Reserved interrupt 8.                            */
    App_Spurious_ISR,                                           /* 009 Reserved interrupt 9.                            */
    App_Spurious_ISR,                                           /* 010 Reserved interrupt 10.                           */
    App_Spurious_ISR,                                           /* 011 A supervisor call exception.                     */
    App_Spurious_ISR,                                           /* 012 Debug Monitor.                                   */
    App_Spurious_ISR,                                           /* 013 Reserved interrupt 13.                           */
    OS_CPU_PendSVHandler,                                       /* 014 PendSV exception.                                */
    OS_CPU_SysTickHandler,                                      /* 015 SysTick Interrupt.                               */

    BSP_IntHandlerPSS,                                          /* 016 IRQ[  0] PSS ISR                                 */
    BSP_IntHandlerCS,                                           /* 017 IRQ[  1] CS ISR                                  */
    BSP_IntHandlerPCM,                                          /* 018 IRQ[  2] PCM ISR                                 */
    BSP_IntHandlerWTD,                                          /* 019 IRQ[  3] WDT ISR                                 */
    BSP_IntHandlerFPU,                                          /* 020 IRQ[  4] FPU ISR                                 */
    BSP_IntHandlerFLCTL,                                        /* 021 IRQ[  5] FLCTL ISR                               */
    BSP_IntHandlerCOMP0,                                        /* 022 IRQ[  6] COMP0 ISR                               */
    BSP_IntHandlerCOMP1,                                        /* 023 IRQ[  7] COMP1 ISR                               */
    BSP_IntHandlerTA0_0,                                        /* 024 IRQ[  8] TA0_0 ISR                               */
    BSP_IntHandlerTA0_N,                                        /* 025 IRQ[  9] TA0_N ISR                               */
    BSP_IntHandlerTA1_0,                                        /* 026 IRQ[ 10] TA1_0 ISR                               */
    BSP_IntHandlerTA1_N,                                        /* 027 IRQ[ 11] TA1_N ISR                               */
    BSP_IntHandlerTA2_0,                                        /* 028 IRQ[ 12] TA2_0 ISR                               */
    BSP_IntHandlerTA2_N,                                        /* 029 IRQ[ 13] TA2_N ISR                               */
    BSP_IntHandlerTA3_0,                                        /* 030 IRQ[ 14] TA3_0 ISR                               */
    BSP_IntHandlerTA3_N,                                        /* 031 IRQ[ 15] TA3_N ISR                               */
    BSP_IntHandlerEUSCIA0,                                      /* 032 IRQ[ 16] EUSCIA0 ISR                             */
    BSP_IntHandlerEUSCIA1,                                      /* 033 IRQ[ 17] EUSCIA1 ISR                             */
    BSP_IntHandlerEUSCIA2,                                      /* 034 IRQ[ 18] EUSCIA2 ISR                             */
    BSP_IntHandlerEUSCIA3,                                      /* 035 IRQ[ 19] EUSCIA3 ISR                             */
    BSP_IntHandlerEUSCIB0,                                      /* 036 IRQ[ 20] EUSCIB0 ISR                             */
    BSP_IntHandlerEUSCIB1,                                      /* 037 IRQ[ 21] EUSCIB1 ISR                             */
    BSP_IntHandlerEUSCIB2,                                      /* 038 IRQ[ 22] EUSCIB2 ISR                             */
    BSP_IntHandlerEUSCIB3,                                      /* 039 IRQ[ 23] EUSCIB3 ISR                             */
    BSP_IntHandlerADC14,                                        /* 040 IRQ[ 24] ADC14 ISR                               */
    BSP_IntHandlerT32_INT1,                                     /* 041 IRQ[ 25] T32_INT1 ISR                            */
    BSP_IntHandlerT32_INT2,                                     /* 042 IRQ[ 26] T32_INT2 ISR                            */
    BSP_IntHandlerT32_INTC,                                     /* 043 IRQ[ 27] T32_INTC ISR                            */
    BSP_IntHandlerAES,                                          /* 044 IRQ[ 28] AES ISR                                 */
    BSP_IntHandlerRTC,                                          /* 045 IRQ[ 29] RTC ISR                                 */
    BSP_IntHandlerDMA_ERR,                                      /* 046 IRQ[ 30] DMA_ERR ISR                             */
    BSP_IntHandlerDMA_INT3,                                     /* 047 IRQ[ 31] DMA_INT3 ISR                            */
    BSP_IntHandlerDMA_INT2,                                     /* 048 IRQ[ 32] DMA_INT2 ISR                            */
    BSP_IntHandlerDMA_INT1,                                     /* 049 IRQ[ 33] DMA_INT1 ISR                            */
    BSP_IntHandlerDMA_INT0,                                     /* 050 IRQ[ 34] DMA_INT0 ISR                            */
    BSP_IntHandlerPORT1,                                        /* 051 IRQ[ 35] PORT1 ISR                               */
    BSP_IntHandlerPORT2,                                        /* 052 IRQ[ 36] PORT2 ISR                               */
    BSP_IntHandlerPORT3,                                        /* 053 IRQ[ 37] PORT3 ISR                               */
    BSP_IntHandlerPORT4,                                        /* 054 IRQ[ 38] PORT4 ISR                               */
    BSP_IntHandlerPORT5,                                        /* 055 IRQ[ 39] PORT5 ISR                               */
    BSP_IntHandlerPORT6,                                        /* 056 IRQ[ 40] PORT6 ISR                               */
};


/*
*********************************************************************************************************
*                                            App_Reset_ISR()
*
* Description : Handle Reset.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_Reset_ISR (void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;            // Halt the WDT
                                                                /* Jump to the CCS C Initialization Routine.            */

    __asm("    .global _c_int00\n"
          "    b.w     _c_int00");


}


/*
*********************************************************************************************************
*                                            App_NMI_ISR()
*
* Description : Handle Non-Maskable Interrupt (NMI).
* Argument(s) : none.
* Return(s)   : none.
* Caller(s)   : This is an ISR.
*
* Note(s)     : (1) Since the NMI is not being used, this serves merely as a catch for a spurious
*                   exception.
*********************************************************************************************************
*/

static  void  App_NMI_ISR (void)
{
    while (DEF_TRUE) {
        ;
    }
}


/*
*********************************************************************************************************
*                                             App_Fault_ISR()
*
* Description : Handle hard fault.
* Argument(s) : none.
* Return(s)   : none.
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_Fault_ISR (void)
{
    while (DEF_TRUE) {
        ;
    }
}


/*
*********************************************************************************************************
*                                           App_BusFault_ISR()
*
* Description : Handle bus fault.
* Argument(s) : none.
* Return(s)   : none.
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_BusFault_ISR (void)
{
    while (DEF_TRUE) {
        ;
    }
}


/*
*********************************************************************************************************
*                                          App_UsageFault_ISR()
*
* Description : Handle usage fault.
* Argument(s) : none.
* Return(s)   : none.
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_UsageFault_ISR (void)
{
    while (DEF_TRUE) {
        ;
    }
}


/*
*********************************************************************************************************
*                                           App_Spurious_ISR()
*
* Description : Handle spurious interrupt.
* Argument(s) : none.
* Return(s)   : none.
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_Spurious_ISR (void)
{
    while (DEF_TRUE) {
        ;
    }
}
