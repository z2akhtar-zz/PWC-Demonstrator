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

/** @tagSettings DEFAULT_ARCHITECTURE=TMS570 */
/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */


#include "Can.h"
#include "core_cr4.h"
#if defined(USE_DEM)
#include "Dem.h"
#endif
#if defined(USE_DET)
#include "Det.h"
#endif
#include "CanIf_Cbk.h"
#include "Os.h"
#include "isr.h"
#include "Mcu.h"
#include "arc.h"



#define CAN_TIMEOUT_DURATION   0xFFFFFFFF
#define CAN_INSTANCE           0

#define DCAN1_MAX_MESSAGEBOXES 64
#define DCAN2_MAX_MESSAGEBOXES 64
#define DCAN3_MAX_MESSAGEBOXES 32

#define DCAN_TX_MSG_OBJECT_NR  1
#define DCAN_RX_MSG_OBJECT_NR  2

#define CAN_REG_CTL_INIT_BIT 0U
#define CAN_REG_CTL_SWR_BIT 15U
#define CAN_REG_CTL_CCE_BIT  6U
#define CAN_REG_CTL_ABO_BIT  9U
#define CAN_REG_CTL_PARITY_BIT  9U
#define CAN_REG_CTL_WUBA_BIT  25U
#define CAN_REG_CTL_IE0_BIT  1U
#define CAN_REG_CTL_IE1_BIT  17U
#define CAN_REG_CTL_SIE_BIT  2U
#define CAN_REG_CTL_EIE_BIT  3U
#define CAN_REG_CTL_TEST_MODE_BIT 7U
#define CAN_REG_CTL_DAR_BIT 5U

#define CAN_REG_TR_LBACK_BIT 4U
#define CAN_REG_TR_TX_BIT 4U

#define CAN_REG_INT_EOB_BIT 7U

#define CAN_REG_ES_TXOK_BIT 3U
#define CAN_REG_ES_RXOK_BIT 4U
#define CAN_REG_ES_EWARN_BIT 6U
#define CAN_REG_ES_BOFF_BIT 7U
#define CAN_REG_ES_PER_BIT 8U
#define CAN_REG_ES_WKPEND_BIT 9U
#define CAN_REG_ES_PDA_BIT 10U

#define CAN_REG_IFX_COM_DATAB_BIT 16U
#define CAN_REG_IFX_COM_DATAA_BIT 17U
#define CAN_REG_IFX_COM_TXREQ_BIT 18U
#define CAN_REG_IFX_COM_NEWDAT_BIT 18U
#define CAN_REG_IFX_COM_CLEARINTPEND_BIT 19U
#define CAN_REG_IFX_COM_CONTROL_BIT 20U
#define CAN_REG_IFX_COM_ARB_BIT 21U
#define CAN_REG_IFX_COM_WRRD_BIT 23U
#define CAN_REG_IFX_ARB_DIR_BIT 29U
#define CAN_REG_IFX_ARB_XTD_BIT 30U
#define CAN_REG_IFX_ARB_MSGVAL_BIT 31U


#define CAN_REG_IFX_MCTL_UMASK_BIT 12U
#define CAN_REG_IFX_MCTL_RXIE_BIT 10U
#define CAN_REG_IFX_MCTL_TXIE_BIT 11U
#define CAN_REG_IFX_MCTL_EOB_BIT 7U

#define CLR_MSG_LOST			(0x00004000U)


/** @req 4.0.3/CAN027 */
/** @req 4.1.2/SWS_Can_00091*/
/** @req 4.1.2/SWS_Can_00089*/
#if ( CAN_DEV_ERROR_DETECT == STD_ON )
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          Det_ReportError(CAN_MODULE_ID,0,_api,_err); \
          return CAN_NOT_OK; \
        }

#define VALIDATE_NO_RV(_exp,_api,_err ) \
        if( !(_exp) ) { \
          Det_ReportError(CAN_MODULE_ID,0,_api,_err); \
          return; \
        }

#define DET_REPORTERROR(_x,_y,_z,_q) Det_ReportError(_x, _y, _z, _q)
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_NO_RV(_exp,_api,_err )
#define DET_REPORTERROR(_x,_y,_z,_q)
#endif

#if !defined(USE_DEM)
// If compiled without the DEM, calls to DEM are simply ignored.
#define Dem_ReportErrorStatus(...)
#endif

#if(CAN_WAKEUP_SUPPORT == STD_ON)
#define WUBA_STATUS 1
#else
#define WUBA_STATUS 0
#endif

#define GET_CONTROLLER_CONFIG(_controller)	\
        					&Can_Global.config->CanConfigSetPtr->CanController[(_controller)]

#define GET_CALLBACKS() (Can_Global.config->CanConfigSetPtr->CanCallbacks)

#define GET_PRIVATE_DATA(_controller) &CanUnits[_controller]



/* Macro for waiting until busy flag is 0 */
#define DCAN_WAIT_UNTIL_NOT_BUSY(ControllerId, IfRegId) \
    { \
		uint32 ErrCounter = CAN_TIMEOUT_DURATION; \
		while(CanRegs[ControllerId]->IFx[IfRegId].COM & 0x00008000) { \
			ErrCounter--; \
			if(ErrCounter == 0) { \
				Dem_ReportErrorStatus(DemConf_DemEventParameter_CAN_E_TIMEOUT, DEM_EVENT_STATUS_FAILED); \
				ErrCounter = CAN_TIMEOUT_DURATION; \
				return CAN_NOT_OK; \
			} \
		} \
    }

/* Macro for waiting until busy flag is 0 */
#define DCAN_WAIT_UNTIL_NOT_BUSY_NO_RV(ControllerId, IfRegId) \
	{ \
		uint32 ErrCounter = CAN_TIMEOUT_DURATION; \
		while(CanRegs[ControllerId]->IFx[IfRegId].COM & 0x00008000) { \
			ErrCounter--; \
			if(ErrCounter == 0) { \
				Dem_ReportErrorStatus(DemConf_DemEventParameter_CAN_E_TIMEOUT, DEM_EVENT_STATUS_FAILED); \
				ErrCounter = CAN_TIMEOUT_DURATION; \
				return; \
			} \
		} \
	}

