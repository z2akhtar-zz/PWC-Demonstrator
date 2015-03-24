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
 * DESCRIPTION:
 *  Re-uses the RTI free running timer setup by the OS
 */


/* ----------------------------[includes]------------------------------------*/

#include "timer.h"
#include "Cpu.h"
#include "Mcu.h"
#include "core_cr4.h"

/* ----------------------------[private define]------------------------------*/

#define RTCC_CNTEN				(1<<(31-0))
#define RTCC_FRZEN				(1<<(31-2))
#define RTCC_CLKSEL_SXOSC		(0<<(31-19))
#define RTCC_CLKSEL_SIRC		(1<<(31-19))
#define RTCC_CLKSEL_FIRC		(2<<(31-19))
#define RTCC_CLKSEL_FXOSC		(3<<(31-19))

#define RTCC_DIV32EN			(1<<(21-31))

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

	/* The RTI module uses RTICLK that uses the same ratio as VCLK_sys, that is
     * VCLK_sys frequency is HCLK divided by 2 */
	rtiREG1->GCTRL  = 0x3;

	Timer_Freq = Mcu_Arc_GetSystemClock()/2;
}

uint32_t Timer_GetTicks( void ) {
	(void)rtiREG1->CNT[1].FRCx; 	/* dummy read to be able to read UCx */
	return rtiREG1->CNT[1].UCx;
}

