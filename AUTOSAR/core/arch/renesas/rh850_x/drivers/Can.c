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

/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */
/** @tagSettings DEFAULT_ARCHITECTURE=RH850F1H */

/* Code IMPROVEMENT
 * - REMOVE1
 * - REMOVE2
 */

/* ----------------------------[information]----------------------------------*/
/*
 * Author: mast
 *
 *
 * Description:
 *   Implements the Can Driver module
 *
 * Support:
 *   General                  Have Support
 *   -------------------------------------------
 *   CAN_DEV_ERROR_DETECT            Y
 *   CAN_HW_TRANSMIT_CANCELLATION    Y
 *   CAN_IDENTICAL_ID_CANCELLATION   N
 *   CAN_INDEX                       N
 *   CAN_MULTIPLEXED_TRANSMISSION    N
 *   CAN_TIMEOUT_DURATION            N
 *   CAN_VERSION_INFO_API            N
 *
 *   Controller                  Have Support
 *   -------------------------------------------
 *   CAN_BUSOFF_PROCESSING           N  , Interrupt only
 *   CAN_RX_PROCESSING               N  , Interrupt only
 *   CAN_TX_PROCESSING               N  , Interrupt only
 *   CAN_WAKEUP_PROCESSING           N  , Interrupt only
 *   CAN_CPU_CLOCK_REFERENCE         N  , *)
 *   CanWakeupSourceRef              N  , **)
 *
 *   *) It assumes that there is a PERIPHERAL clock defined.
 *   **) The RSCAN hardware cannot detect wakeup
 *
 *   Devices    CLK_SRC
 *   ----------------------------------------------
 *   RH850F1H   PPLLCLK2
 *
 * Implementation Notes:
 *   FIFO
 *   - Is always ON for RX.
 *     IMPROVEMENT:    There are "Common FIFOs" available for use.
 *
 *   CAN RAM:
 *     IMPROVEMENT: Add ECC support.
 *
 *   Clock Settings:
 *     IMPROVEMENT: Currently using hardcoded clock frequency values since
 *                  MCU module is not yet supporting proper API calls.
 *
 * Things left:
 *     IMPROVEMENT: Polled solution.
 *
 */
/*
 * General requirements
 * */
/** @req 4.1.2/SWS_Can_00012 *//* ISRs cannot be interrupted by themselves + overwrite detection */
/** @req 4.1.2/SWS_Can_00034 *//* Header file structure */
/** @req 4.1.2/SWS_Can_00237 *//* No transmit triggered by RTR */
/** @req 4.1.2/SWS_Can_00236 *//* No transmit triggered by RTR */
/** @req 4.1.2/SWS_Can_00238 *//* On-chip controller, does not use other drivers */
/** @req 4.1.2/SWS_Can_00401 *//* Multiple mbs per HTH */
/** @req 4.1.2/SWS_Can_00402 *//* Multiplexed transmission, possible to identify free mb */
/** @req 4.1.2/SWS_Can_00403 *//* Multiplexed transmission for device sending in priority (and we don't touch the LBUF bit which is 0 default ) */
/** !req 4.1.2/SWS_Can_00007 *//* Complete polling configurable */
/** @req 4.1.2/SWS_Can_00220 *//* Precompile variant */
/** @req 4.1.2/SWS_Can_00035 *//* No callbacks */
/** !req 4.1.2/SWS_Can_00257 *//* Sleep mode not supported */
/** !req 4.1.2/SWS_Can_00422 *//* Can_SetBaudrate not implemented */
/** @req 4.1.2/SWS_Can_00079 *//* Fulfills BSW guidelines when applicable */
/* ----------------------------[includes]------------------------------------*/
#include "Can.h"
#include <stdio.h>
#include "dr7f701503.dvf.h"
#include "irq_rh850f1h.h"
#include "CanRegs.h"
#include "Cpu.h"
#include "Mcu.h"
#include "CanIf_Cbk.h"
#if (CAN_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif
#if defined(USE_DEM)
#include "Dem.h"
#endif
#include <stdlib.h>
#include <string.h>
#include "Os.h"
#include "isr.h"
#include "irq.h"
#include "arc.h"
//#define USE_LDEBUG_PRINTF
#include "debug.h"

/*
 * Lint Exceptions For this file:
 * - Allow use of union based types (9018)
 */
/*lint -save -e9018*/

/* ----------------------------[private define]------------------------------*/
/* Different CAN structs depending on unit (st_rscan0, st_rscan1)
 * Compatible when it comes to address offsets for common register groups.
 * Use rscan0 struct as common access type */
typedef volatile struct st_rscan0 rscan_t;

#define RSCAN0_UNIT                   0u
#define RSCAN1_UNIT                   1u
#define RSCAN0_HWCH_MAX               6u
#define RSCAN1_HWCH_MAX               2u
#define AF_MAX_NUMPAGES              24u  /* Acceptance filter maximum number of pages */
#define AF_PAGE_SIZE                 16u  /* Acceptance filter page size */
#define TXBUFS_PER_CH                16uL

#define USE_CAN_STATISTICS           STD_OFF
#define USE_ONLY_RXFIFOS             STD_ON
#define USE_CAN_OSCILLATOR_CLOCK     STD_ON

#define INVALID_CANID                0xFFFFFFFFUL
#define EXTENDED_CANID_MASK          0x1FFFFFFFUL
#define STD_CANID_MASK               0x7FFUL

/* ----------------------------[private macro]-------------------------------*/

#define HWCHIDX_TO_CTRL(_rscan_unit,_hwChIdx) ((_rscan_unit)*RSCAN_CH6 + (_hwChIdx))
#define CTRL_TO_UNIT_PTR(_controller)   (&CanUnit[_controller])
#define VALID_CONTROLLER(_ctrl)         ((_ctrl) < CAN_ARC_CTRL_CONFIG_CNT)
#define GET_CALLBACKS()                 (Can_Global.config->CanConfigSetPtr->CanCallbacks)
#define GET_RSCAN_BASE(_rscan_unit)      /*lint -e923 -e9033*/((rscan_t *)(0xFFD00000uL + (0x8000uL*(uint32)(_rscan_unit))))/*lint +e923 +e9033 Ok for us to assign pointers to HW registers in this case */
#define GET_HWUNIT(_hwCh)               (((_hwCh)>RSCAN_CH5_e)?1:0)
#define GET_HWUNIT_BASE(_hwCh) 	        GET_RSCAN_BASE(GET_HWUNIT(_hwCh))
#define GET_HWCH_IDX(_hwCh)             ((((uint32)(_hwCh))>=((uint32)RSCAN_CH6_e))?(((uint32)(_hwCh))-((uint32)RSCAN_CH6)):((uint32)(_hwCh)))
#define GET_CxREGS_BASEPTR(_hwCh)       /*lint -save -e740 -e929 -e9005*/((CAN_CxREGS_t *)GET_HWUNIT_BASE(_hwCh))/*lint -restore Ok for us to cast pointer type to HW registers in this case */

#define BIT_SET(reg, bitpos)            /*lint -e632*/((reg) |=   (1uL << (bitpos)))/*lint +e632 Ok here since macro can be used for other than 32-bit types but strong type assignment complains since bit mask is stored as 32-bits */
#define BIT_CLR(reg, bitpos)            /*lint -e632*/((reg) &= (~(1uL << (bitpos))))/*lint +e632 Ok here since macro can be used for other than 32-bit types but strong type assignment complains since bit mask is stored as 32-bits */
#define BIT_CHK(reg, bitpos)            (((uint32)(reg) & (1uL << (bitpos))) == (1uL << (bitpos)))

#define INSTALL_HANDLER4(_name, _can_entry, _vector, _priority, _app)\
	do { \
		ISR_INSTALL_ISR2(_name, _can_entry, _vector+0, _priority, _app); \
		ISR_INSTALL_ISR2(_name, _can_entry, _vector+1, _priority, _app); \
		ISR_INSTALL_ISR2(_name, _can_entry, _vector+2, _priority, _app); \
		ISR_INSTALL_ISR2(_name, _can_entry, _vector+3, _priority, _app); \
	} while(0)

#define INSTALL_HANDLER16(_name, _can_entry, _vector, _priority, _app)\
	do { \
		INSTALL_HANDLER4(_name, _can_entry, _vector+0, _priority, _app); \
		INSTALL_HANDLER4(_name, _can_entry, _vector+4, _priority, _app); \
		INSTALL_HANDLER4(_name, _can_entry, _vector+8, _priority, _app); \
		INSTALL_HANDLER4(_name, _can_entry, _vector+12,_priority, _app); \
	} while(0)


 /*lint -save --e{9023,9024} Allow use of ## below */
#define _ADD_ISR_HANDLERS(_x,_y) \
        do { \
            if( ( _y &  (CAN_CTRL_BUSOFF_PROCESSING_INTERRUPT | CAN_CTRL_ERROR_PROCESSING_INTERRUPT) ) != 0 ) { \
                ISR_INSTALL_ISR2( "Can", Can_Ch ## _x ## _Err, IRQ_INTRCAN ## _x ## ERR, 2, 0 ); \
            } \
            if( ( _y &  CAN_CTRL_TX_PROCESSING_INTERRUPT ) != 0 ){ \
                ISR_INSTALL_ISR2( "Can", Can_Ch  ## _x ## _IsrTx, IRQ_INTRCAN ## _x ## TRX, 2, 0 ); \
            } \
        } while(FALSE)
/* Wrapper used for expansion of macro parameters... */
#define ADD_ISR_HANDLERS(_x,_y) _ADD_ISR_HANDLERS(_x,_y)
/*lint -restore */

//-------------------------------------------------------------------

#if ( CAN_DEV_ERROR_DETECT == STD_ON )
/** @req 4.0.3/CAN027 */
/** @req 4.1.2/SWS_Can_00091*/
/** @req 4.1.2/SWS_Can_00089*/
#define CAN_VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(CAN_MODULE_ID,0,_api,_err); \
          return CAN_NOT_OK; \
        }

#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(CAN_MODULE_ID,0,_api,_err); \
          return E_NOT_OK; \
        }

#define VALIDATE_NO_RV(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(CAN_MODULE_ID,0,_api,_err); \
          return; \
        }

#define DET_REPORTERROR(_x,_y,_z,_q) (void)Det_ReportError(_x, _y, _z, _q)
#else
/** @req 4.0.3/CAN424 */
#define CAN_VALIDATE(_exp,_api,_err )
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_NO_RV(_exp,_api,_err )
#define DET_REPORTERROR(_x,_y,_z,_q)
#endif

#if defined(USE_DEM)
#define VALIDATE_DEM(_exp,_err,_rv) \
        if( !(_exp) ) { \
          Dem_ReportErrorStatus(_err, DEM_EVENT_STATUS_FAILED); \
          return _rv; \
        }
#else
#define VALIDATE_DEM(_exp,_err,_rv )
#endif


/* ----------------------------[private typedef]-----------------------------*/

#if defined(CFG_CAN_TEST)
Can_TestType Can_Test;
#endif

typedef enum {
    CAN_UNINIT = 0, CAN_READY
} Can_DriverStateType;

typedef enum {
    CAN_CTRL_INDICATION_NONE,
    CAN_CTRL_INDICATION_PENDING_START,
    CAN_CTRL_INDICATION_PENDING_STOP,
    CAN_CTRL_INDICATION_PENDING_SLEEP /* Sleep is not implemented - but we need a logical sleep mode*/
} Can_CtrlPendingStateIndicationType;

/* Type for holding global information used by the driver */
typedef struct {
    Can_DriverStateType     initRun;            /* If Can_Init() have been run */
    const Can_ConfigType *  config;             /* Pointer to config */
} Can_GlobalType;


/* Type for holding information about each controller */
typedef struct {
    CanIf_ControllerModeType            state;
    const Can_ControllerConfigType *    cfgCtrlPtr;     /* Pointer to controller config  */
    const Can_HardwareObjectType *      cfgHohPtr;     /* List of HOHs */
    rscan_t *                           hwPtr;
    uint32                              hwChIdx;
    uint32                              lock_cnt;
    uint32                              Can_Arc_RxFifoMask; /* Dedicated RX FIFOs used by this controller */
    Can_CtrlPendingStateIndicationType  pendingStateIndication;
#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
    uint32                              mbTxCancel;          /* Bit position denotes TX buf 1..16 */
    uint32                              suppressMbTxCancel;  /* Bit position denotes TX buf 1..16 */
#endif
#if (USE_CAN_STATISTICS == STD_ON)
    Can_Arc_StatisticsType              stats;
#endif
} Can_UnitType;


/* Type for holding information about each RS-CAN HW unit */
typedef struct {
    uint32 numRecRules;
    uint32 recRulePageNum;
    uint32 numMbsUsed;
    uint32 numRxFifosUsed;

    struct {
        boolean inUse;
    } setupFlags;

} RSCanSetupInfo_t;

/* Global Mode (RSCAN global state controller) */
typedef enum {
    GLOBAL_MODE_STOP,
    GLOBAL_MODE_RESET,
    GLOBAL_MODE_OPER,
    GLOBAL_MODE_TEST
} Can_GlobalMode_t;

/* Channel Mode (RSCAN channel state controller) */
typedef enum {
    CHANNEL_MODE_STOP,
    CHANNEL_MODE_RESET,
    CHANNEL_MODE_HALT,
    CHANNEL_MODE_COMM
} Can_ChannelMode_t;

