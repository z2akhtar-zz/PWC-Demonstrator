/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Contact: <contact@arccore.com>
 * 
 * You may ONLY use this file:
 * 1)if you have a valid commercial ArcCore license and then in accordance with  
 * the terms contained in the written license agreement between you and ArcCore, 
 * or alternatively
 * 2)if you follow the terms found in GNU General Public License version 2 as 
 * published by the Free Software Foundation and appearing in the file 
 * LICENSE.GPL included in the packaging of this file or here 
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
 *-------------------------------- Arctic Core -----------------------------*/

#include "Os.h"
#include "os_i.h"
#include "core_cr4.h"
#include "irq_types.h"
#include "isr.h"
#include "arc.h"
#include "os_counter_i.h"

#define RTICLK_PRESCALER 10

void CortexR4OsTick() {
	/** Clear all pending interrupts
	 *  otherwise this will hit again
	 *  as soon as interrupts are re-enabled. */
	rtiREG1->INTFLAG = 0x1;

	// Call regular OsTick.
	OsTick();
}

/**
 * Init of free running timer.
 */
void Os_SysTickInit( void ) {
	ISR_INSTALL_ISR2("OsTick",CortexR4OsTick,RTI_COMPARE_0,6,0);
}


static inline uint32_t SysTick_Config(uint32_t ticks)
{

	/** - Setup NTU source, debug options and disable both counter blocks */
	rtiREG1->GCTRL = 0x0;

	/** - Setup timebase for free running counter 0 */
	rtiREG1->TBCTRL = 0x0;

	/** - Enable/Disable capture event sources for both counter blocks */
	rtiREG1->CAPCTRL = 0x0;

	/** - Setup input source compare 0-3 */
	rtiREG1->COMPCTRL = 0x0;

	/** - Reset up counter 0 */
	rtiREG1->CNT[0U].UCx = 0x00000000U;

	/** - Reset free running counter 0 */
	rtiREG1->CNT[0U].FRCx = 0x00000000U;

	/** - Setup up counter 0 compare value
	 *  The RTI module is driven by RTICLK. With the Arctic Core
	 *  MCU driver for cortex R4 RTICLK = VCLK = PLLCLK / 2
	 *  ticks = number of PLLCLK cycles per os tick = number of RTICLK cycles * 2 per os tick
	 *
	 *     - 0x00000000: Divide by 2^32
	 *     - 0x00000001-0xFFFFFFFF: Divide by (CPUCx + 1)
	 */
	rtiREG1->CNT[0U].CPUCx = RTICLK_PRESCALER - 1;

	uint8 vclkDiv = ((systemREG1->CLKCNTL>>16) & 0xf)  + 1;   // VCLKR

	/** - Setup compare 0 value. This value is compared with selected free running counter. */
	rtiREG1->CMP[0U].COMPx = ticks / (RTICLK_PRESCALER * vclkDiv);

	/** - Setup update compare 0 value. This value is added to the compare 0 value on each compare match. */
	rtiREG1->CMP[0U].UDCPx = ticks / (RTICLK_PRESCALER * vclkDiv);

	/** - Clear all pending interrupts */
	rtiREG1->INTFLAG = 0x0;

	/** - Disable all interrupts */
	rtiREG1->CLEARINT = 0x0;

	return (0);
}

/**
 * Start the Sys Tick timer
 *
 * @param period_ticks How long the period in timer ticks should be.
 *
 */
void Os_SysTickStart(uint32_t period_ticks) {

	/* Cortex-M3 have a 24-bit system timer that counts down
	 * from the reload value to zero.
	 */

	SysTick_Config(period_ticks);
	rtiREG1->GCTRL  = 0x1;
	rtiREG1->SETINT = 0x1;

}
