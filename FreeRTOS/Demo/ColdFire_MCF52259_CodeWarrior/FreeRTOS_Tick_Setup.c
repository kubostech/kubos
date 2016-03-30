/*
    FreeRTOS V9.0.0rc2 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#include "FreeRTOS.h"
#include "task.h"

__declspec(interrupt:0) void vPIT0InterruptHandler( void );

/* Constants used to configure the interrupts. */
#define portPRESCALE_VALUE			64
#define portPRESCALE_REG_SETTING	( 5 << 8 )
#define portPIT_INTERRUPT_ENABLED	( 0x08 )
#define configPIT0_INTERRUPT_VECTOR	( 55 )

/*
 * FreeRTOS.org requires two interrupts - a tick interrupt generated from a
 * timer source, and a spare interrupt vector used for context switching.
 * The configuration below uses PIT0 for the former, and vector 16 for the
 * latter.  **IF YOUR APPLICATION HAS BOTH OF THESE INTERRUPTS FREE THEN YOU DO
 * NOT NEED TO CHANGE ANY OF THIS CODE** - otherwise instructions are provided
 * here for using alternative interrupt sources.
 *
 * To change the tick interrupt source:
 *
 *	1) Modify vApplicationSetupInterrupts() below to be correct for whichever
 *	peripheral is to be used to generate the tick interrupt.  The name of the
 *  handler function (currently vPIT0InterruptHandler()) should also be updated
 *  to indicate which peripheral is generating the interrupt.
 *
 *	2) Make sure the interrupt source is cleared within the interrupt handler function.
 *  Currently vPIT0InterruptHandler() clears the PIT0 interrupt.
 *
 *  3) Update the vector table within mcf5225x_vectors.s to install the tick
 *  interrupt handler in the correct vector position.
 *
 * To change the spare interrupt source:
 *
 *  1) Modify vApplicationSetupInterrupts() below to be correct for whichever
 *  interrupt vector is to be used.  Make sure you use a spare interrupt on interrupt
 *  controller 0, otherwise the register used to request context switches will also
 *  require modification.
 *
 *  2) Change the definition of configYIELD_INTERRUPT_VECTOR within FreeRTOSConfig.h
 *  to be correct for your chosen interrupt vector.
 *
 *  3) Update the vector table within mcf5225x_vectors.s to install the handler
 *  _vPortYieldISR() in the correct vector position (by default vector number 16 is
 *  used).
 */
void vApplicationSetupInterrupts( void )
{
const unsigned short usCompareMatchValue = ( ( configCPU_CLOCK_HZ / portPRESCALE_VALUE ) / configTICK_RATE_HZ );

    /* Configure interrupt priority and level and unmask interrupt for PIT0. */
    MCF_INTC0_ICR55 = ( 1 | ( configKERNEL_INTERRUPT_PRIORITY << 3 ) );
    MCF_INTC0_IMRH &= ~( MCF_INTC_IMRH_INT_MASK55 );

    /* Do the same for vector 63 (interrupt controller 0.  I don't think the
    write to MCF_INTC0_IMRH is actually required here but is included for
    completeness. */
    MCF_INTC0_ICR16 = ( 0 | configKERNEL_INTERRUPT_PRIORITY << 3 );
    MCF_INTC0_IMRL &= ~( MCF_INTC_IMRL_INT_MASK16 | 0x01 );

    /* Configure PIT0 to generate the RTOS tick. */
    MCF_PIT0_PCSR |= MCF_PIT_PCSR_PIF;
    MCF_PIT0_PCSR = ( portPRESCALE_REG_SETTING | MCF_PIT_PCSR_PIE | MCF_PIT_PCSR_RLD | MCF_PIT_PCSR_EN );
	MCF_PIT0_PMR = usCompareMatchValue;
}
/*-----------------------------------------------------------*/

__declspec(interrupt:0) void vPIT0InterruptHandler( void )
{
unsigned long ulSavedInterruptMask;

	/* Clear the PIT0 interrupt. */
	MCF_PIT0_PCSR |= MCF_PIT_PCSR_PIF;

	/* Increment the RTOS tick. */
	ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
		if( xTaskIncrementTick() != pdFALSE )
		{
			taskYIELD();
		}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( ulSavedInterruptMask );
}
