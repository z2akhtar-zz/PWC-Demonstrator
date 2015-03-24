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

/*
 * DESCRPTION:
 *   Timer for most PPC chips (Not MPC560x)
 */

/* ----------------------------[includes]------------------------------------*/

#include "timer.h"
//#include "Cpu.h"
#if !defined(CFG_SIMULATOR)
#include "Mcu.h"
#endif

#include "zynq.h"


/* ----------------------------[private define]------------------------------*/


/* ----------------------------[private macro]-------------------------------*/
/* ----------------------------[private typedef]-----------------------------*/
/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/
/* ----------------------------[private functions]---------------------------*/
/* ----------------------------[public functions]----------------------------*/


uint32_t Timer_Freq;

/**
 * Initialize the TB
 */
void Timer_Init( void ) {

	GLOBAL_TIMER.control = 0x1;		/* Enable the timer */

	/* Calculate Timer_Freq */
	Timer_Freq = MCU_ARC_CLOCK_ARM_CPU_3X2X_FREQUENCY;
}

uint32_t Timer_GetTicks( void ) {
	return GLOBAL_TIMER.counterLow;
}

uint64_t Timer_GetTicks64( void ) {
	uint32_t low;
	uint32_t high;

	do  {
		high =  GLOBAL_TIMER.counterHigh;
		low  =  GLOBAL_TIMER.counterLow;
	} while(  GLOBAL_TIMER.counterHigh != high );

	return ((((uint64_t)high)<<32)+low);
}
