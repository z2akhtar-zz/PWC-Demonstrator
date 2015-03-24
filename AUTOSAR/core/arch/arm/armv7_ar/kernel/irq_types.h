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

#include "irq_zynq.h"

#if 0


#define IrqGetCurrentInterruptSource() \
	(((volatile sint8)vimREG->IRQIVEC) - 1)

/** IrqActivateChannel turns the selected channel on in the VIM */
#define IrqActivateChannel(_c) \
	uint8 _ch1 = _c; \
	if (_ch1 > 31) { \
		vimREG->REQMASKSET1 |= (1 << (_ch1 - 32)); \
	} else { \
		vimREG->REQMASKSET0 |= (1 << _ch1); \
	}


/** IrqDeactivateChannel turns the selected channel off in the VIM */
#define IrqDeactivateChannel(_c) \
	uint8 _ch2 = _c; \
	if (_ch2 > 31) { \
		vimREG->REQMASKCLR1 = (1 << (_ch2 - 32)); \
	} else { \
		vimREG->REQMASKCLR0 = (1 << _ch2); \
	}


#define Irq_SOI() \
	IrqDeactivateChannel(isrPtr->activeVector)


#define Irq_EOI() \
	IrqActivateChannel(isrPtr->activeVector)


typedef enum {
  ESM_LEVEL_0 = 0,
  RESERVED = 1,
  RTI_COMPARE_0,
  RTI_COMPARE_1,
  RTI_COMPARE_2,
  RTI_COMPARE_3,
  RTI_OVERFLOW_0,
  RTI_OVERFLOW_1,
  RTI_TIMEBASE,
  DIO_LEVEL_0,
  HET_LEVEL_0,
  HET_TU_LEVEL_0,
  MIBSP1_LEVEL_0,
  LIN1_LEVEL_0,
  MIBADC1_EVENT,
  MIBADC1_GROUP_1,
  CAN1_LEVEL_0,
  RESERVED_0,
  FR_LEVEL_0,
  CRC_1,
  ESM_LEVEL_1,
  SSI,
  PMU,
  DIO_LEVEL_1,
  HET_LEVEL_1,
  HET_TU_LEVEL_1,
  MIBSP1_LEVEL_1,
  LIN1_LEVEL_1,
  MIBADC1_GROUP_2,
  CAN1_LEVEL_1,
  RESERVED_1,
  MIBADC1_MAG,
  FR_LEVEL_1,
  DMA_FTCA,
  DMA_LFSA,
  CAN2_LEVEL_0,
  DMM_LEVEL_0,
  MIBSPI3_LEVEL_0,
  MIBSPI3_LEVEL_1,
  DMA_HBDC,
  DMA_BTCA,
  RESERVED_2,
  CAN2_LEVEL_1,
  DMM_LEVEL_1,
  CAN1_IF_3,
  CAN3_LEVEL_0,
  CAN2_IF_3,
  FPU,
  FR_TU_STATUS,
  LIN2_LEVEL_0,
  MIBADC2_EVENT,
  MIBADC2_GROUP_1,
  FR_TOC,
  MIBSPIP5_LEVEL_0,
  LIN2_LEVEL_1,
  CAN3_LEVEL_1,
  MIBSPI5_LEVEL_1,
  MIBADC2_GROUP_2,
  FR_TU_ERROR,
  MIBADC2_MAG,
  CAN3_IF_3,
  FR_TU_MPU,
  FR_T1C,
  NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS,
} IrqType;

/* Total number of interrupts and exceptions
 */
#define NUMBER_OF_INTERRUPTS_AND_EXCEPTIONS 65

/* Offset from start of exceptions to interrupts
 * Exceptions have negative offsets while interrupts have positive
 */
#define IRQ_INTERRUPT_OFFSET 0

typedef enum {
	  PERIPHERAL_CLOCK_AHB,
	  PERIPHERAL_CLOCK_APB1,
	  PERIPHERAL_CLOCK_APB2,
	  PERIPHERAL_CLOCK_CAN,
	  PERIPHERAL_CLOCK_DCAN1,
	  PERIPHERAL_CLOCK_DCAN2,
	  PERIPHERAL_CLOCK_DCAN3
} Mcu_Arc_PeriperalClock_t;

typedef enum {
	CPU_0=0,
} Cpu_t;

#endif


#endif /* IRQ_H_ */
