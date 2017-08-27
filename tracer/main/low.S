/*****************************************************************************\
*                  ____  __  __        ____      ____                         *
*                 / __ \/ /_/ /_____  / __ \    /  _/___  _____               *
*                / / / / __/ __/ __ \/ / / /    / // __ \/ ___/               *
*               / /_/ / /_/ /_/ /_/ / /_/ /   _/ // / / / /___                *
*               \____/\__/\__/\____/\___\_\  /___/_/ /_/\___(_)               *
*                                                                             *
*   Copyright (c) 2015-2017 OttoQ Inc.                                        *
*   All rights reserved.                                                      *
*                                                                             *
*   Redistribution  and use in  source and  binary forms,  with  or without   *
*   modification, are  permitted provided that the following conditions are   *
*   met:                                                                      *
*                                                                             *
*   1.  Redistributions  of  source  code  must retain the above  copyright   *
*       notice, this list of conditions and the following disclaimer.         *
*   2.  Redistributions in binary  form must reproduce  the above copyright   *
*       notice, this list  of conditions and the  following  disclaimer  in   *
*       the   documentation   and/or  other  materials  provided  with  the   *
*       distribution.                                                         *
*   3.  All  advertising  materials  mentioning  features  or  use of  this   *
*       software  must display the following acknowledgment:  "This product   *
*       includes software developed by OttoQ Inc."                            *
*   4.  The  name OttoQ Inc may not be used to endorse or  promote products   *
*       derived   from   this   software  without  specific  prior  written   *
*       permission.                                                           *
*                                                                             *
*   THIS  SOFTWARE  IS PROVIDED BY OTTOQ INC AND CONTRIBUTORS ``AS IS'' AND   *
*   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED  TO,  THE   *
*   IMPLIED  WARRANTIES  OF  MERCHANTABILITY  AND  FITNESS FOR A PARTICULAR   *
*   PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL OTTOQ INC OR CONTRIBUTORS BE   *
*   LIABLE  FOR  ANY  DIRECT,  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR   *
*   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  TO,  PROCUREMENT  OF   *
*   SUBSTITUTE  GOODS  OR  SERVICES;   LOSS  OF  USE, DATA, OR PROFITS;  OR   *
*   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY  OF  LIABILITY,   *
*   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR   *
*   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  IF   *
*   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                *
*                                                                             *
*   (This license is derived from the Berkeley Public License.)               *
*                                                                             *
\*****************************************************************************/

/*
 *  Vectors and start up.
 */


#include "version.h"

    .syntax unified
    .arch armv7e-m


/**********************************************************************/
/**********************************************************************/

    .section .vectors

LocationZero:

//  Exception Vectors
    .long   __StackTop                  //  Initial stack pointer
    .long   ResetHandler                //  Reset handler
    .long   NMI_Handler
    .long   HardFault_Handler

    .long   MemoryManagement_Handler
    .long   BusFault_Handler
    .long   UsageFault_Handler
    .long   VERSION_HEX                 //  Image version

    .long   0x326b7550                  //  Magic number ("Puk2")
    .long   0                           //  Image size (set by `fixup')
    .long   0                           //  Image checksum (set by `fixup')
    .long   SVC_Handler

    .long   DebugMonitor_Handler
    .long   0
    .long   PendSV_Handler
    .long   SysTick_Handler