extern const Can_ControllerConfigType CanControllerConfigData[];

// Array for easy access to DCAN register definitions.
static Can_RegisterType* CanRegs[] = {
DCAN1_Base,
DCAN2_Base,
DCAN3_Base };

typedef enum {
	CAN_UNINIT = 0, CAN_READY
} Can_DriverStateType;

typedef enum {
	CAN_CTRL_INDICATION_NONE,
	CAN_CTRL_INDICATION_PENDING_START,
	CAN_CTRL_INDICATION_PENDING_STOP,
	CAN_CTRL_INDICATION_PENDING_SLEEP /* Sleep is not implemented - but we need a logical sleep mode*/
} Can_CtrlPendingStateIndicationType;


// Mapping between HRH and Controller//HOH
typedef struct Can_Arc_ObjectHOHMapStruct {
	CanControllerIdType CanControllerRef;    // Reference to controller
	const Can_HardwareObjectType* CanHOHRef;       // Reference to HOH.
} Can_Arc_ObjectHOHMapType;

/* Type for holding global information used by the driver */
typedef struct {
	Can_DriverStateType initRun;

	// Our config
	const Can_ConfigType *config;

	// One bit for each channel that is configured.
	// Used to determine if validity of a channel
	// 1 - configured
	// 0 - NOT configured
	uint32 configured;
	// Maps the a channel id to a configured channel id
	uint8 channelMap[CAN_CONTROLLER_CNT];
} Can_GlobalType;

// Global config
Can_GlobalType Can_Global = { .initRun = CAN_UNINIT };

/* Type for holding information about each controller */
typedef struct {
	CanIf_ControllerModeType state;
	uint32 lock_cnt;

	// Statistics
	Can_Arc_StatisticsType stats;

	// Data stored for Txconfirmation callbacks to CanIf
	Can_CtrlPendingStateIndicationType pendingStateIndication;

	uint64 mxBoxBusyMask;
	PduIdType transmittedPduIds[64];
	uint8 MaxBoxes;
	uint32 WakeupSrc;
} Can_UnitType;




/* Used to switch between IF1 and IF2 of DCAN */
#define CAN_REG_IF_1 0
#define CAN_REG_IF_2 1
#define CAN_REG_IF_3 2

#define CAN_CTL_INTERRUPT_MASK ((1 << CAN_REG_CTL_IE0_BIT) | (1 << CAN_REG_CTL_IE1_BIT)| (0 << CAN_REG_CTL_SIE_BIT) | (1 << CAN_REG_CTL_EIE_BIT))
#define IF_MC_MASK(_dlc, _umask, _rxie, _txie, _eob) \
			(_dlc \
			| (_umask << CAN_REG_IFX_MCTL_UMASK_BIT) \
			| (_rxie  << CAN_REG_IFX_MCTL_RXIE_BIT) \
			| (_txie  << CAN_REG_IFX_MCTL_TXIE_BIT) \
			| (_eob   << CAN_REG_IFX_MCTL_EOB_BIT))

/* Used to order Data Bytes according to hardware registers in DCAN */
static const uint8 ElementIndex[] = { 3, 2, 1, 0, 7, 6, 5, 4 };


/* Shadow Buffer is used for buffering of received data */
static uint8 RxShadowBuf[CAN_ARC_CTRL_CONFIG_CNT][8];

/* Driver must know how often Can_DisableControllerInterrupts() has been called */
static uint32 IntDisableCount[CAN_ARC_CTRL_CONFIG_CNT];


// Controller private data.
Can_UnitType CanUnits[] = {
	{
		.state = CANIF_CS_UNINIT,
		.MaxBoxes = DCAN1_MAX_MESSAGEBOXES,
		.WakeupSrc = 0,
		.mxBoxBusyMask = 0,
	},
	{
		.state = CANIF_CS_UNINIT,
		.MaxBoxes = DCAN2_MAX_MESSAGEBOXES,
		.WakeupSrc = 0,
		.mxBoxBusyMask = 0
	},
	{
		.state = CANIF_CS_UNINIT,
		.MaxBoxes = DCAN3_MAX_MESSAGEBOXES,
		.WakeupSrc = 0,
		.mxBoxBusyMask = 0
	}
};

static inline uint8_t ilog2_64( uint64_t val ) {
    uint32_t t = (uint32_t)(val >> 32);

    if( t != 0) {
        return ilog2(t) + 32;
    }
    return ilog2((uint32_t)(0xFFFFFFFFul & val));
}

static inline const Can_HardwareObjectType * Can_FindTxHoh(Can_HwHandleType hth) {
	const Can_HardwareObjectType * hoh = &Can_Global.config->CanConfigSetPtr->ArcCanHardwareObjects[hth];
	if (hoh->CanObjectType != CAN_OBJECT_TYPE_TRANSMIT || hoh->CanObjectId != hth) {
		return 0;
	}
	return hoh;
}

static inline const Can_HardwareObjectType * Can_FindRxHoh(Can_HwHandleType hrh) {
	const Can_HardwareObjectType * hoh = &Can_Global.config->CanConfigSetPtr->ArcCanHardwareObjects[hrh];
	if (hoh->CanObjectType != CAN_OBJECT_TYPE_RECEIVE || hoh->CanObjectId != hrh) {
		return 0;
	}
	return hoh;
}

static inline const Can_HardwareObjectType * Can_FindHoh(Can_HwHandleType handle) {
	return &Can_Global.config->CanConfigSetPtr->ArcCanHardwareObjects[handle];
}

static inline uint8 Can_FindControllerId(Can_HwHandleType handle) {
	const Can_HardwareObjectType * hoh = &Can_Global.config->CanConfigSetPtr->ArcCanHardwareObjects[handle];
	return hoh->ArcCanControllerId;
}

