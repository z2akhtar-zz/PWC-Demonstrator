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

#include "os_i.h"
#include "stm32f10x.h"
#include "isr.h"
#include "irq_types.h"

extern void *Irq_VectorTable[NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS];

/**
 * Init NVIC vector. We do not use subpriority
 *
 * @param vector	The IRQ number
 * @param prio      NVIC priority, 0-31, 0-high prio
 */
static void NVIC_InitVector(IRQn_Type vector, uint32_t prio)
{
	// Set prio
	NVIC_SetPriority(vector,prio);

	// Enable
    NVIC->ISER[vector >> 5] = (uint32_t)1 << (vector & (uint8_t)0x1F);
}

/*
PRIGROUP
0 			7.1 indicates seven bits of pre-emption priority, one bit of subpriority
1 			6.2 indicates six bits of pre-emption priority, two bits of subpriority
2 			5.3 indicates five bits of pre-emption priority, three bits of subpriority
3 			4.4 indicates four bits of pre-emption priority, four bits of subpriority
4 			3.5 indicates three bits of pre-emption priority, five bits of subpriority
5 			2.6 indicates two bits of pre-emption priority, six bits of subpriority
6 			1.7 indicates one bit of pre-emption priority, seven bits of subpriority
7 			0.8 indicates no pre-emption priority, eight bits of subpriority.
*/
void Irq_Init( void ) {
	NVIC_SetPriorityGrouping(0);
	NVIC_SetPriority(SVCall_IRQn, 0xff); // Set lowest prio
	NVIC_SetPriority(PendSV_IRQn, 0xff); // Set lowest prio

	/* Stop counters and watchdogs when halting in debug */
	DBGMCU->CR |= 0x00ffffff00;
}

void Irq_EOI( void ) {
	/* Note!
	 * This is not applicable on the Cortex-M3 since we
	 * can't terminate the interrupt request without popping
	 * back registers..have to be solved in the context switches
	 * themselves.
	 */
}

#define ICSR_VECTACTIVE		0x1ff


/**
 * NVIC prio have priority 0-31, 0-highest priority.
 * Autosar does it the other way around, 0-Lowest priority
 * NOTE: prio 255 is reserved for SVC and PendSV
 *
 * Autosar    NVIC
 *   31        0
 *   30        1
 *   ..
 *   0         31
 * @param prio
 * @return
 */
static inline int osPrioToCpuPio( uint8_t prio ) {
	assert(prio<32);
	prio = 31 - prio;
	return prio;
}


void Irq_EnableVector( int16_t vector, int priority, int core ) {
	(void)core;
	NVIC_InitVector(vector, osPrioToCpuPio(priority));
}

/**
 * Generates a soft interrupt, ie sets pending bit.
 * This could also be implemented using ISPR regs.
 *
 * @param vector
 */
void Irq_GenerateSoftInt( IrqType vector ) {

	NVIC->STIR = (vector);
}

/**
 * Get the current priority from the interrupt controller.
 * @param cpu
 * @return
 */
uint8_t Irq_GetCurrentPriority( Cpu_t cpu) {

	uint8_t prio = 0;

	// SCB_ICSR contains the active vector
	return prio;
}

typedef struct {
	uint32_t dummy;
} exc_stack_t;