//  Interrupts
    .long   POWER_CLOCK_IRQHandler                          //  16 (0x40)
    .long   RADIO_IRQHandler
    .long   UARTE0_UART0_IRQHandler
    .long   SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler

    .long   SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler    //  20 (0x50)
    .long   NFCT_IRQHandler
    .long   GPIOTE_IRQHandler
    .long   SAADC_IRQHandler

    .long   TIMER0_IRQHandler                               //  24 (0x60)
    .long   TIMER1_IRQHandler
    .long   TIMER2_IRQHandler
    .long   RTC0_IRQHandler

    .long   TEMP_IRQHandler                                 //  28 (0x70)
    .long   RNG_IRQHandler
    .long   ECB_IRQHandler
    .long   CCM_AAR_IRQHandler

    .long   WDT_IRQHandler                                  //  32 (0x80)
    .long   RTC1_IRQHandler
    .long   QDEC_IRQHandler
    .long   COMP_LPCOMP_IRQHandler

    .long   SWI0_EGU0_IRQHandler                            //  36 (0x90)
    .long   SWI1_EGU1_IRQHandler
    .long   SWI2_EGU2_IRQHandler
    .long   SWI3_EGU3_IRQHandler

    .long   SWI4_EGU4_IRQHandler                            //  40 (0xa0)
    .long   SWI5_EGU5_IRQHandler
    .long   TIMER3_IRQHandler
    .long   TIMER4_IRQHandler

    .long   PWM0_IRQHandler                                 //  44 (0xb0)
    .long   PDM_IRQHandler
    .long   0
    .long   0

    .long   MWU_IRQHandler                                  //  48 (0xc0)
    .long   PWM1_IRQHandler
    .long   PWM2_IRQHandler
    .long   SPIM2_SPIS2_SPI2_IRQHandler

    .long   RTC2_IRQHandler                                 //  52 (0xd0)
    .long   I2S_IRQHandler
    .long   FPU_IRQHandler
    .long   0

    .long   0                                               //  56 (0xe0)
    .long   0
    .long   0
    .long   0

    .long   0                                               //  60 (0xf0)
    .long   0
    .long   0
    .long   0

    .long   0                                               //  64 (0x100)


        //  Note that the soft devices for the nrf52 forward exceptions
        //  and interrupts up to vector 64 (0x100) to the beginning of
        //  the App image.  The code used to do this is something like this:
        //
        //  vec60:
        //      movs    r0, #0xf0           //  Vector 60 (0xf0)
        //      ldr     r1, 1f              //  r1 = 0x20000000
        //      ldr     r1, [r1]            //  r1 = *0x20000000
        //      ldr     r1, [r1, r0]        //  r1 = vector handler address
        //      bx      r1                  //  go there
        //  1:  .long   0x20000000
        //
        //  RAM location 0x20000000 contains the address of the current
        //  vector table.  The soft device usually points this to itself,
        //  but presumably the App could overwrite this for it's own
        //  vector table.
        //
        //  More info:  In the typical case, *0x20000000 is the Soft Device
        //  vector table, at 0x1000.  *0x20000004 is the App vector table,
        //  at the App flash start address (0x1c000 for the s132).  The SD
        //  vector handler will grab the vector from *0x20000004 and jump
        //  there.  It makes for about a 14 instruction overhead for each
        //  interrupt/exception vector.
        //
        //  The Soft Device handler code is something like this:
        //
        //  vec60:
        //      ldr     r2, 10f             //  r2 = 0x20000098
        //      ldr     r2, [r2]            //  r2 = *0x20000098
        //      ldr     r3, 11f             //  r3 = 0xcafebabe
        //      cmp     r2, r3
        //      bne     2f
        //
        //      ...     some stuff          //  Cafe Babe flag set, special case
        //
        //  2:
        //      ldr     r1, 12f             //  r1 = 0x20000004
        //      ldr     r1, [r1]            //  r1 = *0x20000004
        //      ldr     r1, [r1, r0]        //  r1 = handlr addr (r0 from above)
        //      bx      r1                  //  Go!
        //
        //  10: .long   0x20000098
        //  11: .long   0xcafebabe
        //  12: .long   0x20000004


    .size LocationZero, . - LocationZero

/**********************************************************************/
/**********************************************************************/
/*
 *  Reset handler
 */

    .text
    .thumb
    .thumb_func
    .align 1

    .globl  ResetHandler
    .type   ResetHandler, %function
ResetHandler:

    /*
     *  Copy initialized data from from where it was stored in Flash by
     *  the linker, to it's final location in RAM.  The data segment is
     *  assumed to start and finish on a 4 byte boundary, and that there
     *  is at least 4 bytes (it copies at least 4 bytes).
     */
    ldr     r1, =__data_load_start
    ldr     r2, =__data_load_end
    ldr     r3, =__data_start__