static void Can_SetId(Can_IdTypeType idType, Can_RegisterType * regs,
		const Can_PduType *PduInfo) {
	if (idType == CAN_ID_TYPE_STANDARD) {
		/* Standard ID*/
		regs->IFx[CAN_REG_IF_1].ARB |= (PduInfo->id & 0x7FFU) << 18U;
	} else if (idType == CAN_ID_TYPE_EXTENDED) {
		/* Extended ID*/
		regs->IFx[CAN_REG_IF_1].ARB |= PduInfo->id & 0x1FFFFFFFU;
	} else {
		/*Mixed ID */
		if ((PduInfo->id & 0x80000000U) == (0x80000000U)) {
			/* Extended ID*/
			regs->IFx[CAN_REG_IF_1].ARB |= (1U << 30U);
			regs->IFx[CAN_REG_IF_1].ARB |= PduInfo->id & 0x1FFFFFFFU;
		} else {
			/* Standard ID*/
			regs->IFx[CAN_REG_IF_1].ARB &= ~(1U << 30U);
			regs->IFx[CAN_REG_IF_1].ARB |= (PduInfo->id & 0x7FFU) << 18U;
		}
	}
}

#define DCAN_MC_NEWDAT	15
#define DCAN_MC_EOB		7


/**
 * This function will handle fifo buffers as well as individual message rx objects.
 * Note that fifos must use consecutive message objects.
 *
 */
static inline void Can_HandleRxOk(uint8 MsgObjectNr, const Can_ControllerConfigType * controllerConfig, const Can_HardwareObjectType * hrh) {
	uint32 MsgId;
	uint8 MsgDlc;
	uint8 DataByteIndex;
	uint8 *SduPtr;
	uint32 mc;
	uint32 arb;



	Can_RegisterType * canRegs = CanRegs[controllerConfig->CanControllerId];

	do {
		// Setup hardware to read arbitration, control and data Bits of the message object.
		// Clear IntPnd and Tx

		canRegs->IFx[CAN_REG_IF_2].COM = (1 << CAN_REG_IFX_COM_DATAB_BIT)
				| (1 << CAN_REG_IFX_COM_DATAA_BIT)
				| (1 << CAN_REG_IFX_COM_NEWDAT_BIT)
				| (1 << CAN_REG_IFX_COM_CLEARINTPEND_BIT)
				| (1 << CAN_REG_IFX_COM_CONTROL_BIT)
				| (1 << CAN_REG_IFX_COM_ARB_BIT)
				| (MsgObjectNr);

		DCAN_WAIT_UNTIL_NOT_BUSY_NO_RV(controllerConfig->CanControllerId, CAN_REG_IF_2);

		// Read message control
		mc = canRegs->IFx[CAN_REG_IF_2].MC;
		arb = canRegs->IFx[CAN_REG_IF_2].ARB;

		// Is there a new message waiting?
		if ((mc & (1 << DCAN_MC_NEWDAT)) == (1 << DCAN_MC_NEWDAT)) {
			/* Extended Id */
			if ((arb & (1U << CAN_REG_IFX_ARB_XTD_BIT)) == (1U << CAN_REG_IFX_ARB_XTD_BIT)) {
				/* Bring Id to standardized format (MSB marks extended Id) */
				MsgId = (arb & 0x1FFFFFFFU) | 0x80000000U;

			} else { /* Standard Id */
				/* Bring Id to standardized format (MSB marks extended Id) */
				MsgId = (arb >> 18U) & 0x7FFU;
			}

			/* DLC (Max 8) */
			MsgDlc = mc & 0x0000000FU;
			if (MsgDlc > 8) {
				MsgDlc = 8;
			}

			/* Let SduPtr point to Shadow Buffer */
			SduPtr = RxShadowBuf[controllerConfig->CanControllerId];

			/* Copy Message Data to Shadow Buffer */
			for (DataByteIndex = 0; DataByteIndex < MsgDlc; DataByteIndex++) {
				SduPtr[DataByteIndex] =
						canRegs->IFx[CAN_REG_IF_2].DATx[ElementIndex[DataByteIndex]];
			}

			/* Indicate successful Reception */
			CanIf_RxIndication(hrh->CanObjectId, MsgId, MsgDlc, SduPtr);

			if (mc & (1 << DCAN_MC_EOB)) {
				break; // We have parsed the last object of this FIFO.
			}


		} else {
			break; // There is no new data in the fifo.

		}
		MsgObjectNr++;
	} while ((mc & (1 << DCAN_MC_EOB)) == 0);
}

static inline void Can_HandleTxOk(CanControllerIdType controllerNr, uint8 MsgNr) {
	Can_UnitType *canUnit = GET_PRIVATE_DATA(controllerNr);

	// Clear busy flag
	canUnit->mxBoxBusyMask &= ~(1ull << (MsgNr - 1));

	if (GET_CALLBACKS()->TxConfirmation != NULL) {
		GET_CALLBACKS()->TxConfirmation(canUnit->transmittedPduIds[(MsgNr - 1)]);
	}
}

static inline void Can_HandleBusOff(CanControllerIdType controllerNr) {
	//This will also indicate controller mode to CanIF, it probably should not
	Can_SetControllerMode(controllerNr, CAN_T_STOP); // CANIF272
}

static inline void Can_HandleWakeupPending() {
	/* Set Init Bit, so that Controller is in Stop state */
	//canRegs->CTL |= 0x1;
	// EcuM_CheckWakeUp(ControllerConfig[0].WakeupSrc);
}

static uint32 canInterruptErrors = 0;
static inline void Can_HandleInterruptError() {
	canInterruptErrors++;
}