/* ----------------------------[private function prototypes]-----------------*/
static void Can_Global_RxFifo(uint8 rscan_unit);
static void Can_IsrChTx(Can_Arc_HwChIdType hwChId);
static void Can_IsrChErr(Can_Arc_HwChIdType hwChId);
static void handleBusOff(Can_UnitType *canUnit);

/* ----------------------------[private variables]---------------------------*/
Can_UnitType    CanUnit[CAN_ARC_CTRL_CONFIG_CNT];
Can_GlobalType  Can_Global = { .initRun = CAN_UNINIT, };/** @req 4.1.2/SWS_Can_00103 */
RSCanSetupInfo_t RSCanSetupInfo[NUM_RSCAN_UNITS];

/* ----------------------------[private functions]---------------------------*/
static Can_GlobalMode_t getRSCanGlobalMode(uint8 rscan_unit)
{
    CAN_GSTS_t gsts;

    /* Read global status reg */
    gsts.R = (vuint32_t)GET_RSCAN_BASE(rscan_unit)->GSTS.UINT32;

    if (gsts.B.GSLPSTS == 1) {
        return GLOBAL_MODE_STOP;
    }
    else if (gsts.B.GHLTSTS  == 1) {
        return GLOBAL_MODE_TEST;
    }
    else if (gsts.B.GRSTSTS == 1) {
        return GLOBAL_MODE_RESET;
    }
    else {
        return GLOBAL_MODE_OPER;
    }
}

