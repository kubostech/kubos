/*
    FreeRTOS V7.4.0 - Copyright (C) 2013 Real Time Engineers Ltd.

    FEATURES AND PORTS ARE ADDED TO FREERTOS ALL THE TIME.  PLEASE VISIT
    http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.

    >>>>>>NOTE<<<<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License
    and the FreeRTOS license exception along with FreeRTOS; if not itcan be
    viewed here: http://www.freertos.org/a00114.html and also obtained by
    writing to Real Time Engineers Ltd., contact details for whom are available
    on the FreeRTOS WEB site.

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************


    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, and our new
    fully thread aware and reentrant UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems, who sell the code with commercial support,
    indemnification and middleware, under the OpenRTOS brand.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.
*/

/* Standard includes. */
#include "limits.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Library includes. */
#include <asf.h>


/*
 * When configCREATE_LOW_POWER_DEMO is set to 1 then the tick interrupt
 * is generated by the AST.  The AST configuration and handling functions are
 * defined in this file.
 *
 * When configCREATE_LOW_POWER_DEMO is set to 0 the tick interrupt is
 * generated by the standard FreeRTOS Cortex-M port layer, which uses the
 * SysTick timer.
 */
#if configCREATE_LOW_POWER_DEMO == 1

/* Constants required to pend a PendSV interrupt from the tick ISR if the
preemptive scheduler is being used.  These are just standard bits and registers
within the Cortex-M core itself. */
#define portNVIC_INT_CTRL_REG	( * ( ( volatile unsigned long * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT	( 1UL << 28UL )

/* The alarm used to generate interrupts in the asynchronous timer. */
#define portAST_ALARM_CHANNEL	0

/*-----------------------------------------------------------*/

/*
 * The tick interrupt is generated by the asynchronous timer.  The default tick
 * interrupt handler cannot be used (even with the AST being handled from the
 * tick hook function) because the default tick interrupt accesses the SysTick
 * registers when configUSE_TICKLESS_IDLE set to 1.  AST_ALARM_Handler() is the
 * default name for the AST alarm interrupt.  This definition overrides the
 * default implementation that is weakly defined in the interrupt vector table
 * file.
 */
void AST_ALARM_Handler(void);

/*-----------------------------------------------------------*/

/* Calculate how many clock increments make up a single tick period. */
static const uint32_t ulAlarmValueForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );

/* Holds the maximum number of ticks that can be suppressed - which is
basically how far into the future an interrupt can be generated. Set
during initialisation. */
static portTickType xMaximumPossibleSuppressedTicks = 0;

/* Flag set from the tick interrupt to allow the sleep processing to know if
sleep mode was exited because of an AST interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;

/* The AST counter is stopped temporarily each time it is re-programmed.  The
following variable offsets the AST counter alarm value by the number of AST
counts that would typically be missed while the counter was stopped to compensate
for the lost time.  _RB_ Value needs calculating correctly. */
static uint32_t ulStoppedTimerCompensation = 10 / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );

/*-----------------------------------------------------------*/