void Can_InterruptHandler(CanControllerIdType controllerNr) {
	const Can_ControllerConfigType * controllerConfig = GET_CONTROLLER_CONFIG(controllerNr);
	Can_RegisterType * canRegs = CanRegs[controllerConfig->CanControllerId];
	
	uint32 ir = canRegs->IR;

	// Interrupt from status register
	if ((ir & 0x0000FFFF) == 0x8000) {
		uint32 sr = canRegs->SR;

		if (sr & (1 << CAN_REG_ES_TXOK_BIT)) {
			Can_HandleInterruptError();

		} else if (sr & (1 << CAN_REG_ES_RXOK_BIT)) {
			Can_HandleInterruptError(); // This is not the way to handle rx interrupts for us.

		} else if ((sr & (1 << CAN_REG_ES_BOFF_BIT)) == (1 << CAN_REG_ES_BOFF_BIT)) {
			Can_HandleBusOff(controllerNr);
			CanIf_ControllerBusOff(controllerNr);

		} else if ((sr & (1 << CAN_REG_ES_WKPEND_BIT)) == (1 << CAN_REG_ES_WKPEND_BIT)) {
			//No support for wakeup

		} else { // MISRA-C compliance.

		}

	} else {

		// ir contains the message object number causing the interrupt
		Can_HwHandleType hohHandle = controllerConfig->Can_Arc_MailBoxToHrh[ir-1];
		const Can_HardwareObjectType * hoh = Can_FindHoh(hohHandle);

		if (hoh->CanObjectType == CAN_OBJECT_TYPE_RECEIVE) {
			Can_HandleRxOk(ir, controllerConfig, hoh);

		} else {
			Can_HandleTxOk(controllerNr, ir);
		}

		// Clear interrupt pending flag.
		DCAN_WAIT_UNTIL_NOT_BUSY_NO_RV(controllerConfig->CanControllerId, CAN_REG_IF_2);

		canRegs->IFx[CAN_REG_IF_2].COM = (1 << CAN_REG_IFX_COM_CLEARINTPEND_BIT) | ir;

	}
}


ISR(Can1_Level0InterruptHandler) {
	Can_InterruptHandler(DCAN1);
}

ISR(Can2_Level0InterruptHandler) {
	Can_InterruptHandler(DCAN2);
}

ISR(Can3_Level0InterruptHandler) {
	Can_InterruptHandler(DCAN3);
}


static inline void Can_InstallInterruptHandlers(const Can_ControllerConfigType * ctrl) {
	// Install interrupt handlers
	if (ctrl->CanControllerId == DCAN1) {
		ISR_INSTALL_ISR2("DCAN1Level0", Can1_Level0InterruptHandler, CAN1_LVL0_INT, 2, 0);

	} else if (ctrl->CanControllerId == DCAN2) {
		ISR_INSTALL_ISR2("DCAN2Level0", Can2_Level0InterruptHandler, CAN2_LVL0_INT, 2, 0);

	} else if (ctrl->CanControllerId == DCAN3) {
		ISR_INSTALL_ISR2("DCAN3Level0", Can3_Level0InterruptHandler, CAN3_LVL0_INT, 2, 0);

	}
}

static inline uint32 Can_CalculateBTR(const Can_ControllerConfigType * ctrl) {

	uint32 clock = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_CAN);
	uint32 tq1 = ctrl->CanControllerPropSeg + ctrl->CanControllerSeg1;
	uint32 tq2 = ctrl->CanControllerSeg2;
	uint32 ntq = tq1 + tq2 + 1 + 1 + 1;
	uint32 brp = clock
			/ (ctrl->CanControllerDefaultBaudrate
					* 1000 * ntq) - 1;

	uint32 retVal = (brp | (tq1 << 8) | (tq2 << 12));
	return retVal;
}

static inline void Can_InitHardwareObjects( const Can_ControllerConfigType * controllerConfig) {
	uint8 rx;
	uint8 tx;
	uint8 extended;

	Can_RegisterType * canCtrlRegs = CanRegs[controllerConfig->CanControllerId];
	/* Configure the HOHs for this controller. */
	const Can_HardwareObjectType* hoh;
	hoh = controllerConfig->Can_Arc_Hoh;
	hoh--;
	do {
		hoh++;

		if (hoh->ArcCanControllerId != controllerConfig->CanControllerId) {
			continue;
		}

		if ((hoh->CanObjectType == CAN_OBJECT_TYPE_RECEIVE)) {
			rx = 1;
			tx = 0;
		} else {
			rx = 0;
			tx = 1;
		}

		if (hoh->CanIdType == CAN_ID_TYPE_EXTENDED) {
			extended = 1;
		} else {
			extended = 0;
		}

		// For every message object in this hoh
		uint64  mbMask = hoh->ArcMailboxMask;
		uint8   mbNr = 0;
		uint8 mbN = 0;
		for (; mbMask; mbMask &= ~(1ull << mbNr)) {
			mbNr = ilog2_64(mbMask);
			mbN++;

			// Check if this is the last message box for this hoh.
			uint8 eob = 1;
			if (hoh->CanObjectType == CAN_OBJECT_TYPE_RECEIVE && mbN != (hoh->Can_Arc_NrMessageBoxes) ) {
				// EndOfBlock Bit will not be set
				eob = 0;
			}

			/* Setup mask register */
			//canCtrlRegs->IFx[CAN_REG_IF_1].MASK &= 0xD0000000;
			//canCtrlRegs->IFx[CAN_REG_IF_1].MASK |= hoh->CanHwFilterMask & 0x1FFFFFFF;
			canCtrlRegs->IFx[CAN_REG_IF_1].MASK = 0x0;

			/* Setup arbitration register */
			canCtrlRegs->IFx[CAN_REG_IF_1].ARB = (1 << CAN_REG_IFX_ARB_MSGVAL_BIT)
									| (extended << CAN_REG_IFX_ARB_XTD_BIT)
									| (tx << CAN_REG_IFX_ARB_DIR_BIT);
									//| (hoh->CanHwFilterMask & 0x1FFFFFFF)

			/* Setup message control register */
			canCtrlRegs->IFx[CAN_REG_IF_1].MC = IF_MC_MASK(8, rx, rx, tx, eob);

			/* Setup command register */
			canCtrlRegs->IFx[CAN_REG_IF_1].COM = 0x00F00000 | (mbNr + 1);

			DCAN_WAIT_UNTIL_NOT_BUSY_NO_RV(controllerConfig->CanControllerId, CAN_REG_IF_1);
		}
	} while (!hoh->Can_Arc_EOL);
}