1:
    ldr     r0, [r1], #4
    str     r0, [r3], #4
    cmp     r1, r2
    blt     1b


    /*
     *  Clear the BSS section.
     */
    mov     r0, #0
    ldr     r1, =__bss_start__
    ldr     r2, =__bss_end__
1:
    str     r0, [r1], #4
    cmp     r1, r2
    blt     1b


    /*
     *  Execute SystemInit function
     */
    bl      SystemInit

    /*
     *  GO!! -- run the system.  (It should never return.)
     */
    bl      Main
    b       .

    .pool
    .size   ResetHandler, .-ResetHandler

/**********************************************************************/
/**********************************************************************/

//  Exception handler macro
    .macro  EXCEPT handler
    .weak   \handler
    .type   \handler, %function
\handler:
    b       .
    .size   \handler, . - \handler
    .endm

//  Interrupt handler macro
    .macro  IRQ handler
    .weak   \handler
    .set    \handler, Default_Handler
    .endm

/**********/

    .text

//  Exception handlers

    EXCEPT  NMI_Handler
    EXCEPT  HardFault_Handler
    EXCEPT  MemoryManagement_Handler
    EXCEPT  BusFault_Handler
    EXCEPT  UsageFault_Handler
    EXCEPT  SVC_Handler
    EXCEPT  DebugMonitor_Handler
    EXCEPT  PendSV_Handler
    EXCEPT  SysTick_Handler

/**********/

//  Interrupt handlers

    IRQ     POWER_CLOCK_IRQHandler
    IRQ     RADIO_IRQHandler
    IRQ     UARTE0_UART0_IRQHandler
    IRQ     SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler
    IRQ     SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler
    IRQ     NFCT_IRQHandler
    IRQ     GPIOTE_IRQHandler
    IRQ     SAADC_IRQHandler
    IRQ     TIMER0_IRQHandler
    IRQ     TIMER1_IRQHandler
    IRQ     TIMER2_IRQHandler
    IRQ     RTC0_IRQHandler
    IRQ     TEMP_IRQHandler
    IRQ     RNG_IRQHandler
    IRQ     ECB_IRQHandler
    IRQ     CCM_AAR_IRQHandler
    IRQ     WDT_IRQHandler
    IRQ     RTC1_IRQHandler
    IRQ     QDEC_IRQHandler
    IRQ     COMP_LPCOMP_IRQHandler
    IRQ     SWI0_EGU0_IRQHandler
    IRQ     SWI1_EGU1_IRQHandler
    IRQ     SWI2_EGU2_IRQHandler
    IRQ     SWI3_EGU3_IRQHandler
    IRQ     SWI4_EGU4_IRQHandler
    IRQ     SWI5_EGU5_IRQHandler
    IRQ     TIMER3_IRQHandler
    IRQ     TIMER4_IRQHandler
    IRQ     PWM0_IRQHandler
    IRQ     PDM_IRQHandler
    IRQ     MWU_IRQHandler
    IRQ     PWM1_IRQHandler
    IRQ     PWM2_IRQHandler
    IRQ     SPIM2_SPIS2_SPI2_IRQHandler
    IRQ     RTC2_IRQHandler
    IRQ     I2S_IRQHandler
    IRQ     FPU_IRQHandler

/**********/

    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       .
    .size   Default_Handler, . - Default_Handler

/**********************************************************************/
/**********************************************************************/

/*
 *  The system Stack.
 */

    .section .stack
    .align 3

#ifdef __STACK_SIZE
    .equ    Stack_Size, __STACK_SIZE
#else
    .equ    Stack_Size, 8192
#endif


    .globl __StackTop
    .globl __StackLimit
__StackLimit:
    .space Stack_Size
    .size __StackLimit, .-__StackLimit
__StackTop:
    .size __StackTop, .-__StackTop

/**********************************************************************/
/**********************************************************************/

  .end