/* The tick interrupt handler.  This is always the same other than the part that
clears the interrupt, which is specific to the clock being used to generate the
tick. */
void AST_ALARM_Handler(void)
{
	/* If using preemption, also force a context switch by pending the PendSV
	interrupt. */
	#if configUSE_PREEMPTION == 1
	{
		portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
	}
	#endif

	/* Protect incrementing the tick with an interrupt safe critical section. */
	( void ) portSET_INTERRUPT_MASK_FROM_ISR();
	{
		vTaskIncrementTick();

		/* Just completely clear the interrupt mask on exit by passing 0 because
		it is known that this interrupt will only ever execute with the lowest
		possible interrupt priority. */
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( 0 );

	/* The CPU woke because of a tick. */
	ulTickFlag = pdTRUE;

	/* If this is the first tick since exiting tickless mode then the AST needs
	to be reconfigured to generate interrupts at the defined tick frequency. */
	ast_write_alarm0_value( AST, ulAlarmValueForOneTick );

	/* Ensure the interrupt is clear before exiting. */
	ast_clear_interrupt_flag( AST, AST_INTERRUPT_ALARM );
}
/*-----------------------------------------------------------*/

/* Override the default definition of vPortSetupTimerInterrupt() that is weakly
defined in the FreeRTOS Cortex-M3 port layer layer with a version that
configures the asynchronous timer (AST) to generate the tick interrupt. */
void vPortSetupTimerInterrupt( void )
{
struct ast_config ast_conf;

	/* Ensure the AST can bring the CPU out of sleep mode. */
	sleepmgr_lock_mode( SLEEPMGR_RET );

	/* Ensure the 32KHz oscillator is enabled. */
	if( osc_is_ready( OSC_ID_OSC32 ) == pdFALSE )
	{
		osc_enable( OSC_ID_OSC32 );
		osc_wait_ready( OSC_ID_OSC32 );
	}

	/* Enable the AST itself. */
	ast_enable( AST );

	ast_conf.mode = AST_COUNTER_MODE;  /* Simple up counter. */
	ast_conf.osc_type = AST_OSC_32KHZ;
	ast_conf.psel = 0; /* No prescale so the actual frequency is 32KHz/2. */
	ast_conf.counter = 0;
	ast_set_config( AST, &ast_conf );

	/* The AST alarm interrupt is used as the tick interrupt.  Ensure the alarm
	status starts clear. */
	ast_clear_interrupt_flag( AST, AST_INTERRUPT_ALARM );

	/* Enable wakeup from alarm 0 in the AST and power manager.  */
	ast_enable_wakeup( AST, AST_WAKEUP_ALARM );
	bpm_enable_wakeup_source( BPM, ( 1 << BPM_BKUPWEN_AST ) );

	/* Tick interrupt MUST execute at the lowest interrupt priority. */
	NVIC_SetPriority( AST_ALARM_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);
	ast_enable_interrupt( AST, AST_INTERRUPT_ALARM );
	NVIC_ClearPendingIRQ( AST_ALARM_IRQn );
	NVIC_EnableIRQ( AST_ALARM_IRQn );

	/* Automatically clear the counter on interrupt. */
	ast_enable_counter_clear_on_alarm( AST, portAST_ALARM_CHANNEL );

	/* Start with the tick active and generating a tick with regular period. */
	ast_write_alarm0_value( AST, ulAlarmValueForOneTick );
	ast_write_counter_value( AST, 0 );

	/* See the comments where xMaximumPossibleSuppressedTicks is declared. */
	xMaximumPossibleSuppressedTicks = ULONG_MAX / ulAlarmValueForOneTick;
}
/*-----------------------------------------------------------*/

void prvDisableAST( void )
{
	while( ast_is_busy( AST ) )
	{
		/* Nothing to do here, just waiting. */
	}
	AST->AST_CR &= ~( AST_CR_EN );
	while( ast_is_busy( AST ) )
	{
		/* Nothing to do here, just waiting. */
	}
}
/*-----------------------------------------------------------*/

void prvEnableAST( void )
{
	while( ast_is_busy( AST ) )
	{
		/* Nothing to do here, just waiting. */
	}
	AST->AST_CR |= AST_CR_EN;
	while( ast_is_busy( AST ) )
	{
		/* Nothing to do here, just waiting. */
	}
}
/*-----------------------------------------------------------*/

/* Override the default definition of vPortSuppressTicksAndSleep() that is weakly
defined in the FreeRTOS Cortex-M3 port layer layer with a version that manages
the asynchronous timer (AST), as the tick is generated from the low power AST
and not the SysTick as would normally be the case on a Cortex-M. */
void vPortSuppressTicksAndSleep( portTickType xExpectedIdleTime )
{
uint32_t ulAlarmValue, ulCompleteTickPeriods;
eSleepModeStatus eSleepAction;
portTickType xModifiableIdleTime;
enum sleepmgr_mode xSleepMode;

	/* THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED. */

	/* Make sure the AST reload value does not overflow the counter. */
	if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
	{
		xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
	}

	/* Calculate the reload value required to wait xExpectedIdleTime tick
	periods.  -1 is used because this code will execute part way through one of
	the tick periods, and the fraction of a tick period is accounted for
	later. */
	ulAlarmValue = ( ulAlarmValueForOneTick * ( xExpectedIdleTime - 1UL ) );
	if( ulAlarmValue > ulStoppedTimerCompensation )
	{
		/* Compensate for the fact that the AST is going to be stopped
		momentarily. */
		ulAlarmValue -= ulStoppedTimerCompensation;
	}

	/* Stop the AST momentarily.  The time the AST is stopped for is accounted
	for as best it can be, but using the tickless mode will inevitably result in
	some tiny drift of the time maintained by the kernel with respect to
	calendar time. */
	prvDisableAST();

	/* Enter a critical section but don't use the taskENTER_CRITICAL() method as
	that will mask interrupts that should exit sleep mode. */
	__asm volatile( "cpsid i		\n\t"
					"dsb			\n\t" );

	/* The tick flag is set to false before sleeping.  If it is true when sleep
	mode is exited then sleep mode was probably exited because the tick was
	suppressed for the entire xExpectedIdleTime period. */
	ulTickFlag = pdFALSE;

	/* If a context switch is pending then abandon the low power entry as
	the context switch might have been pended by an external interrupt that
	requires processing. */
	eSleepAction = eTaskConfirmSleepModeStatus();
	if( eSleepAction == eAbortSleep )
	{
		/* Restart tick. */
		prvEnableAST();

		/* Re-enable interrupts - see comments above the cpsid instruction()
		above. */
		__asm volatile( "cpsie i" );
	}
	else
	{
		/* Adjust the alarm value to take into account that the current time
		slice is already partially complete. */
		ulAlarmValue -= ast_read_counter_value( AST );
		ast_write_alarm0_value( AST, ulAlarmValue );

		/* Restart the AST. */
		prvEnableAST();

		/* Allow the application to define some pre-sleep processing. */
		xModifiableIdleTime = xExpectedIdleTime;
		configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

		/* xExpectedIdleTime being set to 0 by configPRE_SLEEP_PROCESSING()
		means the application defined code has already executed the WAIT
		instruction. */
		if( xModifiableIdleTime > 0 )
		{
			/* Find the deepest allowable sleep mode. */
			xSleepMode = sleepmgr_get_sleep_mode();

			if( xSleepMode != SLEEPMGR_ACTIVE )
			{
				/* Sleep until something happens. */
				bpm_sleep( BPM, xSleepMode );
			}
		}

		/* Allow the application to define some post sleep processing. */
		configPOST_SLEEP_PROCESSING( xModifiableIdleTime );

		/* Stop AST.  Again, the time the SysTick is stopped for is	accounted
		for as best it can be, but using the tickless mode will	inevitably
		result in some tiny drift of the time maintained by the	kernel with
		respect to calendar time. */
		prvDisableAST();

		/* Re-enable interrupts - see comments above the cpsid instruction()
		above. */
		__asm volatile( "cpsie i" );

		if( ulTickFlag != pdFALSE )
		{
			/* The tick interrupt has already executed, although because this
			function is called with the scheduler suspended the actual tick
			processing will not occur until after this function has exited.
			Reset the alarm value with whatever remains of this tick period. */
			ulAlarmValue = ulAlarmValueForOneTick - ast_read_counter_value( AST );
			ast_write_alarm0_value( AST, ulAlarmValue );

			/* The tick interrupt handler will already have pended the tick
			processing in the kernel.  As the pending tick will be processed as
			soon as this function exits, the tick value	maintained by the tick
			is stepped forward by one less than the	time spent sleeping.  The
			actual stepping of the tick appears later in this function. */
			ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
		}
		else
		{
			/* Something other than the tick interrupt ended the sleep.  How
			many complete tick periods passed while the processor was
			sleeping? */
			ulCompleteTickPeriods = ast_read_counter_value( AST ) / ulAlarmValueForOneTick;

			/* The alarm value is set to whatever fraction of a single tick
			period remains. */
			ulAlarmValue = ast_read_counter_value( AST ) - ( ulCompleteTickPeriods * ulAlarmValueForOneTick );
			ast_write_alarm0_value( AST, ulAlarmValue );
		}

		/* Restart the AST so it runs up to the alarm value.  The alarm value
		will get set to the value required to generate exactly one tick period
		the next time the AST interrupt executes. */
		prvEnableAST();

		/* Wind the tick forward by the number of tick periods that the CPU
		remained in a low power state. */
		vTaskStepTick( ulCompleteTickPeriods );
	}
}


#endif /* configCREATE_LOW_POWER_DEMO == 1 */