void Can_Init(const Can_ConfigType *Config) {

	imask_t state;

	/* DET Error Check */
	VALIDATE_NO_RV( (Can_Global.initRun == CAN_UNINIT), CAN_INIT_SERVICE_ID, CAN_E_TRANSITION );
	VALIDATE_NO_RV( (Config != NULL ), CAN_INIT_SERVICE_ID, CAN_E_PARAM_POINTER );

	Irq_Save(state);

	Can_Global.config = Config;
	Can_Global.initRun = CAN_READY;


	for (uint8 canCtrlNr = 0; canCtrlNr < CAN_CONTROLLER_CNT; canCtrlNr++) {

		const Can_ControllerConfigType * controllerConfig = GET_CONTROLLER_CONFIG(canCtrlNr);

		// Assign the configuration channel used later..
		Can_Global.channelMap[controllerConfig->CanControllerId] = canCtrlNr;
		Can_Global.configured |= (1 << controllerConfig->CanControllerId);

		Can_UnitType * canUnit = GET_PRIVATE_DATA(controllerConfig->CanControllerId);
		canUnit->state = CANIF_CS_STOPPED;

		Can_RegisterType *canCtrlRegs = CanRegs[controllerConfig->CanControllerId];

		canCtrlRegs->CTL |= (1U << CAN_REG_CTL_INIT_BIT);
		canCtrlRegs->CTL |= (1U << CAN_REG_CTL_SWR_BIT); /*perform sw reset*/

		// 1. Set init bit in CTL. Set CCE.
		canCtrlRegs->CTL =
				(1 << CAN_REG_CTL_INIT_BIT) | // Init enable
				(1 << CAN_REG_CTL_CCE_BIT)  | // CCE enable
				(1 << CAN_REG_CTL_ABO_BIT)  | // ABO enable
				(0x5 << CAN_REG_CTL_PARITY_BIT) | // Parity disable
				(WUBA_STATUS << CAN_REG_CTL_WUBA_BIT) | // WUBA on or off
				CAN_CTL_INTERRUPT_MASK |
				(controllerConfig->Can_Arc_Loopback << CAN_REG_CTL_TEST_MODE_BIT) | // Test mode enable/disable
				(0 << CAN_REG_CTL_DAR_BIT); // Enable automatic retransmission.

		/* Clear all pending error flags and reset current status - only reading clears the flags */
		canCtrlRegs->SR = canCtrlRegs->SR;


		// Wait for Init = 1. Don't seem to need this.

		/* LEC 7, TxOk, RxOk, PER */
		//CanRegs[Controller]->SR = 0x0000011F;

		/* Test Mode only for Development time: Silent Loopback */
		if (controllerConfig->Can_Arc_Loopback) {
			canCtrlRegs->TR = (1 << CAN_REG_TR_LBACK_BIT);
		}

		Can_InitHardwareObjects(controllerConfig);

		/* Set CCE Bit to allow access to BitTiming Register (Init already set, in mode "stopped") */
		canCtrlRegs->CTL |= 0x00000040;
		/* Set Bit Timing Register */
		canCtrlRegs->BTR = Can_CalculateBTR(controllerConfig);
		/* Reset CCE Bit */
		canCtrlRegs->CTL &= ~((1 << CAN_REG_CTL_INIT_BIT) | (1 << CAN_REG_CTL_CCE_BIT));

		Can_InstallInterruptHandlers(controllerConfig);

	}

	Irq_Restore(state);
	Can_Global.initRun = CAN_READY;

}

// Unitialize the module
void Can_Arc_DeInit() {

	return;
}

Std_ReturnType Can_ChangeBaudrate(uint8 Controller, const uint16 Baudrate) {
	/** @req 4.0.3/CAN449 *//** @req 4.1.2/SWS_Can_00449 */

	/** @req 4.0.3/CAN450 *//** @req 4.1.2/SWS_Can_00450 */
	VALIDATE( (Can_Global.initRun == CAN_READY), CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_UNINIT );

	/** @req 4.0.3/CAN452 *//** @req 4.1.2/SWS_Can_00452 */
	VALIDATE( (Controller < CAN_ARC_CTRL_CONFIG_CNT), CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_CONTROLLER );

	/** @req 4.0.3/CAN453 *//** @req 4.1.2/SWS_Can_00453 */
	Can_UnitType *canUnit = GET_PRIVATE_DATA(Controller);
	VALIDATE( (canUnit->state==CANIF_CS_STOPPED), CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_TRANSITION );

	/** @req 4.0.3/CAN451 *//** @req 4.1.2/SWS_Can_00451 */
	VALIDATE(Can_CheckBaudrate(Controller, Baudrate) == CAN_OK, CAN_CHANGE_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_BAUDRATE);

	return E_NOT_OK ; // Not implemented.
}


