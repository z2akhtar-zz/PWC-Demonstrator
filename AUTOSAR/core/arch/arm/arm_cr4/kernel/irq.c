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
#include "isr.h"
#include "irq_types.h"
#include "core_cr4.h"

extern TaskType Os_Arc_CreateIsr( void (*entry)(void ), uint8_t prio, const char *name );
extern void *Irq_VectorTable[NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS];


static inline void Irq_Setup() {
	vimREG->FIRQPR0 = 0x0;
	vimREG->FIRQPR1 = 0x0;
}

void Irq_Init( void ) {
	Irq_Setup();
	//Irq_Enable();
}

#define MASK_L		0
#define MASK_H		1

uint32_t prioMask[32][4];	/* 128 */
uint16_t Irq_StackPrio[16] ;
uint16_t Irq_Index =  0;


/**
 *
 * Interrupt channels can be found in the datasheet (not RM) under
 * "Interrupt Request Assignments", e.g. http://www.ti.com/lit/ds/spns192a/spns192a.pdf
 *
 * @param stack_p Ptr to the current stack.
 */
void *Irq_Entry( void *stack_p ) {
	uint16_t channel = vimREG->IRQINDEX;

	if( channel == 0 ) {
		/* No Pending Interrupt */
		return stack_p;
	}

	/* Adjust to real channel */
	channel -= 1;

	/* Check for software interrupt */
	if( channel == SSI ) {
		/* First get what software register caused the interrupt */
		uint32 reg = systemREG1->SSIVEC;		/* also clears SSI_FLAG */
		if( (reg & 0xff) == 0 ) {
			/* Nothing pending....*/
			return stack_p;
		}

		channel = (reg >> 8) & 0xff;
	}

//	Irq_Index++;

	Os_Isr(stack_p, channel );

	return stack_p;
}


/**
 *
 * @param priority The priority of the ISR we are about to start
 */
void Irq_SOI3( uint8_t priority ) {
	/* Disable lower prio interrupts */
	Irq_StackPrio[++Irq_Index] = priority; 	/* save prio */

	/* It seems that we need to clear the enable of the interrupt here
	 * because not doing it and enabling interrupts will cause a Data Exception
	 * In the same time we must also set the priority mask according to the
	 * priority of this ISR.
	 * */

	vimREG->REQENACLR0 = (vimREG->REQENACLR0 ^ prioMask[priority][0]);
	vimREG->REQENACLR1 = (vimREG->REQENACLR1 ^ prioMask[priority][1]);
	vimREG->REQENACLR2 = (vimREG->REQENACLR2 ^ prioMask[priority][2]);
	vimREG->REQENACLR3 = (vimREG->REQENACLR3 ^ prioMask[priority][3]);
}


void Irq_EOI( void  ) {
	/* Enable higher prio interrupts (more interrupts )  */
	--Irq_Index;
	vimREG->REQENASET0 = prioMask[Irq_StackPrio[Irq_Index]][0];
	vimREG->REQENASET1 = prioMask[Irq_StackPrio[Irq_Index]][1];
	vimREG->REQENASET2 = prioMask[Irq_StackPrio[Irq_Index]][2];
	vimREG->REQENASET3 = prioMask[Irq_StackPrio[Irq_Index]][3];
}

/**
 * Attach an ISR type 1 to the interrupt controller.
 *
 * @param entry
 * @param int_ctrl
 * @param vector
 * @param prio
 */
void Irq_AttachIsr1( void (*entry)(void), void *int_ctrl, uint32_t vector, uint8_t prio) {

	// IMPROVEMENT: Use NVIC_InitVector(vector, osPrioToCpuPio(pcb->prio)); here
}

/**
 * NVIC prio have priority 0-15, 0-highest priority.
 * Autosar does it the other way around, 0-Lowest priority
 *
 * Autosar    NVIC
 *   31        0
 *   30        0
 *   ..
 *   0         15
 *   0         15
 * @param prio
 * @return
 */
static inline int osPrioToCpuPio( uint8_t prio ) {
	assert(prio<32);
	prio = 31 - prio;
	return (prio>>1);
}

void Irq_EnableVector( int16_t vector, int priority, int core ) {

	if (vector < NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS) {

		/* priority is from 0 to 31, 0 = lowest, 31-highest - priority
		 * The mask should be set to on all lower prio's that this one
		 * */

		for(int i=0; i < priority ; i++  ) {
			prioMask[i][vector/32] |= (1 << (vector%32));

#if 0
			if (vector > 31) {
				prioMask[i][1] |= (1 << (vector - 32));
			} else {
				prioMask[i][0] |= (1 << vector);
			}
#endif
		}

		/* Assume this is done in non-interrupt context, set to lowest prio */
		vimREG->REQENASET0 = prioMask[0][0];
		vimREG->REQENASET1 = prioMask[0][1];
		vimREG->REQENASET2 = prioMask[0][2];
		vimREG->REQENASET3 = prioMask[0][3];

	} else {
		/* Invalid vector! */
		assert(0);
	}
}

void irqDummy( void ) {

}

/**
 * Generates a soft interrupt, ie sets pending bit.
 * This could also be implemented using ISPR regs.
 *
 * @param vector
 */
void Irq_GenerateSoftInt( IrqType vector ) {

	/* Install the software interrupt..note that the irqDummy will never be called */
	ISR_INSTALL_ISR1(  "Soft", irqDummy , SSI ,     31 /*prio*/, 0 /* app */);
	systemREG1->SSIR1 = (0x75 << 8) | vector;
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


