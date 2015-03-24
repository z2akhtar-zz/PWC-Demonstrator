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
#include "Cpu.h"
#include "arch_stack.h"
#include "stm32f10x.h"


/**
 * Function make sure that we switch to supervisor mode(rfi) before
 * we call a task for the first time.
 */

void Os_ArchFirstCall( void )
{
	// IMPROVEMENT: make switch here... for now just call func.
	Irq_Enable();
	OS_SYS_PTR->currTaskPtr->constPtr->entry();
}

void *Os_ArchGetStackPtr( void ) {

	return (void *)__get_MSP();
}

unsigned int Os_ArchGetScSize( void ) {
	return SC_SIZE;
}

void Os_ArchSetTaskEntry(OsTaskVarType *pcbPtr ) {
	// IMPROVEMENT: Add lots of things here, see ppc55xx
	uint32_t *context = (uint32_t *)pcbPtr->stack.curr;

	context[C_CONTEXT_OFFS/4] = SC_PATTERN;

	/* Set LR to start function */
	if( pcbPtr->constPtr->proc_type == PROC_EXTENDED ) {
		context[VGPR_LR_OFF/4] = (uint32_t)Os_TaskStartExtended;
	} else if( pcbPtr->constPtr->proc_type == PROC_BASIC ) {
		context[VGPR_LR_OFF/4] = (uint32_t)Os_TaskStartBasic;
	}

}

void Os_ArchSetupContext( OsTaskVarType *pcb ) {
	// IMPROVEMENT: Add lots of things here, see ppc55xx
	// uint32_t *context = (uint32_t *)pcb->stack.curr;

}

void Os_ArchInit( void ) {
	// nothing to do here, yet :)
}

#if 0
CoreIDType GetCoreID(void)
{
    return 0; // Multicore not supported, just return master core id
}
#endif