Can_ReturnType Can_SetControllerMode(uint8 Controller,
		Can_StateTransitionType Transition) {
	Can_ReturnType Status = CAN_OK;
	uint32 ErrCounter = CAN_TIMEOUT_DURATION;
	uint32 RegBuf;

	/** @req 4.0.3/CAN199 *//** @req 4.1.2/SWS_Can_00199 */
	VALIDATE( (Controller < CAN_CONTROLLER_CNT), CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_PARAM_CONTROLLER );

	/** @req 4.0.3/CAN198 *//** @req 4.1.2/SWS_Can_00198 */
	VALIDATE( (Can_Global.initRun == CAN_READY), CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_UNINIT );

	Can_UnitType *canUnit = GET_PRIVATE_DATA(Controller);
	const Can_ControllerConfigType * controllerConfig = GET_CONTROLLER_CONFIG(Controller);
	Can_RegisterType *canCtrlRegs = CanRegs[controllerConfig->CanControllerId];

	canUnit->pendingStateIndication = CAN_CTRL_INDICATION_NONE;

	switch (Transition) {
	case CAN_T_START:
		/** @req 4.0.3/CAN200 *//** @req 4.1.2/SWS_Can_00200 */
		/** @req 4.0.3/CAN409 *//** @req 4.1.2/SWS_Can_00409 */
		VALIDATE(canUnit->state == CANIF_CS_STOPPED, CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_TRANSITION);

		/* Clear Init Bit */
		canCtrlRegs->CTL &= ~0x00000001;
		/* Clear Status Register */
		canCtrlRegs->SR = 0x0000011F;

		canUnit->state = CANIF_CS_STARTED;
		Can_EnableControllerInterrupts(Controller);

		canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_START;
		break;

	case CAN_T_WAKEUP:
		/** @req 4.0.3/CAN412 *//** @req 4.1.2/SWS_Can_00412 */
		VALIDATE(canUnit->state == CANIF_CS_SLEEP || canUnit->state == CANIF_CS_STOPPED, CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_TRANSITION);
		/* Clear PDR Bit */
		canCtrlRegs->CTL &= ~0x01000000;
		canUnit->state = CANIF_CS_STOPPED;
		canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_STOP;
		break;

	case CAN_T_STOP:
		/* Set Init Bit */
		canCtrlRegs->CTL |= 0x00000001;
		canUnit->state = CANIF_CS_STOPPED;
		Can_DisableControllerInterrupts(Controller);
		canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_STOP;
		break;

	case CAN_T_SLEEP:
		/** @req 4.0.3/CAN411 *//** @req 4.1.2/SWS_Can_00411 */
		VALIDATE(canUnit->state == CANIF_CS_STOPPED, CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_TRANSITION);
		/* Set PDR  Bit */
		canCtrlRegs->CTL |= 0x01000000;
		/* Save actual Register status */
		RegBuf = canCtrlRegs->CTL;
		/* Disable Status Interrupts and WUBA */
		canCtrlRegs->CTL &= ~0x02000004;
		/* Wait until Local Power Down Mode acknowledged */
		while (!(canCtrlRegs->SR & 0x00000400)) {
			/* Check if a WakeUp occurs */
			if (canCtrlRegs->SR & 0x00000200) {
				Status = CAN_NOT_OK;
				break;
			}
			ErrCounter--;
			if (ErrCounter == 0) {
				Dem_ReportErrorStatus(DemConf_DemEventParameter_CAN_E_TIMEOUT, DEM_EVENT_STATUS_FAILED);
				ErrCounter = CAN_TIMEOUT_DURATION;
				Status = CAN_NOT_OK;
				break;
			}
		}
		/* Reset Control Register */
		canCtrlRegs->CTL = RegBuf;
		canUnit->state = CANIF_CS_SLEEP;
		canUnit->pendingStateIndication = CAN_CTRL_INDICATION_PENDING_SLEEP;
		break;

	default:
		/** @req 4.0.3/CAN200 *//** @req 4.1.2/SWS_Can_00200 */
		VALIDATE(canUnit->state == CANIF_CS_STOPPED, CAN_SETCONTROLLERMODE_SERVICE_ID, CAN_E_TRANSITION);
		break;
	}

	return Status;
}

void Can_DisableControllerInterrupts(uint8 Controller) {

	/** @req 4.0.3/CAN206 *//** @req 4.1.2/SWS_Can_00206*/
	VALIDATE_NO_RV( (Controller < CAN_CONTROLLER_CNT), CAN_DISABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_PARAM_CONTROLLER );

	/** @req 4.0.3/CAN209 *//** @req 4.1.2/SWS_Can_00209 */
	VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_DISABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_UNINIT );

	Can_RegisterType *canCtrlRegs = CanRegs[Controller];

	// Set init bit in CTL
	canCtrlRegs->CTL |= (1 << CAN_REG_CTL_INIT_BIT);

	canCtrlRegs->CTL &= ~CAN_CTL_INTERRUPT_MASK;

	// Clear init bit in CTL
	canCtrlRegs->CTL &= ~(1 << CAN_REG_CTL_INIT_BIT);

	/* Increment Disable Counter */
	IntDisableCount[Controller]++;
}

