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

/* ----------------------------[information]----------------------------------*/
/*
 * Author: mahi
 *
 * Description:
 *
 */


/* ----------------------------[includes]------------------------------------*/


#include "Std_Types.h"
#include "Mcu.h"
#include "io.h"
#include "mpc55xx.h"
#include "Mcu_Arc.h"
#if defined(USE_FEE)
#include "Fee_Memory_Cfg.h"
#endif
#if defined(USE_DMA)
#include "Dma.h"
#endif
#include "asm_ppc.h"
#include "Os.h"
#include "arc.h"

/* ----------------------------[private define]------------------------------*/

#if defined(CFG_MPC5516)
    /* since the MCM module contains the registers normally located in ECSM */
    #define ECSM        MCM
#endif

/* ----------------------------[private macro]-------------------------------*/



/* ----------------------------[private typedef]-----------------------------*/
/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/

#if defined(CFG_MPC5XXX_TEST)
uint32_t Mpc5xxx_vectorMask;
uint8_t Mpc5xxx_Esr;
uint8_t Mpc5xxx_Intc_Esr;
#endif

#if defined(USE_FEE) && defined(USE_FLS)
extern uint8 EccError;
#endif

/* ----------------------------[private functions]---------------------------*/


/**
 * Function called by exception handlers when there is a fatal error.
 * Will also call ShutdownOs(E_OS_PANIC).
 *
 * @param error		The exception that caused the error
 * @param pData		Pointer to private data that may want to be dumped.
 */
void Mpc5xxx_Panic( uint32 error, void *pData ) {

	(void)error;
	(void)pData; /*lint !e920 Argument not used  */

	Os_Arc_Panic(error, pData );
}

/**
 * Function that checks for ECC errors
 *
 * @return
 */
static uint32_t checkEcc(void) {
	uint32_t rv = EXC_NOT_HANDLED;
#if (defined(USE_FEE) && defined(USE_FLS)) || defined(CFG_MPC5XXX_TEST)
	boolean flashEccError = FALSE;
	uint32 address = 0;
#if defined(CFG_MPC5744P) || defined(CFG_MPC5777M)
    static const typeof(MEMU.ERR_FLAG) flashError = {.B.F_UCE = 1};
	if(MEMU.ERR_FLAG.R == flashError.R) {
	    flashEccError = TRUE;
        address = MEMU.FLASH_UNCERR_ADDR.R;
        MEMU.FLASH_UNCERR_STS.B.VLD = 0;
        static const typeof(MEMU.ERR_FLAG) clear = {.B.F_UCE = 1, .B.F_UCO = 1};
	    MEMU.ERR_FLAG.R = clear.R;
	}
#if defined(CFG_MPC5XXX_TEST)
	boolean ramEccError = FALSE;
    static const typeof(MEMU.ERR_FLAG) ramError = {.B.SR_UCE = 1};
    if(MEMU.ERR_FLAG.R == ramError.R) {
        ramEccError = TRUE;
        address = MEMU.SYS_RAM_UNCERR_ADDR.R;
        MEMU.SYS_RAM_UNCERR_STS.B.VLD = 0;
        static const typeof(MEMU.ERR_FLAG) clear = {.B.SR_UCE = 1, .B.SR_UCO = 1};
        MEMU.ERR_FLAG.R = clear.R;
	}
#endif
#else
    /*lint -save -e10 -e40 -e24 -e63 */
	typeof(ECSM.ESR) esr = ECSM.ESR;
	if(esr.B.FNCE) {
        flashEccError = TRUE;
        typeof(ECSM.ESR.B) clear = {.FNCE = 1};
        address = ECSM.FEAR.R;
        /* Clear the exception */
        ECSM.ESR.B = clear;
	}
#if defined(CFG_MPC5XXX_TEST)
    boolean ramEccError = FALSE;
	if(esr.B.RNCE) {
        ramEccError = TRUE;
        typeof(ECSM.ESR.B) clear = {.RNCE = 1};
        address = ECSM.FEAR.R;
        /* Clear the exception */
        ECSM.ESR.B = clear;
    }
#endif
	/*lint -restore */
#endif

#if defined(USE_FEE) && defined(USE_FLS)
	/* Find FLS errors */
	if (flashEccError) {
		/* Check if we are in FEE range */
		if ( ((FEE_BANK1_OFFSET <= address) &&
						(FEE_BANK1_OFFSET + FEE_BANK1_LENGTH > address)) ||
				((FEE_BANK2_OFFSET <= address) &&
						(FEE_BANK2_OFFSET + FEE_BANK2_LENGTH > address)) )
		{
            /* Record that something bad has happened */
            EccError = 1;
			rv = EXC_HANDLED | EXC_ADJUST_ADDR;
		}
	}
#endif	 /* USE_FLS && USE_FEE */

#if defined(CFG_MPC5XXX_TEST)
	if(ramEccError) {
		/* ECC RAM problems */
		rv = (EXC_HANDLED | EXC_ADJUST_ADDR);
	}
    if (flashEccError) {
		rv = (EXC_HANDLED | EXC_ADJUST_ADDR);
	}
#endif
#endif
	return rv;
}


/* ----------------------------[public functions]----------------------------*/

/**
 * Called by lower level exception handlers and tries to handle
 * the exception.
 *
 * @param exceptionVector The exception vector.
 * @return
 */
uint32_t Mpc5xxx_ExceptionHandler(uint32_t exceptionVector) {
	uint32_t rv = EXC_NOT_HANDLED;

#if defined(CFG_MPC5XXX_TEST)
	Mpc5xxx_vectorMask |= (1<<exceptionVector);
#endif

	switch (exceptionVector) {
	case 1:
	{
#if defined(CFG_MPC5744P)
        // readout the FCCU errors
        // unlock
        FCCU.TRANS_LOCK.R = 0xBC;
        FCCU.CTRL./*B.OPR*/R = 10;    // set mode to CONFIG
        while(FCCU.CTRL.B.OPS == 1) {}
        volatile uint32 FCCU_NCF_S0 = FCCU.NCF_S[0].R;
        volatile uint32 FCCU_NCF_S1 = FCCU.NCF_S[1].R;
        volatile uint32 FCCU_NCF_S2 = FCCU.NCF_S[2].R;
        FCCU.CTRLK.R = 0x825A132BUL;
        FCCU.CTRL./*B.OPR*/R = 2;    // set mode to NORMAL
#endif
		/* CSRR0, CSRR1, MCSR */
		/* ECC: MSR[EE] = 0 */
#if defined(CFG_MPC5XXX_TEST)
	    uint32_t mcsr = get_spr(SPR_MCSR);
		if( get_spr(SPR_MCSR) & ( MCSR_BUS_DRERR | MCSR_BUS_WRERR )) {
			/* We have a bus error */
			rv = EXC_HANDLED | EXC_ADJUST_ADDR;
		    set_spr(SPR_MCSR,mcsr);    
			break;
		}
#endif
		rv = checkEcc();
		break;
	}
	case 2:
		/* SRR0, SRR1, ESR, DEAR */
		/* ECC: MSR[EE] = 1 */
#if defined(CFG_MPC5XXX_TEST)
		if( get_spr(SPR_ESR) &  ESR_XTE) {
			/* We have a external termination bus error */
			rv = EXC_HANDLED | EXC_ADJUST_ADDR;
			break;
		}
#endif
		rv = checkEcc();
		break;
	case 3:
	{
		/* SRR0, SRR1, ESR */
		rv = checkEcc();
		break;
	}

	default:
		break;
	}

	return rv;
}