static void setRSCanGlobalMode(uint8 rscan_unit, Can_GlobalMode_t mode)
{
    Can_GlobalMode_t curMode;
    CAN_GCTR_t *gctr;

    curMode = getRSCanGlobalMode(rscan_unit);
    gctr = (CAN_GCTR_t *)&GET_RSCAN_BASE(rscan_unit)->GCTR; /*lint !e929 Ok to cast pointer type to HW register in this case */

    switch(mode) {
        case GLOBAL_MODE_STOP:
            /* Can only enter stop mode when in reset mode */
            if ( GLOBAL_MODE_RESET == curMode ) {
                gctr->B.GSLPR = CAN_GSLPR_GLBL_STOP_MODE;
                while( GLOBAL_MODE_STOP != getRSCanGlobalMode(rscan_unit) ) {}
            }
            break;
        case GLOBAL_MODE_OPER:
            /* Can enter operating mode when in reset mode or test mode */
            if ( (GLOBAL_MODE_RESET == curMode) || (GLOBAL_MODE_TEST == curMode) ) {
                gctr->B.GMDC = CAN_GMDC_GLBL_OPR_MODE;
                while( GLOBAL_MODE_OPER != getRSCanGlobalMode(rscan_unit) ) {}
            }
            break;
        case GLOBAL_MODE_RESET:
            /* Can enter reset mode when in stop mode or... */
            if ( GLOBAL_MODE_STOP == curMode ) {
                gctr->B.GSLPR = 0;
                while( GLOBAL_MODE_RESET != getRSCanGlobalMode(rscan_unit) ) {}
            }
            /* ...when in test mode or operational mode */
            else if ( (GLOBAL_MODE_TEST == curMode) || (GLOBAL_MODE_OPER == curMode) ) {
                gctr->B.GMDC = CAN_GMDC_GLBL_RST_MODE;
                while( GLOBAL_MODE_RESET != getRSCanGlobalMode(rscan_unit) ) {}
            }
            else {
                /* Please Lint and MISRA 2012 Rule 15.7 with this empty else clause */
            }
            break;
        case GLOBAL_MODE_TEST:
            /* Can enter test mode when in reset mode or operating mode */
            if ( (GLOBAL_MODE_RESET == curMode) || (GLOBAL_MODE_OPER == curMode) ) {
                gctr->B.GMDC = CAN_GMDC_GLBL_TST_MODE;
                while( GLOBAL_MODE_TEST != getRSCanGlobalMode(rscan_unit) ) {}
            }
            break;
        default:
            /* Unexpected mode */
            DET_REPORTERROR(CAN_MODULE_ID, 0, CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
            break;
    }
}

static Can_ChannelMode_t getRSCanChannelMode(Can_Arc_HwChIdType hwChId)
{
    const CAN_CxREGS_t *cxreg = &GET_CxREGS_BASEPTR(hwChId)[GET_HWCH_IDX(hwChId)];

    if (cxreg->CxSTS.B.CSLPSTS == 1) {
        return CHANNEL_MODE_STOP;
    }
    else if (cxreg->CxSTS.B.CHLTSTS == 1) {
        return CHANNEL_MODE_HALT;
    }
    else if (cxreg->CxSTS.B.CRSTSTS == 1) {
        return CHANNEL_MODE_RESET;
    }
    else {
        return CHANNEL_MODE_COMM;
    }
}

static void setRSCanChannelMode(Can_Arc_HwChIdType hwChId, Can_ChannelMode_t mode)
{
    Can_ChannelMode_t curMode;
    CAN_CxREGS_t *cxreg;

    curMode = getRSCanChannelMode(hwChId);
    cxreg = &GET_CxREGS_BASEPTR(hwChId)[GET_HWCH_IDX(hwChId)];

    switch(mode) {
        case CHANNEL_MODE_STOP:
            /* Can only enter stop mode when in channel reset mode */
            if ( CHANNEL_MODE_RESET == curMode ) {
                cxreg->CxCTR.B.CSLPR = CAN_CSLPR_CH_STOP_MODE;
                while( CHANNEL_MODE_STOP != getRSCanChannelMode(hwChId) ) {}
            }
            break;
        case CHANNEL_MODE_HALT:
            if ( (CHANNEL_MODE_RESET == curMode) || (CHANNEL_MODE_COMM == curMode) ) {
                cxreg->CxCTR.B.CHMDC = CAN_CHMDC_CH_HLT_MODE;
                while( CHANNEL_MODE_HALT != getRSCanChannelMode(hwChId) ) {}
            }
            break;
        case CHANNEL_MODE_RESET:
            if ( CHANNEL_MODE_STOP == curMode ) {
                cxreg->CxCTR.B.CSLPR = 0;
                while( CHANNEL_MODE_RESET != getRSCanChannelMode(hwChId) ) {}
            }
            else if ( (CHANNEL_MODE_HALT == curMode) || (CHANNEL_MODE_COMM == curMode) ) {
                cxreg->CxCTR.B.CHMDC = CAN_CHMDC_CH_RST_MODE;
                while( CHANNEL_MODE_RESET != getRSCanChannelMode(hwChId) ) {}
            }
            else {
                /* Please Lint and MISRA 2012 Rule 15.7 with this empty else-clause */
            }
            break;
        case CHANNEL_MODE_COMM:
            if ( (CHANNEL_MODE_RESET == curMode) || (CHANNEL_MODE_HALT == curMode) ) {
                cxreg->CxCTR.B.CHMDC = CAN_CHMDC_CH_COM_MODE;
                while( CHANNEL_MODE_COMM != getRSCanChannelMode(hwChId) ) {}
            }
            break;
        default:
            /* Unexpected mode */
            DET_REPORTERROR(CAN_MODULE_ID, 0, CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
            break;
    }
}

static void rxIrqEnable(const Can_UnitType *canUnit)
{
    Can_GlobalMode_t mode = getRSCanGlobalMode(GET_HWUNIT(canUnit->cfgCtrlPtr->Can_Arc_HwChId));

    if ((GLOBAL_MODE_OPER == mode) || (GLOBAL_MODE_TEST == mode)) {
        /* Global RX FIFOs */
#if (USE_ONLY_RXFIFOS == STD_ON)
        /* Write settings to specific fifo reg */
        CAN_RFCCx_t *rfccPtr = &((CAN_RFCCx_t *)&canUnit->hwPtr->RFCC0)[canUnit->hwChIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
        rfccPtr->B.RFIE = 1;
        rfccPtr->B.RFE = 1;
#else
        /* Enable channel wise through FIFO ctrl reg */
        for (uint8 rxFifoIdx = 0; rxFifoIdx < 8; rxFifoIdx++) {
            if ( canUnit->Can_Arc_RxFifoMask & (1 << rxFifoIdx) ) {
                /* Write settings to specific fifo reg */
                CAN_RFCCx_t *rfccPtr = &((CAN_RFCCx_t *)&canUnit->hwPtr->RFCC0)[rxFifoIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
                rfccPtr->B.RFIE = 1;
                rfccPtr->B.RFE = 1;
            }
        }
#endif
    }

}

static void rxIrqDisable(const Can_UnitType *canUnit)
{
    Can_GlobalMode_t mode = getRSCanGlobalMode(GET_HWUNIT(canUnit->cfgCtrlPtr->Can_Arc_HwChId));

    if ((GLOBAL_MODE_OPER == mode) || (GLOBAL_MODE_TEST == mode)) {
        /* Global RX FIFOs */
#if (USE_ONLY_RXFIFOS == STD_ON)
        /* Write settings to specific fifo reg */
        CAN_RFCCx_t *rfccPtr = &((CAN_RFCCx_t *)&canUnit->hwPtr->RFCC0)[canUnit->hwChIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
        rfccPtr->B.RFE = 0;
        rfccPtr->B.RFIE = 0;
#else
        /* Disable channel wise through FIFO ctrl reg */
        for (uint8 rxFifoIdx = 0; rxFifoIdx < 8; rxFifoIdx++) {
            if ( canUnit->Can_Arc_RxFifoMask & (1 << rxFifoIdx) ) {
                /* Write settings to specific fifo reg */
                CAN_RFCCx_t *rfccPtr = &((CAN_RFCCx_t *)&canUnit->hwPtr->RFCC0)[rxFifoIdx];
                rfccPtr->B.RFE = 0;
                rfccPtr->B.RFIE = 0;
            }
        }
#endif
    }

}

static void txIrqEnable(const Can_UnitType *canUnit)
{
    Can_ChannelMode_t mode = getRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId);

    if ((CHANNEL_MODE_COMM == mode) || (CHANNEL_MODE_HALT == mode)) {
        CAN_THLCCx_t *thlccPtr;
        CAN_TXQCCx_t *txqccPtr;

        /* Enable Transmit History Buffers & IRQ */
        thlccPtr = &((CAN_THLCCx_t *)&canUnit->hwPtr->THLCC0)[canUnit->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */
        thlccPtr->B.THLIE = 1;
        thlccPtr->B.THLE = 1;

        /* Enable TX queue */
        txqccPtr = &((CAN_TXQCCx_t *)&canUnit->hwPtr->TXQCC0)[canUnit->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */
        txqccPtr->B.TXQE = 1;
    }
}

static void txIrqDisable(const Can_UnitType *canUnit)
{
    Can_ChannelMode_t mode = getRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId);

    if ((CHANNEL_MODE_COMM == mode) || (CHANNEL_MODE_HALT == mode)) {
        CAN_THLCCx_t *thlccPtr;
        CAN_TXQCCx_t *txqccPtr;

        /* Disable Transmit History Buffers */
        thlccPtr = &((CAN_THLCCx_t *)&canUnit->hwPtr->THLCC0)[canUnit->hwChIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
        thlccPtr->B.THLE = 0;
        thlccPtr->B.THLIE = 0;

        /* Disable TX queue & IRQs */
        txqccPtr = &((CAN_TXQCCx_t *)&canUnit->hwPtr->TXQCC0)[canUnit->hwChIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
        txqccPtr->B.TXQE = 0;
    }
}

static void initRSCanInfo(void)
{
    for (uint8 i = 0; i < NUM_RSCAN_UNITS; i++) {
        if (0 == i) {
            RSCanSetupInfo[i].setupFlags.inUse = RSCAN0_IN_USE;
        }
        else if(1 == i) {
            RSCanSetupInfo[i].setupFlags.inUse = RSCAN1_IN_USE;
        }
        else {
            /* Please Lint and MISRA 2012 Rule 15.7 with this empty else-clause */
        }
        RSCanSetupInfo[i].numRecRules = 0;
        RSCanSetupInfo[i].recRulePageNum = 0;
        RSCanSetupInfo[i].numMbsUsed = 0;
        RSCanSetupInfo[i].numRxFifosUsed = 0;
    }
}

/*lint --e{923,9048} Ok to let these through since remark is in macros for register access which are defined in Renesas header file */
static void setNumRecRulesForCh(Can_Arc_HwChIdType hwChId, uint8 numRecRules)
{
    switch (hwChId) {
        case RSCAN_CH0_e:
            RSCAN0GAFLCFG0HH = numRecRules;
            break;
        case RSCAN_CH1_e:
            RSCAN0GAFLCFG0HL = numRecRules;
            break;
        case RSCAN_CH2_e:
            RSCAN0GAFLCFG0LH = numRecRules;
            break;
        case RSCAN_CH3_e:
            RSCAN0GAFLCFG0LL = numRecRules;
            break;
        case RSCAN_CH4_e:
            RSCAN0GAFLCFG1HH = numRecRules;
            break;
        case RSCAN_CH5_e:
            RSCAN0GAFLCFG1HL = numRecRules;
            break;
        case RSCAN_CH6_e:
            RSCAN1GAFLCFG0HH = numRecRules;
            break;
        case RSCAN_CH7_e:
            RSCAN1GAFLCFG0HL = numRecRules;
            break;
        default:
            /* Unexpected channel */
            DET_REPORTERROR(CAN_MODULE_ID, 0, CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
            break;
    }
}

static boolean setupRxFifo(Can_UnitType *canUnit, uint32 rxFifoNr, uint32 rxFifoSz)
{
    CAN_RFCCx_t *rfcc;
    uint32 rfdc = 0;

    /* Check fifo number */
    if (rxFifoNr >= 8) {
        /* Until common fifos are used, 8 is the limit */
        return FALSE;
    }

    /* Convert RX fifo size to reg val */
    switch (rxFifoSz) {
        case 0:
            rfdc = CAN_RFDC_0_MSGS;  /* Do as instructed even if this will make the FIFO unusable */
            break;
        case 4:
            rfdc = CAN_RFDC_4_MSGS;
            break;
        case 8:
            rfdc = CAN_RFDC_8_MSGS;
            break;
        case 16:
            rfdc = CAN_RFDC_16_MSGS;
            break;
        case 32:
            rfdc = CAN_RFDC_32_MSGS;
            break;
        case 48:
            rfdc = CAN_RFDC_48_MSGS;
            break;
        case 64:
            rfdc = CAN_RFDC_64_MSGS;
            break;
        case 128:
            rfdc = CAN_RFDC_128_MSGS;
            break;
        default:
            return FALSE;
    }

    /* Write settings to specific fifo reg */
    /*lint -save -e661 -e662 Within range of RFCCx (x = 0 to 7) rxFifoNr even checked earlier in code */
    rfcc = &((CAN_RFCCx_t *)&canUnit->hwPtr->RFCC0)[rxFifoNr];  /*lint !e929 Ok to cast pointer type to HW register in this case */
    rfcc->R = 0;
    rfcc->B.RFIM = 1; /* Trigger interrupt for each msg received */
    rfcc->B.RFDC = rfdc;
    rfcc->B.RFIE = 0; /* Do not enable RX FIFO IRQ yet */
    /*lint -restore*/

    BIT_SET(canUnit->Can_Arc_RxFifoMask, rxFifoNr);

    return TRUE;
}

static uint32 firstFreePduHandleIdx(const Can_UnitType *canUnit)
{
    for (uint32 i = 0; i < TXBUFS_PER_CH; i++) {
        if (canUnit->cfgCtrlPtr->Can_Arc_TxPduHandles[i] == 0) {
            return i;
        }
    }
    /* Function should only be called when TX queue has free buffers,
     * i.e. if we reach here, indicate error... */
    DET_REPORTERROR(CAN_MODULE_ID, 0, CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
    return 0xFF;
}

static void setupTransmitQueues(const Can_UnitType *canUnit)
{
    CAN_TXQCCx_t *txqccxPtr;
    CAN_THLCCx_t *thlccPtr;

    /* Setup transmit queues (enabling is done later) */
    txqccxPtr = &((CAN_TXQCCx_t *)&canUnit->hwPtr->TXQCC0)[canUnit->hwChIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
    txqccxPtr->B.TXQDC = 15; /* Transmit Queue Length (2..15) */
    txqccxPtr->B.TXQIM = 1; /* IRQ for every buffer transmitted for now... */

    /* Setup transmit history*/
    /* Enable Transmit History Buffers */
    thlccPtr = &((CAN_THLCCx_t *)&canUnit->hwPtr->THLCC0)[canUnit->hwChIdx];  /*lint !e929 Ok to cast pointer type to HW register in this case */
    thlccPtr->B.THLDTE = 0;
    thlccPtr->B.THLIM = 1;
}

/**
 * Function that takes a hardware receive handle configuration
 * and setups an acceptance filter for it.
 *
 * @param Controller   Id of current CAN controller
 * @param hrhPtr       Pointer to a hardware receive handle
 */
static boolean setupReceiveRule(const Can_UnitType *canUnit, const Can_HardwareObjectType *hrhPtr)
{
    uint8 hwUnitNum = GET_HWUNIT(canUnit->cfgCtrlPtr->Can_Arc_HwChId);
    volatile CAN_GAFL_t *gaflReg;
    RSCanSetupInfo_t *canInfo = &RSCanSetupInfo[hwUnitNum];

    /* Are all receive rules occupied? */
    if ( canInfo->recRulePageNum == AF_MAX_NUMPAGES ) {
        return FALSE;
    }

    /* Setup an access pointer to the next available receive rule on the current page */
    gaflReg = &((CAN_GAFL_t *)&canUnit->hwPtr->GAFLID0)[canInfo->numRecRules % AF_PAGE_SIZE]; /*lint !e826 !e929 Ok to cast pointer type to HW register in this case */

    /* Setup acceptance filter(s)
     *
     * There are 16 receive rules on each page.
     * The receive rules in one page are set by manipulating the following
     * registers: GAFLIDj, GAFLMj, GAFLP0j and GAFLP1j, where i=0..15
     *
     * There are also 24 pages. The current page number is controlled
     * by setting register GAFLECTR.AFLPN to a value between 0..23.
     *
     * So in total there are 16*24 = 384 receive rules to share among the
     * CAN channels/controllers on each hardware unit (RSCAN0,1).
     */
    // ID register
    gaflReg->GAFLID.R = 0x0;
    gaflReg->GAFLID.B.GAFLID = hrhPtr->CanHwFilterCode;
    if ((CAN_ID_TYPE_EXTENDED == hrhPtr->CanIdType) ||
        ((CAN_ID_TYPE_MIXED == hrhPtr->CanIdType) &&
         ((hrhPtr->CanHwFilterCode & ~STD_CANID_MASK) !=0 ))) {
        gaflReg->GAFLID.B.GAFLIDE = 1;
    }
    else {
        gaflReg->GAFLID.B.GAFLIDE = 0;
    }
    gaflReg->GAFLID.B.GAFLRTR = 0;
    gaflReg->GAFLID.B.GAFLLB = ( (canUnit->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_LOOPBACK) != 0 ) ? 1 : 0;

    // ID mask register
    gaflReg->GAFLM.R = 0x0;
    gaflReg->GAFLM.B.GAFLIDM = (vuint32_t)hrhPtr->CanHwFilterMask;
    gaflReg->GAFLM.B.GAFLIDEM = (vuint32_t)((hrhPtr->CanHwFilterMask & 0x80000000uL) != 0u);
    gaflReg->GAFLM.B.GAFLRTRM = 0;

    // Pointer 0 and 1 registers
    gaflReg->GAFLP0.R = 0x0;
    gaflReg->GAFLP1.R = 0x0;
    gaflReg->GAFLP0.B.GAFLDLC = 0; // Consider adding support for DLC filtering
    gaflReg->GAFLP0.B.GAFLPTR = hrhPtr->CanObjectId;

#if (USE_ONLY_RXFIFOS == STD_ON)
    /* Only use RX FIFOs
     * 1 RX FIFO per HW channel
     * Map fifo number to logical hw channel number
     * i.e On RSCAN0 HW Channel 0..5 ==> RSCAN0 FIFO 0..5
     *        RSCAN1 HW Channel 0..1 ==> RSCAN1 FIFO 0..1
     */
    gaflReg->GAFLP1.B.GAFLFDP0 = 1u << canUnit->hwChIdx;
#else
    /* Setup fifo or receive buffer? */
    if( hrhPtr->Can_Arc_Flags & CAN_HOH_FIFO_MASK ) {
        gaflReg->GAFLP1.B.GAFLFDP0 = 1 << canInfo->numRxFifosUsed;
        if(FALSE == setupRxFifo(canUnit, canInfo->numRxFifosUsed, hrhPtr->CanHwObjectCount)) {
            /* Out of fifos or wrong fifo size specified */
            return FALSE;
        }
        canInfo->numRxFifosUsed++;
    }
    else {
        gaflReg->GAFLP0.B.GAFLRMV = 1;  // Enable receive buffers for this channel
        gaflReg->GAFLP0.B.GAFLRMDP = canInfo->numMbsUsed++;
    }
#endif

    canInfo->numRecRules++;

    /* Check if we need a new page */
    if ( 0 == ( canInfo->numRecRules % AF_PAGE_SIZE ) ) {
        CAN_GAFLECTR_t *gaflectr = (CAN_GAFLECTR_t *)&canUnit->hwPtr->GAFLECTR; /*lint !e929 Ok to cast pointer type to HW register in this case */
        canInfo->recRulePageNum++;
        gaflectr->B.AFLPN = canInfo->recRulePageNum;
    }

    return TRUE;
}

static void setupReceiveRules(const Can_UnitType *canUnit)
{
    const Can_ControllerConfigType *config;
    const Can_HardwareObjectType *hohPtr;
    rscan_t *canHw;
    uint8 numRecRulesForThisCh = 0;

    config = canUnit->cfgCtrlPtr;
    canHw = canUnit->hwPtr;

    /* Write enable receive rules */
    BIT_SET(canHw->GAFLECTR.UINT32, CAN_AFLDAE_BITPOS);

    hohPtr = config->Can_Arc_Hoh;

    /* Process the hardware objects */
    for( uint32 i=0; i < config->Can_Arc_HohCnt; i++,hohPtr++) { /*lint !e9008 !e9049 Allow this comma operator and increment of ptr */
        VALIDATE_NO_RV((hohPtr != NULL), CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
        if( ( hohPtr->CanObjectType != CAN_OBJECT_TYPE_RECEIVE ) ) {
            continue;
        }
        VALIDATE_NO_RV(( FALSE != setupReceiveRule(canUnit, hohPtr)), CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
        numRecRulesForThisCh++;
    }

    /* All receive rules for this channel processed,
     * now set total number of rules used for it */
    setNumRecRulesForCh(config->Can_Arc_HwChId, numRecRulesForThisCh);

    /* ...and write protect receive rules */
    BIT_CLR(canHw->GAFLECTR.UINT32, CAN_AFLDAE_BITPOS);
}


#if (CAN_USE_READ_POLLING == STD_ON)
static void handleRxBufRegRead(CAN_RXBUF_t *rxBufBase, uint32 regFlags, volatile uint32 *regPtr, uint32 regIdx)
{
    uint32 bitPos = 0;

    while ( regFlags ) {
        if (regFlags & (1 << bitPos)) {
            CAN_RXBUF_t rxBuf;
            /* Clear bit, both in local copy and corresponding bit in HW */
            BIT_CLR(regFlags,bitPos);
            BIT_CLR(*regPtr,bitPos);

            /* Read/copy the buffer at once since it can be overwritten at any time */
            rxBuf = rxBufBase[bitPos*regIdx];

            /** @req 4.1.2/SWS_Can_00396 */
            /** @req 4.1.2/SWS_Can_00279 */
            /** @req 4.1.2/SWS_Can_00060 *//* Is according to definition */
            /** @req 4.1.2/SWS_Can_00423 *//* Is according to definition */
            GET_CALLBACKS()->RxIndication(rxBuf.RFPTRx.B.RFPTR,
                                          rxBuf.RFIDx.R,
                                          rxBuf.RFPTRx.B.RFDLC,
                                          (uint8 *) &rxBuf.RFDF0x.R);
        }
        bitPos++;
    }
}

static void msgBuffersRead(uint32 unit)
{
    uint32 numRegs = 0;
    uint32 rmnd[3];
    volatile uint32 *rmndPtr[3];
    rscan_t *hwPtr = GET_RSCAN_BASE(unit);

    rmndPtr[0] = (volatile uint32 *)&hwPtr->RMND0.UINT32;
    rmndPtr[1] = (volatile uint32 *)&hwPtr->RMND1.UINT32;
    rmndPtr[2] = (volatile uint32 *)&hwPtr->RMND2.UINT32;

    /* Read current state of RX flag registers */
    rmnd[0] = *rmndPtr[0];
    rmnd[1] = *rmndPtr[1];
    rmnd[2] = *rmndPtr[2];

    /*
     *  The manual (and the register definitions) is sometimes not consistent.
     *
     * According to manual and register definitions we have the following:
     *
     *                                        RSCAN0           RSCAN1
     * Receive Buffer New Data registers         3                2
     * Number of receive buffers                96               32
     *
     * Each "Receive Buffer New Data register" (RMNDz) contains 32 bits/flags,
     * one bit/flag per receive buffer to indicate if a message has been received or not.
     * But for RSCAN1, there are two registers defined which seems strange since
     * there are only 32 receive buffers to handle for this HW unit.
     *
     * I therefore make the assumption that there is only one RMNDz register for RSCAN1
     */
    if (RSCAN0_UNIT == unit) {
        numRegs = 3;
    }
    else if (RSCAN1_UNIT == unit) {
        numRegs = 1;
    }

    /* Check and handle RX buffers */
    for (uint8 i = 0; i < numRegs; i++) {
        handleRxBufRegRead((CAN_RXBUF_t *)&hwPtr->RMID0.UINT32, rmnd[i], rmndPtr[i], i);
    }
}
#endif

/*lint -save -e957 No prototype needed for ISR funcs */
#if RSCAN0_IN_USE
ISR(Can_Global_Rx0) {Can_Global_RxFifo(RSCAN0_UNIT);}
#endif
#if RSCAN1_IN_USE
ISR(Can_Global_Rx1) {Can_Global_RxFifo(RSCAN1_UNIT);}
#endif

#if USE_RSCAN_CH0 == STD_ON
ISR(Can_Ch0_IsrTx) {Can_IsrChTx(RSCAN_CH0_e);}
ISR(Can_Ch0_Err) {Can_IsrChErr(RSCAN_CH0_e);}
#endif

#if USE_RSCAN_CH1 == STD_ON
ISR(Can_Ch1_IsrTx) {Can_IsrChTx(RSCAN_CH1_e);}
ISR(Can_Ch1_Err) {Can_IsrChErr(RSCAN_CH1_e);}
#endif

#if USE_RSCAN_CH2 == STD_ON
ISR(Can_Ch2_IsrTx) {Can_IsrChTx(RSCAN_CH2_e);}
ISR(Can_Ch2_Err) {Can_IsrChErr(RSCAN_CH2_e);}
#endif

#if USE_RSCAN_CH3 == STD_ON
ISR(Can_Ch3_IsrTx) {Can_IsrChTx(RSCAN_CH3_e);}
ISR(Can_Ch3_Err) {Can_IsrChErr(RSCAN_CH3_e);}
#endif

#if USE_RSCAN_CH4 == STD_ON
ISR(Can_Ch4_IsrTx) {Can_IsrChTx(RSCAN_CH4_e);}
ISR(Can_Ch4_Err) {Can_IsrChErr(RSCAN_CH4_e);}
#endif

#if USE_RSCAN_CH5 == STD_ON
ISR(Can_Ch5_IsrTx) {Can_IsrChTx(RSCAN_CH5_e);}
ISR(Can_Ch5_Err) {Can_IsrChErr(RSCAN_CH5_e);}
#endif

#if USE_RSCAN_CH6 == STD_ON
ISR(Can_Ch6_IsrTx) {Can_IsrChTx(RSCAN_CH6_e);}
ISR(Can_Ch6_Err) {Can_IsrChErr(RSCAN_CH6_e);}
#endif

#if USE_RSCAN_CH7 == STD_ON
ISR(Can_Ch7_IsrTx) {Can_IsrChTx(RSCAN_CH7_e);}
ISR(Can_Ch7_Err) {Can_IsrChErr(RSCAN_CH7_e);}
#endif
/*lint -restore*/
//-------------------------------------------------------------------

#if defined(CFG_CAN_TEST)
Can_TestType * Can_Arc_GetTestInfo( void ) {
	return &Can_Test;
}
#endif

#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
static void HandlePendingTxCancel(Can_UnitType *canUnit)
{
    uint32 mbufNr;
    uint32 mbCancelMask;
    rscan_t *canHw = canUnit->hwPtr;
    PduIdType pduId;
    PduInfoType pduInfo;
    imask_t state;
    uint32 mbufOffset = canUnit->hwChIdx * TXBUFS_PER_CH;
    Irq_Save(state);
    mbCancelMask = canUnit->mbTxCancel;
    for( ; mbCancelMask != 0; BIT_CLR(mbCancelMask,mbufNr - mbufOffset) ) {
        volatile uint8 *tmsts;
        mbufNr = mbufOffset + (uint32)ilog2(mbCancelMask);
        tmsts = &((volatile uint8 *)&canHw->TMSTS0)[mbufNr]; /*lint !e926 Ok to cast pointer type to HW register in this case */

        /* Set abort request on mbuf */
        BIT_SET((&canHw->TMC0)[mbufNr], CAN_TMC_TMTAR_BITPOS);
        /* Did it take immediately? */
        if ((*tmsts & CAN_TMSTS_TMTRF_MASK) == CAN_TMSTS_TMTRF_ABORTOK) {
            const CAN_THLACCm_t *thlacc = &((CAN_THLACCm_t *)&canHw->THLACC0)[canUnit->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */
            CAN_TXBUF_t *txBufs = (CAN_TXBUF_t *)&canHw->TMID0; /*lint !e826 !e929 Ok to cast pointer type to HW register in this case */

            /* Aborted
             * Clear the TX Abort Interrupt Flag (TAIF) */
            *tmsts &= (uint8)~CAN_TMSTS_TMTRF_MASK;

            pduId = canUnit->cfgCtrlPtr->Can_Arc_TxPduHandles[thlacc->B.TID];
            canUnit->cfgCtrlPtr->Can_Arc_TxPduHandles[thlacc->B.TID] = 0;
            pduInfo.SduLength = (PduLengthType)txBufs[mbufNr].TMPTRx.B.TMDLC;
            pduInfo.SduDataPtr = (uint8*)&txBufs[mbufNr].TMDF0x; /*lint !e928 Ok to cast pointer to (uint8 *) here */
            /** @req 4.0.3/CAN287 *//** @req 4.1.2/SWS_Can_00287 */
            if( NULL != GET_CALLBACKS()->CancelTxConfirmation ) {
                GET_CALLBACKS()->CancelTxConfirmation(pduId, &pduInfo); /*lint !e934 Contents will be copied by upper layer*/
            }
        } else {
            /* Did not take. Interrupt will be generated.
             * CancelTxConfirmation will be handled there. */
        }
        BIT_CLR(canUnit->mbTxCancel, mbufNr - mbufOffset);
        BIT_CLR(canUnit->suppressMbTxCancel, mbufNr - mbufOffset);
    }
    Irq_Restore(state);
}
#endif

/**
 * Abort transmit function for CAN
 *
 * @param canUnit
 */
// Uses 25.4.5.1 Transmission Abort Mechanism
static void Can_AbortTx(Can_UnitType *canUnit)
{
    volatile uint8 *tmcBasePtr = (volatile uint8 *)&canUnit->hwPtr->TMC0; /*lint !e926 Ok to cast pointer type to HW register in this case */
    uint32 txBufIdx = TXBUFS_PER_CH * canUnit->hwChIdx;
    uint32 txBufIdxEnd = txBufIdx + TXBUFS_PER_CH;

    /* Find transmit buffers that has transmission request flag set
     * and mark these for abortion */
    for (; txBufIdx < txBufIdxEnd; txBufIdx++) {
        if( BIT_CHK(tmcBasePtr[txBufIdx], CAN_TMC_TMTR_BITPOS) ) {
            BIT_SET(tmcBasePtr[txBufIdx], CAN_TMC_TMTAR_BITPOS);
#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
            BIT_SET(canUnit->suppressMbTxCancel, txBufIdx);
            BIT_CLR(canUnit->mbTxCancel, txBufIdx);
#endif
        }
    }
}

/**
 * BusOff handling function
 *
 * @param canUnit
 */
static void handleBusOff(Can_UnitType *canUnit)
{
    Can_ReturnType status;

    /** @req 4.0.3/CAN033 *//** @req 4.1.2/SWS_Can_00033 */
#if (USE_CAN_STATISTICS == STD_ON)
    canUnit->stats.boffCnt++;
#endif
    if (GET_CALLBACKS()->ControllerBusOff != NULL) {
#if defined(CFG_CAN_USE_SYMBOLIC_CANIF_CONTROLLER_ID)
        GET_CALLBACKS()->ControllerBusOff(canUnit->cfgCtrlPtr->Can_Arc_CanIfControllerId);
#else
        GET_CALLBACKS()->ControllerBusOff(Can_Global.config->CanConfigSetPtr->ArcHwUnitToController[canUnit->cfgCtrlPtr->Can_Arc_HwChId]);
#endif
    }
    /** @req SWS_Can_00020 */
    /** @req 4.0.3/CAN272 *//** @req 4.1.2/SWS_Can_00272 */
    /* NOTE: This will give an indication to CanIf that stopped is reached. Should it? */
    status = Can_SetControllerMode(Can_Global.config->CanConfigSetPtr->ArcHwUnitToController[canUnit->cfgCtrlPtr->Can_Arc_HwChId], CAN_T_STOP);
    VALIDATE_NO_RV(( CAN_OK != status), CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);

    Can_AbortTx(canUnit);/** @req 4.0.3/CAN273 *//** @req 4.1.2/SWS_Can_00273 */
}

static void rxFifoIsrHandler(uint8 rscan_unit, uint8 fifoChannel)
{
    uint32 numUnread;
    vuint32_t *rfstsBase = (vuint32_t *)&GET_RSCAN_BASE(rscan_unit)->RFSTS0;  /*lint !e929 Ok to cast pointer type to HW register in this case */

    /* Get number of unread messages in FIFO */
    numUnread = (rfstsBase[fifoChannel] & CAN_RFMC_MASK) >> CAN_RFMC_BITPOS;

    while ( 0 != numUnread-- ) {
        /* Handle RX processing */
        if (GET_CALLBACKS()->RxIndication != NULL) {
            const CAN_RXFIFO_t * rxFifoPduBase = (const CAN_RXFIFO_t *)&GET_RSCAN_BASE(rscan_unit)->RFID0; /*lint !e826 !e929 Ok to cast pointer type to HW register in this case */

            /** @req 4.1.2/SWS_Can_00396 */
            /** @req 4.1.2/SWS_Can_00279 */
            /** @req 4.1.2/SWS_Can_00060 *//* Is according to definition */
            /** @req 4.1.2/SWS_Can_00423 *//* Is according to definition */
            /*lint -e928 -e9005 Allow casts to interface parameter types */
            GET_CALLBACKS()->RxIndication((uint8)rxFifoPduBase[fifoChannel].RFPTRx.B.RFPTR,
                    (Can_IdType)rxFifoPduBase[fifoChannel].RFIDx.R,
                    (uint8)rxFifoPduBase[fifoChannel].RFPTRx.B.RFDLC,
                    (const uint8 *) &rxFifoPduBase[fifoChannel].RFDF0x.R);
            /* ...get the next RXFIFO entry */
            (&GET_RSCAN_BASE(rscan_unit)->RFPCTR0.UINT32)[fifoChannel] = 0xFF;
        }
    }

    /* Check "Receive Fifo Message Lost" flag */
    if ( BIT_CHK(rfstsBase[fifoChannel], CAN_RFMLT_BITPOS) ) {
#if (USE_CAN_STATISTICS == STD_ON)
        /* Fifo channel has a 1 to 1 mapping to HW channel idx */
        Can_UnitType *uPtr = CTRL_TO_UNIT_PTR(HWCHIDX_TO_CTRL(rscan_unit, fifoChannel));
        uPtr->stats.fifoOverflow++;
#endif
        DET_REPORTERROR(CAN_MODULE_ID,0,0, CAN_E_DATALOST); /** @req 4.0.3/CAN395 *//** @req 4.1.2/SWS_Can_00395 */

        /* Clear "Receive Fifo Message Lost" flag */
        BIT_CLR(rfstsBase[fifoChannel], CAN_RFMLT_BITPOS);
    }

#if (USE_CAN_STATISTICS == STD_ON)
    /* Check "Receive Fifo Full" flag */
    if ( BIT_CHK(rfstsBase[fifoChannel], CAN_RFFLL_BITPOS) ) {
        /* Fifo channel has a 1 to 1 mapping to HW channel idx */
        Can_UnitType *uPtr = CTRL_TO_UNIT_PTR(HWCHIDX_TO_CTRL(rscan_unit, fifoChannel));
        uPtr->stats.fifoWarning++;

        /* "Receive Fifo Full" flag is automatically cleared when there is
         * available space in the fifo again */
    }
#endif

    /** @req 4.1.2/SWS_Can_00420 */
    /* Clear RX FIFO irq */
    BIT_CLR(rfstsBase[fifoChannel], CAN_RFIF_BITPOS);
}


static void Can_Global_RxFifo(uint8 rscan_unit)
{
    /** @req 4.0.3/CAN033 *//** @req 4.1.2/SWS_Can_00033 */
    uint32 irqStatusMask = (uint32)GET_RSCAN_BASE(rscan_unit)->RFISTS.UINT32;
    uint8 fifoChannel = 0;

    while ( irqStatusMask != 0 ) {
        fifoChannel = ilog2(irqStatusMask);
        rxFifoIsrHandler(rscan_unit, fifoChannel);
        BIT_CLR(irqStatusMask, fifoChannel);
    }
}

/**
 *  TX Channel m Interrupt Handler
 *
 *  Possible interrupt sources:
 *  – CANm transmit complete interrupt
 *  + CANm transmit abort interrupt
 *  – CANm transmit/receive FIFO transmit complete interrupt (in transmit mode, gateway mode)
 *  + CANm transmit history interrupt
 *  – CANm transmit queue Interrupt
 *
 *  Currently handling interrupt sources above bulleted with '+'
 *
 * @param uPtr
 */
static void Can_Ch_Isr_Tx(Can_UnitType *uPtr)
{
    /** @req 4.0.3/CAN033 *//** @req 4.1.2/SWS_Can_00033 */
    rscan_t *canHw;
    uint8 txIntSts;

    canHw = uPtr->hwPtr;
    txIntSts = ((volatile uint8 *)&canHw->GTINTSTS0)[uPtr->hwChIdx]; /*lint !e928 Ok to cast pointer type to HW register in this case */

    /* Check Transmit History Interrupt Status Flag */
    if ( BIT_CHK(txIntSts, CAN_GTINTSTS_THIF_BITPOS) ) {
        CAN_THLSTSx_t *thlsts;
        uint32 numMsgs;

        thlsts = &((CAN_THLSTSx_t *)&canHw->THLSTS0)[uPtr->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */
        numMsgs = thlsts->B.THLMC;

        while ( 0 != numMsgs-- ) {
            const CAN_THLACCm_t *thlacc;
            PduIdType pduId;
            uint32 pduIdIdx;

            /* Get ID of the transmitted PDU */
            thlacc = &((CAN_THLACCm_t *)&canHw->THLACC0)[uPtr->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */
            pduIdIdx = thlacc->B.TID;
            pduId = uPtr->cfgCtrlPtr->Can_Arc_TxPduHandles[pduIdIdx];
            uPtr->cfgCtrlPtr->Can_Arc_TxPduHandles[pduIdIdx] = 0;

#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
            /* Clear pending cancel on this msg buf */
            BIT_CLR(uPtr->mbTxCancel, thlacc->B.BN / TXBUFS_PER_CH);
#endif

            /** @req 4.1.2/SWS_Can_00016 */
            if (GET_CALLBACKS()->TxConfirmation != NULL) {
                GET_CALLBACKS()->TxConfirmation(pduId);
            }

            /* Advance to next buffer */
            (&canHw->THLPCTR0)[uPtr->hwChIdx].UINT32 = 0xFF;
        }

        /** @req 4.0.3/CAN420 *//** @req 4.1.2/SWS_Can_00420 */
        /* Clear Transmit History Interrupt Status Flag */
        thlsts->B.THLIF = 0;
    }

#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
    /* Check Transmit Buffer Abort Interrupt Status Flag */
    if ( BIT_CHK(txIntSts, CAN_GTINTSTS_TAIF_BITPOS) ) {
        uint32 mbNr, mbChNr;
        volatile uint8 *tmsts;
        uint16 txChAbortFlags;

        /* Get abort mask of buffers for this channel */
        txChAbortFlags = ((uint16 *)&canHw->TMTASTS0)[uPtr->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */
        while ( txChAbortFlags != 0 ) {
            mbChNr = (uint32)ilog2((uint32)txChAbortFlags);
            mbNr = (uPtr->hwChIdx * TXBUFS_PER_CH) + mbChNr;
            tmsts = (uint8 *)&(&canHw->TMSTS0)[mbNr]; /*lint !e926 Ok to cast pointer type to HW register in this case */

            if( 0 == (uPtr->suppressMbTxCancel & (1uL << mbChNr)) ) {
                /* This was an abort and it should not be suppressed */
                PduIdType pduId;
                PduInfoType pduInfo;
                uint32 pduIdIdx;
                CAN_TXBUF_t *txBuf;

                txBuf = &((CAN_TXBUF_t *)&canHw->TMID0)[mbNr]; /*lint !e826 !e929 Ok to cast pointer type to HW register in this case */
                pduIdIdx = txBuf->TMPTRx.B.TMPTR;

                pduId = uPtr->cfgCtrlPtr->Can_Arc_TxPduHandles[pduIdIdx];
                uPtr->cfgCtrlPtr->Can_Arc_TxPduHandles[pduIdIdx] = 0;
                pduInfo.SduLength = (PduLengthType)txBuf->TMPTRx.B.TMDLC;
                pduInfo.SduDataPtr = (uint8*)&txBuf->TMDF0x;
                /** @req 4.0.3/CAN287 */ /** @req 4.1.2/SWS_Can_00287 */
                if( NULL != GET_CALLBACKS()->CancelTxConfirmation ) {
                    GET_CALLBACKS()->CancelTxConfirmation(pduId, &pduInfo); /*lint !e934 Contents will be copied by upper layer*/
                }
            }

            BIT_CLR(txChAbortFlags, mbChNr);

            /** @req 4.0.3/CAN420 *//** @req 4.1.2/SWS_Can_00420 */
            /* Clear abort status of buffer */
            *tmsts &= (uint8)~CAN_TMSTS_TMTRF_MASK;
        }
    }
#endif
}

static void Can_IsrChTx(Can_Arc_HwChIdType hwChId)
{
    Can_UnitType *uPtr = CTRL_TO_UNIT_PTR(Can_Global.config->CanConfigSetPtr->ArcHwUnitToController[hwChId]);

    if( (uPtr->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_TX_PROCESSING_INTERRUPT) != 0 ){
        Can_Ch_Isr_Tx(uPtr);
    }
}

static void Can_IsrChErr(Can_Arc_HwChIdType hwChId)
{
    /** @req 4.0.3/CAN033 *//** @req 4.1.2/SWS_Can_00033 */
    Can_UnitType *canUnit = CTRL_TO_UNIT_PTR(Can_Global.config->CanConfigSetPtr->ArcHwUnitToController[hwChId]);
    Can_Arc_ErrorType err;
    CAN_CxREGS_t *cxregPtr = &GET_CxREGS_BASEPTR(canUnit->cfgCtrlPtr->Can_Arc_HwChId)[canUnit->hwChIdx];
    CxERFL_t erfl = cxregPtr->CxERFL;

    err.R = 0;

    // Copy err flags
    err.B.BEF = erfl.B.BEF; /* Bus Error Flag */
    err.B.EWF = erfl.B.EWF; /*  Error Warning Flag */
    err.B.EPF = erfl.B.EPF; /*  Error Passive Flag */
    err.B.BOEF = erfl.B.BOEF; /*  Bus Off Entry Flag */
    err.B.BORF = erfl.B.BORF; /*  Bus Off Recovery Flag */
    err.B.OVLF = erfl.B.OVLF; /*  Overload Flag */
    err.B.BLF = erfl.B.BLF; /*  Bus Lock Flag */
    err.B.ALF = erfl.B.ALF; /*  Arbitration-lost Flag */
    err.B.SERR = erfl.B.SERR; /*  Stuff Error Flag */
    err.B.FERR = erfl.B.FERR; /*  Form Error Flag */
    err.B.AERR = erfl.B.AERR; /*  ACK Error Flag */
    err.B.CERR = erfl.B.CERR; /*  CRC Error Flag */
    err.B.B1ERR = erfl.B.B1ERR; /*  Recessive Bit Error Flag */
    err.B.B0ERR = erfl.B.B0ERR; /*  Dominant Bit Error Flag */
    err.B.ADERR = erfl.B.ADERR; /*  ACK Delimiter Error Flag */
    err.B.CRCREG = erfl.B.CRCREG; /* CRC Calculation Data */

    if ( err.B.BOEF == 1 ) {
        handleBusOff(canUnit);
    }

    if( err.R != 0 )
    {
        if (GET_CALLBACKS()->Arc_Error != NULL) {
#if defined(CFG_CAN_USE_SYMBOLIC_CANIF_CONTROLLER_ID)
            GET_CALLBACKS()->Arc_Error(canUnit->cfgCtrlPtr->Can_Arc_CanIfControllerId, err);
#else
            GET_CALLBACKS()->Arc_Error(Can_Global.config->CanConfigSetPtr->ArcHwUnitToController[hwChId], err);
#endif
        }
    }

    // Clear error flags
    /** @req 4.0.3/CAN420 *//** @req 4.1.2/SWS_Can_00420 */
    cxregPtr->CxERFL.R = ~erfl.R;
}

static void installGlobalControllerInterrupts(void)
{
#if RSCAN0_IN_USE
    ISR_INSTALL_ISR2( "Can", Can_Global_Rx0, IRQ_INTRCANGRECC0, 2, 0 );
#endif
#if RSCAN1_IN_USE
    ISR_INSTALL_ISR2( "Can", Can_Global_Rx1, IRQ_INTRCANGRECC1, 2, 0 );
#endif
}

static void installControllerInterrupts(Can_Arc_HwChIdType hwChId, uint32 ArcFlags)
{
    switch (hwChId) {
#if USE_RSCAN_CH0 == STD_ON
    case RSCAN_CH0_e:
        ADD_ISR_HANDLERS(RSCAN_CH0, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH1 == STD_ON
    case RSCAN_CH1_e:
        ADD_ISR_HANDLERS(RSCAN_CH1, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH2 == STD_ON
    case RSCAN_CH2_e:
        ADD_ISR_HANDLERS(RSCAN_CH2, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH3 == STD_ON
    case RSCAN_CH3_e:
        ADD_ISR_HANDLERS(RSCAN_CH3, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH4 == STD_ON
    case RSCAN_CH4_e:
        ADD_ISR_HANDLERS(RSCAN_CH4, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH5 == STD_ON
    case RSCAN_CH5_e:
        ADD_ISR_HANDLERS(RSCAN_CH5, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH6 == STD_ON
    case RSCAN_CH6_e:
        ADD_ISR_HANDLERS(RSCAN_CH6, ArcFlags);
        break;
#endif
#if USE_RSCAN_CH7 == STD_ON
    case RSCAN_CH7_e:
        ADD_ISR_HANDLERS(RSCAN_CH7, ArcFlags);
        break;
#endif
    default:
        /* Unexpected channel */
        DET_REPORTERROR(CAN_MODULE_ID, 0, CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
        break;
    }
}

static boolean hwChIsInUse(uint8 hwCh) {
    switch (hwCh) {
        case RSCAN_CH0:
            return (boolean)(USE_RSCAN_CH0 == STD_ON);
        case RSCAN_CH1:
            return (boolean)(USE_RSCAN_CH1 == STD_ON);
        case RSCAN_CH2:
            return (boolean)(USE_RSCAN_CH2 == STD_ON);
        case RSCAN_CH3:
            return (boolean)(USE_RSCAN_CH3 == STD_ON);
        case RSCAN_CH4:
            return (boolean)(USE_RSCAN_CH4 == STD_ON);
        case RSCAN_CH5:
            return (boolean)(USE_RSCAN_CH5 == STD_ON);
        case RSCAN_CH6:
            return (boolean)(USE_RSCAN_CH6 == STD_ON);
        case RSCAN_CH7:
            return (boolean)(USE_RSCAN_CH7 == STD_ON);
        default:
            return FALSE;
    }
}
//-------------------------------------------------------------------

// This initiates ALL can controllers
void Can_Init(const Can_ConfigType *Config)
{
    /** @req 4.0.3/CAN223 *//** @req 4.1.2/SWS_Can_00223 */
    /** @req 4.1.2/SWS_Can_00021 */
    /** @req 4.1.2/SWS_Can_00239 */
    /** @req 4.1.2/SWS_Can_00291 */
    /** !req 4.1.2/SWS_Can_00408 */
    /** @req 4.1.2/SWS_Can_00419 *//* Disable all unused interrupts. We only enable the ones used.. */
    /** @req 4.1.2/SWS_Can_00053 *//* Don't change registers for controllers not used */
    /** @req 4.1.2/SWS_Can_00250 *//* Initialize variables, init controllers */

    Can_UnitType *unitPtr;

    /** @req 4.0.3/CAN104 *//** @req 4.1.2/SWS_Can_00104 */
    /** @req 4.0.3/CAN174 *//** @req 4.1.2/SWS_Can_00174 */
    VALIDATE_NO_RV( (Can_Global.initRun == CAN_UNINIT), CAN_INIT_SERVICE_ID, CAN_E_TRANSITION );
    /** @req 4.0.3/CAN175 *//** @req 4.1.2/SWS_Can_00175 */
    VALIDATE_NO_RV( (Config != NULL ), CAN_INIT_SERVICE_ID, CAN_E_PARAM_POINTER );

    // Init some setup flags for RSCAN
    initRSCanInfo();

    // Save config
    Can_Global.config = Config;
    /** !req 4.1.2/SWS_Can_00246 *//* Should be done after initializing all controllers */
    Can_Global.initRun = CAN_READY;
    /** @req 4.1.2/SWS_Can_00245 */

    /*
     * RS-CAN init:
     * Configure global registers
     */
    for (uint8 i = 0; i < NUM_RSCAN_UNITS; i++) {
        rscan_t *canHw = GET_RSCAN_BASE(i); /*lint !e929 Ok to cast pointer type to HW register in this case */
        CAN_GCFG_t *gcfg;

        if ( !RSCanSetupInfo[i].setupFlags.inUse ) {
            continue;
        }

        // Wait for CAN RAM initialization to complete after (possible) reset
        while( ( canHw->GSTS.UINT32 & CAN_GSTS_GRAMINIT_MASK ) != 0 ) {}

        /* Transition to global reset mode */
        setRSCanGlobalMode(i, GLOBAL_MODE_RESET);

        // Select clock source for RSCAN hw units
        gcfg = (CAN_GCFG_t *)&canHw->GCFG; /*lint !e929 Ok to cast pointer type to HW register in this case */
#if (USE_CAN_OSCILLATOR_CLOCK == STD_ON)
        gcfg->B.DCS = 1;
#else
        gcfg->B.DCS = 0;
#endif
        gcfg->B.TSSS = 0x0; // Timestamp Source Select pclk/2
        gcfg->B.TSBTCS = 0x0;
    }

    /*
     * RS-CAN init:
     * Configure channel specific registers
     */
    for (uint8 controllerId = 0; controllerId < CAN_ARC_CTRL_CONFIG_CNT; controllerId++) {
        const Can_ControllerConfigType *cfgCtrlPtr  = &Can_Global.config->CanConfigSetPtr->CanController[controllerId];
        CAN_CxREGS_t *cxreg;

        unitPtr = CTRL_TO_UNIT_PTR(controllerId);

        memset(unitPtr, 0, sizeof(Can_UnitType));

        unitPtr->hwPtr = GET_HWUNIT_BASE(cfgCtrlPtr->Can_Arc_HwChId);
        unitPtr->hwChIdx = GET_HWCH_IDX(cfgCtrlPtr->Can_Arc_HwChId);

        unitPtr->cfgCtrlPtr = cfgCtrlPtr;
        /** @req 4.0.3/CAN259 *//** @req 4.1.2/SWS_Can_00259 */
        unitPtr->state = CANIF_CS_STOPPED;
        unitPtr->cfgHohPtr = cfgCtrlPtr->Can_Arc_Hoh;

        unitPtr->Can_Arc_RxFifoMask = 0;

        unitPtr->pendingStateIndication = CAN_CTRL_INDICATION_NONE;

        /* Transition from channel stop mode to channel reset mode */
        setRSCanChannelMode(cfgCtrlPtr->Can_Arc_HwChId, CHANNEL_MODE_RESET);

        /* Perform channel configuration */
        /** @req 4.1.2/SWS_Can_00274 */
        /* The CAN driver shall not recover from bus-off automatically */
        cxreg = &GET_CxREGS_BASEPTR(cfgCtrlPtr->Can_Arc_HwChId)[unitPtr->hwChIdx];
#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
        cxreg->CxCTR.B.TAIE = 1; /* Enable Tx Abort Interrupts */
#endif
        cxreg->CxCTR.B.BOM = CAN_BOM_PROG_REQ;
        if( ( cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_BUSOFF_PROCESSING_INTERRUPT ) != 0 ) {
            cxreg->CxCTR.B.BOEIE = 1; /* Enable Bus Off Entry Interrupts */
        }

        /* Install controller interrupt handlers */
        installControllerInterrupts(cfgCtrlPtr->Can_Arc_HwChId, cfgCtrlPtr->Can_Arc_Flags);
    }

    /* Install global RSCAN interrupt handlers */
    installGlobalControllerInterrupts();

    /* Now set controller baudrates and configure receive & transmit info */
    /* IMPORTANT!
     * Receive rules must be setup in CAN Channel order, i.e Can0, Can1, Can2 and so on.
     * Therefore we loop over channel id here instead of controller id.  */
    for (uint8 hwCh = RSCAN_CH0; (hwCh <= RSCAN_CH7); hwCh++) {
        Std_ReturnType status;
        if ( !hwChIsInUse(hwCh) ) {
            continue; /* Skip CAN channels that are not in use */
        }
        uint8 controllerId = Can_Global.config->CanConfigSetPtr->ArcHwUnitToController[hwCh];
        unitPtr = CTRL_TO_UNIT_PTR(controllerId);

        status = Can_ChangeBaudrate(controllerId, (uint16)unitPtr->cfgCtrlPtr->CanControllerDefaultBaudrate);
        VALIDATE_NO_RV(E_OK == status, CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);

#if (USE_ONLY_RXFIFOS == STD_ON)
        boolean rxSetupStatus;
        /*
         * 1 RX FIFO per HW channel
         * Fifo number is mapped to logical hw channel number
         * i.e On RSCAN0 HW Channel 0..5 ==> RSCAN0 FIFO 0..5
         *        RSCAN1 HW Channel 0..1 ==> RSCAN1 FIFO 0..1
         */
        rxSetupStatus = setupRxFifo(unitPtr, unitPtr->hwChIdx, unitPtr->cfgCtrlPtr->Can_Arc_RxFifoSz);
        VALIDATE_NO_RV(( FALSE != rxSetupStatus), CAN_GLOBAL_ID, CAN_E_UNEXPECTED_EXECUTION);
#endif

        /* Setup receive rules (and receive buffers/fifos) */
        setupReceiveRules(unitPtr);

        /* Setup transmit queues */
        setupTransmitQueues(unitPtr);
    }

    /* Complete configuration and prepare for operation */
    for (uint8 i = 0; i < NUM_RSCAN_UNITS; i++) {
        rscan_t *canHw = (rscan_t *)GET_RSCAN_BASE(i); /*lint !e929 Ok to cast pointer type to HW register in this case */

        if ( !RSCanSetupInfo[i].setupFlags.inUse ) {
            continue;
        }

        /* Configure the number of message buffers that has been setup */
        canHw->RMNB.UINT32 = RSCanSetupInfo[i].numMbsUsed;

        /* Transition to global operating mode */
        setRSCanGlobalMode(i, GLOBAL_MODE_OPER);
    }

    return;
}

// Unitialize the module
void Can_Arc_DeInit()
{
    Can_UnitType *canUnit;
    if( CAN_UNINIT == Can_Global.initRun ) {
        return;
    }
    for (uint8 controllerId = 0; controllerId < CAN_ARC_CTRL_CONFIG_CNT; controllerId++) {

        canUnit = CTRL_TO_UNIT_PTR(controllerId);

        Can_DisableControllerInterrupts(controllerId);
 
        canUnit->state = CANIF_CS_UNINIT;

        canUnit->lock_cnt = 0;

#if (USE_CAN_STATISTICS == STD_ON)
        // Clear stats
        memset(&canUnit->stats, 0, sizeof(Can_Arc_StatisticsType));
#endif
    }

    Can_Global.config = NULL;
    Can_Global.initRun = CAN_UNINIT;

    return;
}

Std_ReturnType Can_CheckBaudrate(uint8 Controller, const uint16 Baudrate)
{
    /** @req 4.0.3/CAN454 *//** @req 4.1.2/SWS_Can_00454 *//* API */

    // Checks that the target baudrate is found among the configured
    // baudrates for this controller.

    const Can_UnitType *canUnit;
    const Can_ControllerConfigType *config;
    boolean supportedBR;

    /** @req 4.0.3/CAN456 *//** @req 4.1.2/SWS_Can_00456 *//* UNINIT  */
    VALIDATE( (Can_Global.initRun == CAN_READY), CAN_CHECK_BAUD_RATE_SERVICE_ID, CAN_E_UNINIT );
    /** @req 4.0.3/CAN457 *//** @req 4.1.2/SWS_Can_00457 *//* Invalid controller */
    VALIDATE( VALID_CONTROLLER(Controller) , CAN_CHECK_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_CONTROLLER );
    /** @req 4.0.3/CAN458 *//** @req 4.1.2/SWS_Can_00458 *//* Invalid baudrate value */
    VALIDATE( Baudrate <= 1000, CAN_CHECK_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_BAUDRATE );

    canUnit = CTRL_TO_UNIT_PTR(Controller);
    config = canUnit->cfgCtrlPtr;

    // Find the baudrate config for the target baudrate
    supportedBR = FALSE;
    for( uint32 i=0; i < config->CanControllerSupportedBaudratesCount; i++)
    {
        if (config->CanControllerSupportedBaudrates[i].CanControllerBaudRate == Baudrate)
        {
            supportedBR = TRUE;
        }
    }

    return supportedBR ? E_OK : E_NOT_OK;
}

Std_ReturnType Can_ChangeBaudrate(uint8 Controller, const uint16 Baudrate)
{
    /** @req 4.0.3/CAN449 *//** @req 4.1.2/SWS_Can_00449 */

    uint32 canHwChIdx;
    CAN_CxREGS_t *cxreg;
    uint32 tq;
    uint32 tseg1;
    uint32 tseg2;
    uint32 sjw;
    uint32 clock;
    Can_UnitType *canUnit;
    const Can_ControllerConfigType *config;
    const Can_ControllerBaudrateConfigType *baudratePtr;

    /** @req 4.0.3/CAN450 *//** @req 4.1.2/SWS_Can_00450 */
    VALIDATE( (Can_Global.initRun == CAN_READY), CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_UNINIT );
    /** @req 4.0.3/CAN452 *//** @req 4.1.2/SWS_Can_00452 */
    VALIDATE( VALID_CONTROLLER(Controller) , CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_CONTROLLER );

    canUnit = CTRL_TO_UNIT_PTR(Controller);

    /** @req 4.0.3/CAN453 *//** @req 4.1.2/SWS_Can_00453 */
    VALIDATE( (canUnit->state==CANIF_CS_STOPPED), CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_TRANSITION );

    /** @req 4.0.3/CAN451 *//** @req 4.1.2/SWS_Can_00451 */
    VALIDATE(Can_CheckBaudrate(Controller, Baudrate) == E_OK, CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_BAUDRATE);

    canHwChIdx = canUnit->hwChIdx;
    config = canUnit->cfgCtrlPtr;

    // Find the baudrate config for the target baudrate
    baudratePtr = NULL;
    for( uint32 i=0; i < config->CanControllerSupportedBaudratesCount; i++) {
        if (config->CanControllerSupportedBaudrates[i].CanControllerBaudRate == Baudrate) {
            baudratePtr = &config->CanControllerSupportedBaudrates[0];
        }
    }
    // Catch NULL-pointer when no DET is available
    if (baudratePtr == NULL) {
        return E_NOT_OK; // Baudrate not found
    }

    /* Re-initialize the CAN controller and the controller specific settings. */
    /** @req 4.0.3/CAN062*//** @req 4.1.2/SWS_Can_00062 */
    /* Only affect register areas that contain specific configuration for a single CAN controller. */
    /** @req 4.0.3/CAN255 *//** @req 4.1.2/SWS_Can_00255 */


    /* Clock calculation
     * -------------------------------------------------------------------
     *
     * 1 TQ = 1 CANmTq clock cycle
     * fCAN = clkc or clk_xincan (depends on DCS bit setting in GCFG)
     * fCANTQm = baud rate for channel m
     * ss = 1 (fixed)
     * tseg1 = PROP_SEG + PHASE_SEG1
     * tseg2 = PHASE_SEG2
     * Tq = ss + tseg1 + tseg2 = 1 + PROP_SEG + PHASE_SEG1 + PHASE_SEG2
     *
     * fCANTQm = fCAN / (([BRP value] + 1) * Tq)
     * -->
     * [BRP value] = (fCAN / (fCANTQm * Tq)) - 1
     *             = (fCAN / (baudrate * (1 + tseg1 + tseg2))) - 1
     */

    /* Calculate the number of timequanta's*/
    tseg1 = baudratePtr->CanControllerPropSeg + baudratePtr->CanControllerSeg1; /*lint !e9031 Ok to assign to wider type */
    tseg2 = baudratePtr->CanControllerSeg2;
    tq = 1uL + tseg1 + tseg2;
    sjw = baudratePtr->CanControllerSyncJumpWidth;

    // Check TQ limitations..
    VALIDATE(( (tseg1>=4) && (tseg1<=16)), CAN_E_PARAM_TIMING, E_NOT_OK);
    VALIDATE(( (tseg2>=2) && (tseg2<=8)), CAN_E_PARAM_TIMING, E_NOT_OK);
    VALIDATE(( (tq>8) && (tq<25 )), CAN_E_PARAM_TIMING, E_NOT_OK);
    VALIDATE(( (sjw>=1) && (sjw<=4 )), CAN_E_PARAM_TIMING, E_NOT_OK);

    /* NOTE:
     * Use hardcoded values until MCU implementation provides proper numbers through API calls.
     * i.e. use
     * clock = Mcu_Arc_GetPeripheralClock((Mcu_Arc_PeriperalClock_t) config->CanCpuClockRef);
     * and
     * clock = Mcu_Arc_GetClockReferencePointFrequency();
     *
     */
#if (USE_CAN_OSCILLATOR_CLOCK == STD_ON)
        clock = 16000000;
#else
        // clkc = PPLLCLK2 = 40MHz
        clock = 40000000;
#endif

    cxreg = &GET_CxREGS_BASEPTR(config->Can_Arc_HwChId)[canHwChIdx];
    cxreg->CxCFG.R = 0;
    cxreg->CxCFG.B.BRP = (vuint32_t)(clock / (baudratePtr->CanControllerBaudRate * 1000uL * tq)) - 1uL;
    cxreg->CxCFG.B.TSEG1 = tseg1 - 1uL;
    cxreg->CxCFG.B.TSEG2 = tseg2 - 1uL;
    cxreg->CxCFG.B.SJW   = sjw - 1uL;

    /** @req 4.0.3/CAN260 *//** @req 4.1.2/SWS_Can_00260 */
    canUnit->state = CANIF_CS_STOPPED;

    return E_OK;
}

Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_StateTransitionType Transition)
{
    /** @req 4.0.3/CAN230 *//** @req 4.1.2/SWS_Can_00230 */
    /** @req 4.0.3/CAN017 *//** @req 4.1.2/SWS_Can_00017 */
    /** !req 4.0.3/CAN294 *//** !req 4.1.2/SWS_Can_00294 *//* Wakeup not supported */
    /** !req 4.0.3/CAN197 *//** !req 4.1.2/SWS_Can_00197 *//* Disable interrupts */
    /** !req 4.0.3/CAN426 *//** !req 4.1.2/SWS_Can_00426 *//* Disable interrupts */
    /** @req 4.0.3/CAN200 *//** @req 4.1.2/SWS_Can_00200 *//* Detect invalid transitions */
    imask_t state;
    Can_ReturnType rv = CAN_OK;

    /** @req 4.0.3/CAN104 *//** @req 4.1.2/SWS_Can_00104 */
    /** @req 4.0.3/CAN198 *//** @req 4.1.2/SWS_Can_00198 */
    CAN_VALIDATE( (Can_Global.initRun == CAN_READY), CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_UNINIT );
    /** @req 4.0.3/CAN199 *//** @req 4.1.2/SWS_Can_00199 */
    CAN_VALIDATE( VALID_CONTROLLER(Controller), CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_PARAM_CONTROLLER );
    Can_UnitType *canUnit = CTRL_TO_UNIT_PTR(Controller);
    CAN_VALIDATE( (canUnit->state!=CANIF_CS_UNINIT), CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_UNINIT );

    switch (Transition) {
    case CAN_T_START:
        /** @req 4.0.3/CAN409 *//** @req 4.1.2/SWS_Can_00409 */
        CAN_VALIDATE(canUnit->state == CANIF_CS_STOPPED, 0x3, CAN_E_TRANSITION);

        if ( ( canUnit->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_LOOPBACK ) != 0 ) {
            CAN_CxREGS_t *cxreg;

            /* Must be in halt mode to configure loopback */
            setRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId, CHANNEL_MODE_HALT);

            cxreg = &GET_CxREGS_BASEPTR(canUnit->cfgCtrlPtr->Can_Arc_HwChId)[canUnit->hwChIdx];
#if defined(USE_EXT_LOOPBACK)
            cxreg->CxCTR.B.CTMS = CAN_CTMS_EXT_LOOPBACK;
#else
            cxreg->CxCTR.B.CTMS = CAN_CTMS_INT_LOOPBACK;
#endif
            cxreg->CxCTR.B.CTME = 1; /* Enable Communication Test Mode (Loopback) */
        }

        /** @req 4.0.3/CAN384 *//** @req 4.1.2/SWS_Can_00384 *//* I.e. present conf not changed */
        /** @req 4.1.2/SWS_Can_00261 */
        setRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId, CHANNEL_MODE_COMM);
        canUnit->state = CANIF_CS_STARTED;
        Irq_Save(state);
        /** @req 4.0.3/CAN196 *//** @req 4.1.2/SWS_Can_00196 */
        /** @req 4.0.3/CAN425 *//** @req 4.1.2/SWS_Can_00425 */
        if (canUnit->lock_cnt == 0)
        {
            Can_EnableControllerInterrupts(Controller);
        }
        Irq_Restore(state);
        canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_START;
        break;
    case CAN_T_WAKEUP:
        /** @req 4.0.3/CAN267 *//** @req 4.1.2/SWS_Can_00267 */
        /** @req 4.0.3/CAN412 *//** @req 4.1.2/SWS_Can_00412 */
        /** @req 4.1.2/SWS_Can_00405 *//* Logical sleep left on CAN_T_WAKEUP */
        CAN_VALIDATE( (canUnit->state == CANIF_CS_SLEEP) || (canUnit->state == CANIF_CS_STOPPED), 0x3, CAN_E_TRANSITION);

        // Wake from logical sleep mode or stop
        setRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId, CHANNEL_MODE_RESET);
        canUnit->state = CANIF_CS_STOPPED;
        canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_STOP;
        break;
    case CAN_T_SLEEP:
        /** @req 4.0.3/CAN411 *//** @req 4.1.2/SWS_Can_00411 */
        /** @req 4.0.3/CAN258 *//** @req 4.1.2/SWS_Can_00258 */
        /** @req 4.0.3/CAN290 *//** @req 4.1.2/SWS_Can_00290 */
        /** @req 4.1.2/SWS_Can_00404 *//* Hardware stopped while logical sleep active */
        /** !req 4.1.2/SWS_Can_00265 */
        CAN_VALIDATE(canUnit->state == CANIF_CS_STOPPED, 0x3, CAN_E_TRANSITION);

        // Logical sleep mode = Stop
        setRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId, CHANNEL_MODE_STOP);
        canUnit->state = CANIF_CS_SLEEP;
        /** @req 4.1.2/SWS_Can_00283 *//* Can_AbortTx does not notify cancellation */
        /** @req 4.0.3/CAN282 *//** @req 4.1.2/SWS_Can_00282 */
        Can_AbortTx(canUnit);
        canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_SLEEP;
        break;
    case CAN_T_STOP:
        /** @req 4.0.3/CAN410 *//** @req 4.1.2/SWS_Can_00410 */
        CAN_VALIDATE( (canUnit->state == CANIF_CS_STARTED) || (canUnit->state == CANIF_CS_STOPPED), 0x3, CAN_E_TRANSITION);

        // Stop
        /** @req 4.1.2/SWS_Can_00263 */
        setRSCanChannelMode(canUnit->cfgCtrlPtr->Can_Arc_HwChId, CHANNEL_MODE_RESET);
        canUnit->state = CANIF_CS_STOPPED;
        /** @req 4.1.2/SWS_Can_00283 *//* Can_AbortTx does not notify cancellation */
        /** @req 4.0.3/CAN282 *//** @req 4.1.2/SWS_Can_00282 */
        Can_AbortTx(canUnit);
        canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_STOP;
        break;
    default:
        /** @req 4.0.3/CAN200 *//** @req 4.1.2/SWS_Can_00200 */
        /* Undefined state transition shall always raise CAN_E_TRANSITION */
        CAN_VALIDATE(FALSE, 0x3, CAN_E_TRANSITION); /*lint !e506 !e774 Ok to pass constant boolean in this case*/
        /* intentionally no break */
    }

    return rv;
}

void Can_DisableControllerInterrupts(uint8 Controller)
{
    /** @req 4.0.3/CAN231 *//** @req 4.1.2/SWS_Can_00231 */
    /** @req 4.0.3/CAN202 *//** @req 4.1.2/SWS_Can_00202 */
    /** !req 4.0.3/CAN204 *//** !req 4.1.2/SWS_Can_00204 */

	VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_DISABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_UNINIT );
    /** @req 4.0.3/CAN206 *//** @req 4.1.2/SWS_Can_00206*/
    VALIDATE_NO_RV( VALID_CONTROLLER(Controller) , CAN_DISABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_PARAM_CONTROLLER );
    Can_UnitType *canUnit = CTRL_TO_UNIT_PTR(Controller);
    imask_t state;

    /** @req 4.0.3/CAN205 *//** @req 4.1.2/SWS_Can_00205 */
    VALIDATE_NO_RV( (canUnit->state!=CANIF_CS_UNINIT), CAN_DISABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_UNINIT );

    Irq_Save(state);
    /** @req 4.0.3/CAN049 *//** @req 4.1.2/SWS_Can_00049 */
    if (canUnit->lock_cnt > 0) {
        // Interrupts already disabled
        canUnit->lock_cnt++;
        Irq_Restore(state);
        return;
    }
    canUnit->lock_cnt++;
    Irq_Restore(state);

    rxIrqDisable(canUnit);
    txIrqDisable(canUnit);

}


void Can_EnableControllerInterrupts(uint8 Controller)
{

    /** @req 4.0.3/CAN232 *//** @req 4.1.2/SWS_Can_00232 */
    /** @req 4.0.3/CAN050 *//** @req 4.1.2/SWS_Can_00050 */

    Can_UnitType *canUnit;
    imask_t state;

    /** @req 4.0.3/CAN209 *//** @req 4.1.2/SWS_Can_00209 */
	VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_ENABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_UNINIT );
    /** @req 4.0.3/CAN210 *//** @req 4.1.2/SWS_Can_00210 */
    VALIDATE_NO_RV( VALID_CONTROLLER(Controller), CAN_ENABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_PARAM_CONTROLLER );
    canUnit = CTRL_TO_UNIT_PTR(Controller);
    VALIDATE_NO_RV( (canUnit->state!=CANIF_CS_UNINIT), CAN_ENABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_UNINIT );

    Irq_Save(state);
    /* NOTE: If lock_cnt == 0, no action should be performed? */
    if (canUnit->lock_cnt > 1) {
        /** @req 4.0.3/CAN208 *//** @req 4.1.2/SWS_Can_00208 */
        // IRQ should still be disabled so just decrement counter
        canUnit->lock_cnt--;
        Irq_Restore(state);
        return;
    } else if (canUnit->lock_cnt == 1) {
        canUnit->lock_cnt = 0;
    }
    else {
        /* Please Lint and MISRA 2012 Rule 15.7 with this empty else-clause */
    }
    Irq_Restore(state);

    if ( ( canUnit->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_RX_PROCESSING_INTERRUPT ) != 0 ) {
        /* Turn on the RX FIFO interrupts  */
        rxIrqEnable(canUnit);
    }

    if ( ( canUnit->cfgCtrlPtr->Can_Arc_Flags &  CAN_CTRL_TX_PROCESSING_INTERRUPT ) != 0 ) {
        /* Turn on the interrupt mailboxes */
        txIrqEnable(canUnit);
    }

    return;
}

/*lint -e{954} Pointer variable canUnit cannot be declared as const at all times since it is depending on configuration */
Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType *PduInfo)
{
    /** @req 4.0.3/CAN233 *//** @req 4.1.2/SWS_Can_00233 */
    /** !req 4.0.3/CAN214 *//** !req 4.1.2/SWS_Can_00214 */
    /** @req 4.1.2/SWS_Can_00011 */
    /** @req 4.1.2/SWS_Can_00275 */

    Can_ReturnType rv = CAN_OK;

    rscan_t *canHw;
    const Can_HardwareObjectType *hohObj;
    Can_UnitType *canUnit;
    imask_t state;
    uint32 qTxBufIdx;
    CAN_TXBUF_t *txBuf;
    Can_HwHandleType internalHth;
    const CAN_TXQSTSx_t *txqsts;

    /** @req 4.0.3/CAN104 *//** @req 4.1.2/SWS_Can_00104 */
    /** @req 4.0.3/CAN216 *//** @req 4.1.2/SWS_Can_00216 */
    CAN_VALIDATE( (Can_Global.initRun == CAN_READY), 0x6, CAN_E_UNINIT );
    /** @req 4.0.3/CAN219 *//** @req 4.1.2/SWS_Can_00219 */
    CAN_VALIDATE( (PduInfo != NULL), 0x6, CAN_E_PARAM_POINTER );
    CAN_VALIDATE( (PduInfo->sdu != NULL), 0x6, CAN_E_PARAM_POINTER );
    /** @req 4.0.3/CAN218 *//** @req 4.1.2/SWS_Can_00218 */
    CAN_VALIDATE( (PduInfo->length <= 8), 0x6, CAN_E_PARAM_DLC );
    /** @req 4.0.3/CAN217 *//** @req 4.1.2/SWS_Can_00217 */
    CAN_VALIDATE( (Hth < NUM_OF_HOHS ), 0x6, CAN_E_PARAM_HANDLE );
    /* Hth is the symbolic name for this hoh. Get the internal id */
    internalHth = Can_Global.config->CanConfigSetPtr->ArcSymbolicHohToInternalHoh[Hth];
    CAN_VALIDATE( (internalHth < NUM_OF_HTHS ), 0x6, CAN_E_PARAM_HANDLE );

    canUnit = CTRL_TO_UNIT_PTR(Can_Global.config->CanConfigSetPtr->ArcHthToSymbolicController[internalHth]);
    hohObj  =  &canUnit->cfgHohPtr[Can_Global.config->CanConfigSetPtr->ArcHthToHoh[internalHth]];
    canHw   =  canUnit->hwPtr;

    Irq_Save(state);

    txqsts = &((CAN_TXQSTSx_t *)&canHw->TXQSTS0)[canUnit->hwChIdx]; /*lint !e929 Ok to cast pointer type to HW register in this case */

    qTxBufIdx = (canUnit->hwChIdx * TXBUFS_PER_CH) + 15uL;

    /** @req 4.0.3/CAN212 *//** @req 4.1.2/SWS_Can_00212 */
    /* Check if queue full or not */
    if ( txqsts->B.TXQFLL == 0 ) {
        uint32 pduHandleIdx;
        txBuf = &((CAN_TXBUF_t *)&canHw->TMID0)[qTxBufIdx]; /*lint !e826 !e929 Ok to cast pointer type to HW register in this case */

        /** @req 4.1.2/SWS_Can_00427 *//* SDU buffer adaptation */
        txBuf->TMIDx.R = 0;
        txBuf->TMIDx.B.TMIDE = (vuint32_t)((hohObj->CanIdType == CAN_ID_TYPE_EXTENDED) ||
                                         (hohObj->CanIdType == CAN_ID_TYPE_MIXED));
        txBuf->TMIDx.B.THLEN = 1;   /* Transmit history */
        txBuf->TMIDx.B.TMID = (vuint32_t)PduInfo->id;

        txBuf->TMPTRx.R = 0;
        txBuf->TMPTRx.B.TMDLC = (uint32)PduInfo->length;
        pduHandleIdx = firstFreePduHandleIdx(canUnit);
        txBuf->TMPTRx.B.TMPTR = pduHandleIdx; /* Use label to store index of PDU handle ID */

        /** @req 4.1.2/SWS_Can_000059 *//* Definition of data structure */
        memset((void *)&txBuf->TMDF0x, 0, 8); /*lint !e419 Ok since PDU data starts at base of TMDF0 and spans 8 bytes */
        memcpy((void *)&txBuf->TMDF0x, PduInfo->sdu, (size_t)(PduInfo->length));

        /* Set transmit request and advance queue ptr */
        (&(canHw->TXQPCTR0.UINT32))[canUnit->hwChIdx] = 0xFF;

#if (USE_CAN_STATISTICS == STD_ON)
        canUnit->stats.txSuccessCnt++;
#endif

        // Store pdu handle in unit to be used by TxConfirmation
        /** @req 4.1.2/SWS_Can_00276 */
        canUnit->cfgCtrlPtr->Can_Arc_TxPduHandles[pduHandleIdx] = PduInfo->swPduHandle;

    } else {
#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
        /** @req 4.0.3/CAN213 *//** @req 4.1.2/SWS_Can_00213 */
        /** @req 4.0.3/CAN215 *//** @req 4.1.2/SWS_Can_00215 */
        /** @req 4.0.3/CAN278 *//** @req 4.1.2/SWS_Can_00278 */
        /* Check if there are messages that may be cancelled */
        /* Find the one with the lowest priority */
        /* Assume that mixed is not supported, i.e. only check the canid */
        Can_IdType lowestPrioCanId = INVALID_CANID;
        Can_IdType mbCanId = 0;
        uint32 mbCancel = 0;

        for (uint32 i = canUnit->hwChIdx * TXBUFS_PER_CH; i <= qTxBufIdx; i++) {
            txBuf = &((CAN_TXBUF_t *)&canHw->TMID0)[i]; /*lint !e826 !e929 Ok to cast pointer type to HW register in this case */
            mbCanId = (Can_IdType)txBuf->TMIDx.B.TMID;

            if( (lowestPrioCanId < mbCanId) || (INVALID_CANID == lowestPrioCanId) ) {
                lowestPrioCanId = mbCanId;
                mbCancel = i;
            }
        }

        /** @req 4.0.3/CAN286 *//** @req 4.1.2/SWS_Can_00286 */
        /** @req 4.0.3/CAN399 *//** @req 4.1.2/SWS_Can_00399 */
        if( (lowestPrioCanId > (PduInfo->id & EXTENDED_CANID_MASK)) && (INVALID_CANID != lowestPrioCanId) ) {
            /* Cancel this */
            canUnit->mbTxCancel = 1uL << mbCancel;
        } else {
            /** @req 4.0.3/CAN213 *//** @req 4.1.2/SWS_Can_00213 */
        }
#endif
        /** @req 4.1.2/SWS_Can_00434 *//* But CanIdenticalIdCancellation not supported */
        rv = CAN_BUSY;
    }
    Irq_Restore(state);

    return rv;
}

Can_ReturnType Can_CheckWakeup( uint8 Controller ) {
    /** !req 4.0.3/CAN360 *//** !req 4.1.2/SWS_Can_00360 */
    /** !req 4.0.3/CAN361 *//** !req 4.1.2/SWS_Can_00361 */
    /** !req 4.0.3/CAN362 *//** !req 4.1.2/SWS_Can_00362 */
    /** !req 4.0.3/CAN363 *//** !req 4.1.2/SWS_Can_00363 */
    /** !req 4.1.2/SWS_Can_00484 */
    /** !req 4.1.2/SWS_Can_00485 */

    /* NOT SUPPORTED */
	(void)Controller;
	return CAN_NOT_OK;
}

#if (CAN_USE_WRITE_POLLING == STD_ON)
void Can_MainFunction_Write( void ) {
    /** !req 4.0.3/CAN225 *//** !req 4.1.2/SWS_Can_00225 */
    /** !req 4.0.3/CAN031 *//** !req 4.1.2/SWS_Can_00031 */
    /** !req 4.1.2/SWS_Can_00441 */
    /** !req 4.0.3/CAN179 *//** !req 4.1.2/SWS_Can_00179 *//* CAN_E_UNINIT */

    /* IMPROVEMENT
     * Partly implemented. Code kept for future improvement.

    Can_UnitType *uPtr;

    VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_MAINFUNCTION_WRITE_SERVICE_ID, CAN_E_UNINIT );

    if (Can_Global.initRun == CAN_UNINIT) {
        return;
    }

    for(uint8 controllerId = 0; controllerId < CAN_ARC_CTRL_CONFIG_CNT; controllerId++ ) {
        uPtr = CTRL_TO_UNIT_PTR(controllerId);
        if( (uPtr->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_TX_PROCESSING_INTERRUPT) == 0 ) {
            Can_Ch_Isr_Tx(uPtr);
        }
    }
     */
}
#else
/** @req 4.1.2/SWS_Can_00178 */
#define Can_MainFunction_Write()
#endif

#if (CAN_USE_READ_POLLING == STD_ON)
void Can_MainFunction_Read(void)
{
    /** !req 4.0.3/CAN226 *//** !req 4.1.2/SWS_Can_00226 */
    /** !req 4.0.3/CAN108 *//** !req 4.1.2/SWS_Can_00108 */
    /** !req 4.1.2/SWS_Can_00442 */
    /** !req 4.0.3/CAN181 *//** !req 4.1.2/SWS_Can_00181 *//* CAN_E_UNINIT */
    /* NOT SUPPORTED */

    /* IMPROVEMENT
     * Partly implemented. Code kept for future improvement.

    VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_MAINFUNCTION_READ_SERVICE_ID, CAN_E_UNINIT );

    if (Can_Global.initRun == CAN_UNINIT) {
        return;
    }

    if (RSCAN0_IN_USE) {
        msgBuffersRead(RSCAN0_UNIT);
    }
    if (RSCAN1_IN_USE) {
        msgBuffersRead(RSCAN1_UNIT);
    }
     */
}
#else
/** @req 4.1.2/SWS_Can_00180 */
#define Can_MainFunction_Read()
#endif

#if (CAN_USE_BUSOFF_POLLING == STD_ON)
void Can_MainFunction_BusOff(void)
{
    /* Bus-off polling events */
      /** !req 4.0.3/CAN227 *//** !req 4.1.2/SWS_Can_00227 */
      /** !req 4.0.3/CAN109 *//** !req 4.1.2/SWS_Can_00109 */
      /** !req 4.0.3/CAN184 *//** !req 4.1.2/SWS_Can_00184 *//* CAN_E_UNINIT */
      /* NOT SUPPORTED */

    /* IMPROVEMENT
     * Partly implemented. Code kept for future improvement.

    Can_UnitType *uPtr;

    VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_MAINFUNCTION_BUSOFF_SERVICE_ID, CAN_E_UNINIT );

    if (Can_Global.initRun == CAN_UNINIT) {
        return;
    }

    for(uint8 controllerId = 0; controllerId < CAN_ARC_CTRL_CONFIG_CNT; controllerId++ ) {
        uPtr = CTRL_TO_UNIT_PTR(controllerId);
        if( (uPtr->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_BUSOFF_PROCESSING_INTERRUPT) == 0 ) {
        	handleBusOff(uPtr);
        }
    }
    */
}
#else
/** @req 4.1.2/SWS_Can_00183 */
#define Can_MainFunction_BusOff()
#endif

#if (ARC_CAN_ERROR_POLLING == STD_ON)
void Can_Arc_MainFunction_Error(void)
{
    /* Error polling events */
    // NOTE: remove function (not in req spec)

    const Can_UnitType *uPtr;
    VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_MAINFUNCTION_ERROR_SERVICE_ID, CAN_E_UNINIT );

    if (Can_Global.initRun == CAN_UNINIT) {
        return;
    }
    for(uint8 controllerId = 0; controllerId < CAN_ARC_CTRL_CONFIG_CNT; controllerId++ ) {
        uPtr = CTRL_TO_UNIT_PTR(controllerId);
        if( (uPtr->cfgCtrlPtr->Can_Arc_Flags & CAN_CTRL_ERROR_PROCESSING_INTERRUPT) == 0) {
        	Can_IsrChErr(uPtr->cfgCtrlPtr->Can_Arc_HwChId);
        }
    }
}
#endif

#if (CAN_USE_WAKEUP_POLLING == STD_ON)
void Can_MainFunction_Wakeup(void)
{
    /** !req 4.0.3/CAN228 *//** !req 4.1.2/SWS_Can_00228 */
    /** !req 4.0.3/CAN112 *//** !req 4.1.2/SWS_Can_00112 */
    /** !req 4.0.3/CAN186 *//** !req 4.1.2/SWS_Can_00186 */
    /* Wakeup polling events */

    /* NOT SUPPORTED */
}
#else
/** @req 4.1.2/SWS_Can_00185 */
#define Can_MainFunction_Wakeup()
#endif

void Can_MainFunction_Mode( void )
{
    /** @req 4.0.3/CAN368 *//** @req 4.1.2/SWS_Can_00368 *//* API */
    /** @req 4.0.3/CAN369 *//** @req 4.1.2/SWS_Can_00369 *//* Polling */

    /** !req 4.0.3/CAN398 *//** !req 4.1.2/SWS_Can_00398 *//* Blocking call not supported */
    /** !req 4.0.3/CAN371 *//** !req 4.1.2/SWS_Can_00371 *//* Blocking call not supported */
    /** !req 4.0.3/CAN372 *//** !req 4.1.2/SWS_Can_00372 *//* Blocking call not supported */

    Can_UnitType *uPtr;

    /** @req 4.0.3/CAN379 *//** @req 4.1.2/SWS_Can_00379 *//* CAN_E_UNINIT */
    VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_MAIN_FUNCTION_MODE_SERVICE_ID, CAN_E_UNINIT );

    /** @req 4.0.3/CAN431 *//* On Uninit, return immediately without prod err */
    if (Can_Global.initRun == CAN_UNINIT) {
        return;
    }

    for(uint8 controllerId=0; controllerId < CAN_ARC_CTRL_CONFIG_CNT; controllerId++ )
    {
        boolean clearPending = TRUE;
        CanIf_ControllerModeType indicateMode = CANIF_CS_UNINIT;
        Can_ChannelMode_t chMode;

        uPtr = CTRL_TO_UNIT_PTR(controllerId);
        chMode = getRSCanChannelMode(uPtr->cfgCtrlPtr->Can_Arc_HwChId);

        switch (uPtr->pendingStateIndication)
        {
            case CAN_CTRL_INDICATION_PENDING_START:
                if (uPtr->state == CANIF_CS_STARTED) {
                    if (CHANNEL_MODE_COMM == chMode) {
                        // Started OK, indicate to upper layer
                        indicateMode = CANIF_CS_STARTED;
                    } else {
                        // Not yet ready
                        clearPending = FALSE;
                    }
                }
                break;

            case CAN_CTRL_INDICATION_PENDING_STOP:
                if (uPtr->state == CANIF_CS_STOPPED) {
                    // Stopped, indicate to upper layer
                    indicateMode = CANIF_CS_STOPPED;
                }
                break;

            case CAN_CTRL_INDICATION_PENDING_SLEEP:
                if (uPtr->state == CANIF_CS_SLEEP) {
                    // Stopped, indicate to upper layer
                    indicateMode = CANIF_CS_SLEEP;
                }
                break;


            case CAN_CTRL_INDICATION_NONE:
            default:
                // No action necessary
                clearPending = FALSE;
                break;
        }
        if( (CANIF_CS_UNINIT != indicateMode) && (NULL != GET_CALLBACKS()->ControllerModeIndication) ) {
            /** @req 4.0.3/CAN370 *//** @req 4.1.2/SWS_Can_00370 */
            /** @req 4.0.3/CAN373 *//** @req 4.1.2/SWS_Can_00373 */
#if defined(CFG_CAN_USE_SYMBOLIC_CANIF_CONTROLLER_ID)
            GET_CALLBACKS()->ControllerModeIndication(uPtr->cfgCtrlPtr->Can_Arc_CanIfControllerId, indicateMode);
#else
            GET_CALLBACKS()->ControllerModeIndication(controllerId, indicateMode);
#endif
        }
        if (clearPending) {
            uPtr->pendingStateIndication = CAN_CTRL_INDICATION_NONE;
        }
#if (CAN_HW_TRANSMIT_CANCELLATION == STD_ON)
        if(0 != uPtr->mbTxCancel) {
            HandlePendingTxCancel(uPtr);
        }
#endif
    }
}

#if ( CAN_VERSION_INFO_API == STD_ON )
void Can_GetVersionInfo( Std_VersionInfoType* versioninfo) {

    /** @req SWS_Can_00224 */
    /** @req SWS_Can_00177 */
    VALIDATE_NO_RV( ( NULL != versioninfo ), CAN_GETVERSIONINFO_SERVICE_ID, CAN_E_PARAM_POINTER);

    versioninfo->vendorID = CAN_VENDOR_ID;
    versioninfo->moduleID = CAN_MODULE_ID;
    versioninfo->sw_major_version = CAN_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CAN_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CAN_SW_PATCH_VERSION;
}
#endif

#if (USE_CAN_STATISTICS == STD_ON)
/**
 * Get send/receive/error statistics for a controller
 *
 * @param controller The controller
 * @param stats Pointer to data to copy statistics to
 */


void Can_Arc_GetStatistics(uint8 controller, Can_Arc_StatisticsType *stats)
{
	if(Can_Global.initRun == CAN_READY)
	{
		Can_UnitType *canUnit = CTRL_TO_UNIT_PTR(controller);
		*stats = canUnit->stats;
	}
}
#endif

/*lint -restore */