void Can_EnableControllerInterrupts(uint8 Controller) {
	/** @req 4.0.3/CAN210 *//** @req 4.1.2/SWS_Can_00210 */
	VALIDATE_NO_RV( (Controller < CAN_CONTROLLER_CNT), CAN_ENABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_PARAM_CONTROLLER );

	/** @req 4.0.3/CAN209 *//** @req 4.1.2/SWS_Can_00209 */
	VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_ENABLECONTROLLERINTERRUPTS_SERVICE_ID, CAN_E_UNINIT );

	if (IntDisableCount[Controller] > 0) {
		if (IntDisableCount[Controller] == 1) {

			Can_RegisterType *canCtrlRegs = CanRegs[Controller];

			// Set init bit in CTL
			canCtrlRegs->CTL |= (1 << CAN_REG_CTL_INIT_BIT);

			/* Set IE */
			canCtrlRegs->CTL |= CAN_CTL_INTERRUPT_MASK;

			// Clear init bit in CTL
			canCtrlRegs->CTL &= ~(1 << CAN_REG_CTL_INIT_BIT);
		}
		IntDisableCount[Controller]--;
	}
}


Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType *PduInfo) {
	uint8 ControllerId;
	uint32 ArbRegValue;
	uint8 DataByteIndex;
	imask_t state;

	/** @req 4.0.3/CAN216 *//** @req 4.1.2/SWS_Can_00216 */
	VALIDATE( (Can_Global.initRun == CAN_READY), CAN_WRITE_SERVICE_ID, CAN_E_UNINIT );
	/** @req 4.0.3/CAN219 *//** @req 4.1.2/SWS_Can_00219 */
	VALIDATE( (PduInfo != NULL), CAN_WRITE_SERVICE_ID, CAN_E_PARAM_POINTER );
	VALIDATE( (PduInfo->sdu != NULL), CAN_WRITE_SERVICE_ID, CAN_E_PARAM_POINTER );
	/** @req 4.0.3/CAN218 *//** @req 4.1.2/SWS_Can_00218 */
	VALIDATE( (PduInfo->length <= 8), CAN_WRITE_SERVICE_ID, CAN_E_PARAM_DLC );

	ControllerId = Can_FindControllerId(Hth);
	const Can_HardwareObjectType *hoh = Can_FindTxHoh(Hth);

	/** @req 4.0.3/CAN217 *//** @req 4.1.2/SWS_Can_00217 */
	VALIDATE( (Hth < NUM_OF_HOHS ), CAN_WRITE_SERVICE_ID, CAN_E_PARAM_HANDLE );
	VALIDATE( (hoh != NULL ), CAN_WRITE_SERVICE_ID, CAN_E_PARAM_HANDLE );


    if (hoh == NULL) {
    	Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE, 6, CAN_E_PARAM_HANDLE);
        return CAN_NOT_OK;
    }

	if(hoh->CanObjectType != CAN_OBJECT_TYPE_TRANSMIT) {
		Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE, 6, CAN_E_PARAM_HANDLE);
		return CAN_NOT_OK;
	}

	const Can_ControllerConfigType * controllerConfig = GET_CONTROLLER_CONFIG(ControllerId);
	Can_UnitType * canUnit = GET_PRIVATE_DATA(controllerConfig->CanControllerId);
	Can_RegisterType *canCtrlRegs = CanRegs[controllerConfig->CanControllerId];

	// We cannot allow an interrupt or other task to play with the COM, MC and ARB registers here.
	Irq_Save(state);

	/* Bring Id Value to appropriate format and set ArbRegValue */
	if (hoh->CanIdType == CAN_ID_TYPE_EXTENDED) {
		/* MsgVal, Ext, Transmit, Extended Id */
		ArbRegValue = ((1U << ( CAN_REG_IFX_ARB_MSGVAL_BIT )) |
		                               (1U << ( CAN_REG_IFX_ARB_XTD_BIT )) |
		                               (1U << ( CAN_REG_IFX_ARB_DIR_BIT )));

	} else {
		/* MsgVal, Std, Transmit, Standard Id */
		ArbRegValue = ((1U << ( CAN_REG_IFX_ARB_MSGVAL_BIT )) |
		                   (1U << ( CAN_REG_IFX_ARB_DIR_BIT )));
	}

	DCAN_WAIT_UNTIL_NOT_BUSY(ControllerId, CAN_REG_IF_1);


	/* Set NewDat, TxIE (dep on ControllerConfig), TxRqst, EoB and DLC */
	canCtrlRegs->IFx[CAN_REG_IF_1].MC = IF_MC_MASK(PduInfo->length, 0, 0, 1, 1);

	/* Set ArbitrationRegister */
	canCtrlRegs->IFx[CAN_REG_IF_1].ARB = ArbRegValue;
	Can_SetId(hoh->CanIdType, canCtrlRegs, PduInfo);

	/* Set Databytes */
	for (DataByteIndex = 0; DataByteIndex < PduInfo->length; DataByteIndex++) {
		canCtrlRegs->IFx[CAN_REG_IF_1].DATx[ElementIndex[DataByteIndex]] = PduInfo->sdu[DataByteIndex];
	}

	/* Find first free mb box */
	uint64  mbMask = hoh->ArcMailboxMask;
	uint64   mbNr = 0;
	for (; mbMask; mbMask &= ~(1ull << mbNr)) {
		mbNr = ilog2_64(mbMask);
		if (!(canUnit->mxBoxBusyMask & mbMask)) break;
	}

	canUnit->mxBoxBusyMask |= (1ull << mbNr); // Mark selected box as busy.
	canUnit->transmittedPduIds[mbNr] = PduInfo->swPduHandle;

	/* Start transmission to MessageRAM */
	canCtrlRegs->IFx[CAN_REG_IF_1].COM = 0x00BF0000 | (mbNr + 1);

	Irq_Restore(state);
	return CAN_OK;
}

#if (CAN_USE_WRITE_POLLING == STD_ON)
void Can_MainFunction_Write()
{
#error "Polling write not supported by this driver."
	/** !req 4.0.3/CAN225 *//** !req 4.1.2/SWS_Can_00225 */
	/** !req 4.0.3/CAN031 *//** !req 4.1.2/SWS_Can_00031 */
	/** !req 4.1.2/SWS_Can_00441 */
	/** !req 4.0.3/CAN179 *//** !req 4.1.2/SWS_Can_00179 */
}
#endif

#if (CAN_USE_READ_POLLING == STD_ON)
void Can_MainFunction_Read()
{
#error "Polling read not supported by this driver."
	/** !req 4.0.3/CAN226 *//** !req 4.1.2/SWS_Can_00226 */
	/** !req 4.0.3/CAN108 *//** !req 4.1.2/SWS_Can_00108 */
	/** !req 4.1.2/SWS_Can_00442 */
	/** !req 4.0.3/CAN181 *//** !req 4.1.2/SWS_Can_00181 */
}
#endif

#if (CAN_USE_BUSOFF_POLLING == STD_ON)
void Can_MainFunction_BusOff()
{
#error "Polling bus off not supported by this driver."
	/** !req 4.0.3/CAN227 *//** !req 4.1.2/SWS_Can_00227 */
	/** !req 4.0.3/CAN109 *//** !req 4.1.2/SWS_Can_00109 */
	/** !req 4.0.3/CAN184 *//** !req 4.1.2/SWS_Can_00184 */
}
#endif

