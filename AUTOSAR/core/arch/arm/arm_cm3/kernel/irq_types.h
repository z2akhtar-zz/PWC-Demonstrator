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

#ifndef IRQ_TYPES_H
#define IRQ_TYPES_H

#include "stm32f10x.h"

typedef IRQn_Type IrqType;

/* Offset from start of exceptions to interrupts
 * Exceptions have negative offsets while interrupts have positive
 */
#define IRQ_INTERRUPT_OFFSET  16

#define Irq_SOI()

/* Total number of interrupts and exceptions
 */

#if   defined(STM32F10X_LD) || defined(STM32F10X_MD)
#define NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS (USBWakeUp_IRQn+IRQ_INTERRUPT_OFFSET)
#elif defined(STM32F10X_HD)
#define NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS (DMA2_Channel4_5_IRQn+IRQ_INTERRUPT_OFFSET)
#elif defined(STM32F10X_CL)
#define NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS (OTG_FS_IRQn+IRQ_INTERRUPT_OFFSET)
#else
#error No device selected
#endif


typedef enum {
	  PERIPHERAL_CLOCK_AHB,
	  PERIPHERAL_CLOCK_APB1,
	  PERIPHERAL_CLOCK_APB2,
} Mcu_Arc_PeriperalClock_t;


typedef enum {
	CPU_0=0,
} Cpu_t;

#endif /* IRQ_H_ */