#if (CAN_USE_WAKEUP_POLLING == STD_ON)
void Can_MainFunction_Wakeup()
{
#error "Polling wakeup not supported by this driver."
	/** !req 4.0.3/CAN228 *//** !req 4.1.2/SWS_Can_00228 */
	/** !req 4.0.3/CAN112 *//** !req 4.1.2/SWS_Can_00112 */
	/** !req 4.0.3/CAN186 *//** !req 4.1.2/SWS_Can_00186 */
}
#endif

Std_ReturnType Can_CheckBaudrate(uint8 Controller, const uint16 Baudrate) {
	/** @req 4.0.3/CAN454 *//** @req 4.1.2/SWS_Can_00454 *//* API */

	// Checks that the target baudrate is found among the configured
	// baudrates for this controller.
	const Can_ControllerConfigType *config;
	boolean supportedBR;

	/** @req 4.0.3/CAN456 UNINIT */
	VALIDATE( (Can_Global.initRun == CAN_READY), CAN_CHECK_BAUD_RATE_SERVICE_ID, CAN_E_UNINIT );
	/** @req 4.0.3/CAN457 Invalid controller */
	VALIDATE( Can_Global.configured & (1<<Controller) , CAN_CHECK_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_CONTROLLER );

	/** @req 4.0.3/CAN458 Invalid baudrate value */
	VALIDATE( Baudrate <= 1000, CAN_CHECK_BAUD_RATE_SERVICE_ID, CAN_E_PARAM_BAUDRATE );

	config = GET_CONTROLLER_CONFIG(Can_Global.channelMap[Controller]);

	// Find the baudrate config for the target baudrate
	supportedBR = FALSE;
	for (int i = 0; i < config->CanControllerSupportedBaudratesCount; i++) {
		if (config->CanControllerSupportedBaudrates[i].CanControllerBaudRate
				== Baudrate) {
			supportedBR = TRUE;
		}
	}

	return supportedBR ? E_OK : E_NOT_OK;
}

void Can_MainFunction_Mode(void) {
	/** @req 4.0.3/CAN368 *//** @req 4.1.2/SWS_Can_00368 *//* API */
	/** @req 4.0.3/CAN369 *//** @req 4.1.2/SWS_Can_00369 *//* Polling */

	/** !req 4.0.3/CAN398 *//** !req 4.1.2/SWS_Can_00398 *//* Blocking call not supported */
	/** !req 4.0.3/CAN371 *//** !req 4.1.2/SWS_Can_00371 *//* Blocking call not supported */
	/** !req 4.0.3/CAN372 *//** !req 4.1.2/SWS_Can_00372 *//* Blocking call not supported */


	/** @req 4.0.3/CAN379 *//** @req 4.1.2/SWS_Can_00379 *//* CAN_E_UNINIT */
	VALIDATE_NO_RV( (Can_Global.initRun == CAN_READY), CAN_MAIN_FUNCTION_MODE_SERVICE_ID, CAN_E_UNINIT );

	/** @req 4.0.3/CAN431 *//* On Uninit, return immediately without prod err */
	if (Can_Global.initRun == CAN_UNINIT) {
		return;
	}
	for (int configId = 0; configId < CAN_ARC_CTRL_CONFIG_CNT; configId++) {
		boolean clearPending = TRUE;
		CanIf_ControllerModeType indicateMode = CANIF_CS_UNINIT;
		//Get the controller from config index and index CanUnit
		const Can_ControllerConfigType * canHwConfig = GET_CONTROLLER_CONFIG(configId);

		Can_UnitType * canUnit = GET_PRIVATE_DATA(canHwConfig->CanControllerId);

		switch (canUnit->pendingStateIndication) {
		case CAN_CTRL_INDICATION_PENDING_START:
			if (canUnit->state == CANIF_CS_STARTED) {
				indicateMode = CANIF_CS_STARTED;
			}
			break;

		case CAN_CTRL_INDICATION_PENDING_STOP:
			if (canUnit->state == CANIF_CS_STOPPED) {
				// Stopped, indicate to upper layer
				indicateMode = CANIF_CS_STOPPED;
			}
			break;

		case CAN_CTRL_INDICATION_PENDING_SLEEP:
			if (canUnit->state == CANIF_CS_SLEEP) {
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
		if ((CANIF_CS_UNINIT != indicateMode)
				&& (NULL != GET_CALLBACKS()->ControllerModeIndication)) {
			/** @req 4.0.3/CAN370 *//** @req 4.1.2/SWS_Can_00370 */
			/** @req 4.0.3/CAN373 *//** @req 4.1.2/SWS_Can_00373 */
#if defined(CFG_CAN_USE_SYMBOLIC_CANIF_CONTROLLER_ID)
			GET_CALLBACKS()->ControllerModeIndication(canHwConfig->Can_Arc_CanIfControllerId, indicateMode);
#else
			GET_CALLBACKS()->ControllerModeIndication(
					canHwConfig->CanControllerId, indicateMode);
#endif
		}
		if (clearPending) {
			canUnit->pendingStateIndication = CAN_CTRL_INDICATION_NONE;
		}
	}
}

#if ( CAN_VERSION_INFO_API == STD_ON )
void Can_GetVersionInfo( Std_VersionInfoType* versioninfo) {

	/* @req SWS_Can_00224 */
	/* @req SWS_Can_00177 */
	VALIDATE_NO_RV( ( NULL != versioninfo ), CAN_GETVERSIONINFO_SERVICE_ID, CAN_E_PARAM_POINTER);

	versioninfo->vendorID = CAN_VENDOR_ID;
	versioninfo->moduleID = CAN_MODULE_ID;
	versioninfo->sw_major_version = CAN_SW_MAJOR_VERSION;
	versioninfo->sw_minor_version = CAN_SW_MINOR_VERSION;
	versioninfo->sw_patch_version = CAN_SW_PATCH_VERSION;
}
#endif

