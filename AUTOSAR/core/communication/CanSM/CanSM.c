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

//lint -emacro(904,CANSM_VALIDATE_INIT,CANSM_VALIDATE_NETWORK,CANSM_VALIDATE_POINTER,CANSM_VALIDATE_MODE) //904 PC-Lint exception to MISRA 14.7 (validate macros).

/* Globally fulfilled requirements */
/** @req CANSM069 */

//#include "CanSM_Cfg.h"          /* @req CANSM254  @req CANSM025.configuration */
#include "ComStack_Types.h"
#include "CanSM.h"              /* @req CANSM013 */
#include "ComM.h"               /* @req CANSM174 */
#include "ComM_BusSm.h"         /* @req CANSM191 */
#include "Det.h"                /* @req CANSM015 */
#if defined(USE_DEM)
#include "Dem.h"                /* @req CANSM014 */
#endif
#include "CanIf.h"              /* @req CANSM017 */
#include "CanSM_Internal.h"
#include "CanSM_BswM.h"			/* @req CANSM348 */
#if defined(USE_BSWM)
#include "BswM.h"
#endif

#if defined(USE_CANSM_EXTENSION)
#include "CanSM_Extension.h"
#endif

#define INVALID_CONTROLLER_ID 0xff

#if defined(USE_DEM)
#define DEM_REPORT_ERROR_STATUS(_event,_eventstatus) Dem_ReportErrorStatus(_event,_eventstatus);
#endif

static CanSM_Internal_CtrlStatusType CanSM_InternalControllerStatus[NOF_CANSM_CANIF_CONTROLLERS];
static CanSM_Internal_NetworkType CanSM_InternalNetworks[CANSM_NETWORK_COUNT];

static CanSM_InternalType CanSM_Internal = {
		.InitStatus = CANSM_STATUS_UNINIT,
		.Networks = CanSM_InternalNetworks,
		.ControllerModeBuf = CanSM_InternalControllerStatus,
};

#ifdef HOST_TEST
void GetInternals(CanSM_InternalType **ptr){
	*ptr = &CanSM_Internal;
}
#endif

const CanSM_ConfigType* CanSM_ConfigPtr;

static Std_ReturnType CanSM_Internal_GetCanSMControllerIndex(const uint8 controller, uint8* indexPtr)
{
    Std_ReturnType ret = E_NOT_OK;
    for(uint8 index = 0; index < NOF_CANSM_CANIF_CONTROLLERS; index++) {
        if( controller == CanSM_Internal.ControllerModeBuf[index].controllerId ) {
            *indexPtr = index;
            ret = E_OK;
            break;
        }
    }
    return ret;
}
static void CanSM_Internal_ResetControllerIndications(NetworkHandleType NetworkHandle)
{
    const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];
    for(uint8 ctrl = 0; ctrl < NetworkCfg->ControllerCount; ctrl++) {
        uint8 index;
        if( E_OK == CanSM_Internal_GetCanSMControllerIndex(NetworkCfg->Controllers[ctrl].CanIfControllerId, &index) ) {
            CanSM_Internal.ControllerModeBuf[index].indCtrlMode = CANIF_CS_UNINIT;
        }
    }
}

static void CanSM_Internal_EnterState(NetworkHandleType NetworkHandle, CanSM_Internal_BsmStateType state)
{
    CanSM_Internal_NetworkType *Network = &CanSM_Internal.Networks[NetworkHandle];
    Network->RepeatCounter = 0;
    Network->subStateTimer = 0;
    Network->BsmState = state;
    switch(state) {
        case CANSM_BSM_S_NOT_INITIALIZED:
            break;
        case CANSM_BSM_S_PRE_NOCOM:
            Network->PreNoCommState = CANSM_PRENOCOMM_S_CC_STOPPED;
            CanSM_Internal_ResetControllerIndications(NetworkHandle);
            break;
        case CANSM_BSM_S_NOCOM:
            Network->initialNoComRun = TRUE;
            break;
        case CANSM_BSM_S_PRE_FULLCOM:
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
            Network->PreFullCommState = CANSM_PREFULLCOMM_S_TRCV_NORMAL;
#else
            Network->PreFullCommState = CANSM_PREFULLCOMM_S_CC_STOPPED;
#endif
            break;
        case CANSM_BSM_S_FULLCOM:
            Network->FullCommState = CANSM_FULLCOMM_S_BUSOFF_CHECK;
            break;
        case CANSM_BSM_S_SILENTCOM:
            break;
        default:
            break;
    }
}

static boolean CanSM_Internal_HasPendingControllerIndication(NetworkHandleType NetworkHandle)
{
    boolean hasPendingIndication = FALSE;
    const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];

    for(uint8 ctrl = 0; ctrl < NetworkCfg->ControllerCount; ctrl++) {
        uint8 ctrlIndex;
        if( E_OK != CanSM_Internal_GetCanSMControllerIndex(NetworkCfg->Controllers[ctrl].CanIfControllerId, &ctrlIndex) ) {
            return TRUE;
        }
        if( CanSM_Internal.ControllerModeBuf[ctrlIndex].hasPendingCtrlIndication ) {
            /* No indication received after last request */
            hasPendingIndication = TRUE;
        }

    }
    return hasPendingIndication;
}

/* @req CANSM023 */
void CanSM_Init( const CanSM_ConfigType* ConfigPtr ) {

	/* @req CANSM179 */
	if( NULL == ConfigPtr ) {
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_INIT, CANSM_E_PARAM_POINTER);
		return;
	}

	CanSM_ConfigPtr = ConfigPtr;

	for( uint8 ctrl = 0; ctrl < NOF_CANSM_CANIF_CONTROLLERS; ctrl++ ) {
		CanSM_Internal.ControllerModeBuf[ctrl].controllerId = INVALID_CONTROLLER_ID;
		CanSM_Internal.ControllerModeBuf[ctrl].indCtrlMode = CANIF_CS_UNINIT;
		CanSM_Internal.ControllerModeBuf[ctrl].hasPendingCtrlIndication = FALSE;
	}
	for (uint8 i = 0; i < CANSM_NETWORK_COUNT; ++i) {
		CanSM_Internal.Networks[i].requestedMode = COMM_NO_COMMUNICATION;
		CanSM_Internal.Networks[i].currentMode = COMM_NO_COMMUNICATION;
		CanSM_Internal.Networks[i].BsmState = CANSM_BSM_S_NOT_INITIALIZED;
		CanSM_Internal.Networks[i].FullCommState = CANSM_FULLCOMM_S_BUSOFF_CHECK;
		CanSM_Internal.Networks[i].initialNoComRun = FALSE;
		CanSM_Internal.Networks[i].RepeatCounter = 0;
		CanSM_Internal.Networks[i].requestAccepted = FALSE;
		CanSM_Internal.Networks[i].busoffCounter = 0;

#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
		CanSM_Internal.Networks[i].TransceiverModeBuf.trcvId = CanSM_ConfigPtr->Networks[i].CanIfTransceiverId ;
		CanSM_Internal.Networks[i].TransceiverModeBuf.hasPendingTrcvIndication = FALSE;
		CanSM_Internal.Networks[i].TransceiverModeBuf.indTrcvMode = CANTRCV_TRCVMODE_NORMAL;
		CanSM_Internal.Networks[i].PreNoCommState = CANSM_PRENOCOMM_S_TRCV_NORMAL;
        CanSM_Internal.Networks[i].PreFullCommState = CANSM_PREFULLCOMM_S_TRCV_NORMAL;
		busoffNotifyPending[i] = FALSE;
#else
		CanSM_Internal.Networks[i].PreNoCommState = CANSM_PRENOCOMM_S_CC_STOPPED;
		CanSM_Internal.Networks[i].PreFullCommState = CANSM_PREFULLCOMM_S_CC_STOPPED;
#endif

		for(uint8 ctrl = 0; ctrl < CanSM_ConfigPtr->Networks[i].ControllerCount; ctrl++) {
			for(uint8 index = 0; index < NOF_CANSM_CANIF_CONTROLLERS; index++) {
				if( CanSM_ConfigPtr->Networks[i].Controllers[ctrl].CanIfControllerId ==
					CanSM_Internal.ControllerModeBuf[index].controllerId ) {
					/* Controller already indexed */
					break;
				} else if( INVALID_CONTROLLER_ID == CanSM_Internal.ControllerModeBuf[index].controllerId ) {
					/* Empty slot, insert controller id */
					CanSM_Internal.ControllerModeBuf[index].controllerId = CanSM_ConfigPtr->Networks[i].Controllers[ctrl].CanIfControllerId;
					break;
				}
			}
		}
	}

	CanSM_Internal.InitStatus = CANSM_STATUS_INIT;

}

/* @req CANSM064 */
void CanSM_ControllerBusOff(uint8 ControllerId)
{
    /* @req CANSM190 */
    if( CANSM_STATUS_UNINIT == CanSM_Internal.InitStatus ) {
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_CONTROLLERBUSOFF, CANSM_E_UNINIT);
        return;
    }

    Std_ReturnType status = E_NOT_OK;

    // Find which network has this controller
    for (uint8 i = 0; i < CANSM_NETWORK_COUNT; ++i) {
        const CanSM_NetworkType* Network = &CanSM_ConfigPtr->Networks[i];
        for (uint8 j = 0; j < Network->ControllerCount; ++j) {
            const CanSM_ControllerType* ptrController = &Network->Controllers[j];
            if(ptrController->CanIfControllerId == ControllerId)
            {
                if (CANSM_BSM_S_FULLCOM == CanSM_Internal.Networks[i].BsmState)
                {
                    /* @req CANSM235 */
                    CanSM_Internal.Networks[i].busoffevent = TRUE;
                }
                else {
                    CANSM_DET_REPORTERROR(CANSM_SERVICEID_CONTROLLERBUSOFF, CANSM_E_INVALID_INVALID_BUSOFF);
                }
                status = E_OK;
            }
        }
    }

    // Check if controller was valid
    if(status != E_OK){
        /* @req CANSM189 */
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_CONTROLLERBUSOFF, CANSM_E_PARAM_CONTROLLER);
    }
}

/* @req CANSM062 */
Std_ReturnType CanSM_RequestComMode( NetworkHandleType NetworkHandle, ComM_ModeType ComM_Mode ) {

    /* @req CANSM182 */

    /* @req CANSM369 */
    /* @req CANSM370 */

    /* @req CANSM278 */
    if( CANSM_STATUS_UNINIT == CanSM_Internal.InitStatus ) {
        /* @req CANSM184 */
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_REQUESTCOMMODE, CANSM_E_UNINIT);
        return E_NOT_OK;
    }

    if( NetworkHandle >= CANSM_NETWORK_COUNT) {
        /* @req CANSM183 */
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_REQUESTCOMMODE, CANSM_E_INVALID_NETWORK_HANDLE);
        return E_NOT_OK;
    }
    /* Not a requirement but still.. */
    if( (COMM_NO_COMMUNICATION != ComM_Mode) && (COMM_SILENT_COMMUNICATION != ComM_Mode) && (COMM_FULL_COMMUNICATION != ComM_Mode) ) {
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_REQUESTCOMMODE, CANSM_E_INVALID_NETWORK_MODE);
        return E_NOT_OK;
    }
    CanSM_Internal_NetworkType *Network = &CanSM_Internal.Networks[NetworkHandle];

    /* @req CANSM278 */
    if( CANSM_BSM_S_NOT_INITIALIZED == Network->BsmState ) {
      return E_NOT_OK;
    }
#if 0
    /* !req CANSM555 */
    if( !Network->initialNoComRun ) {
        return E_NOT_OK;
    }
#endif

    if( (CanSM_Internal_HasPendingControllerIndication(NetworkHandle))
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
            || (E_OK != CanSM_Internal_GetTransceiverModeIndicated(NetworkHandle))
#endif
        )
    {
        /* @req CANSM395 */
        /* Pending indication from CanIf */
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_REQUESTCOMMODE, CANSM_E_WAIT_MODE_INDICATION);
        return E_NOT_OK;
    }

    /* @req CANSM402 */
    /* Added to this requirement that state machine cannot be in CANSM_BSM_S_PRECOM
    * if COMM_SILENT_COMMUNICATION is requested this to handle the case when
    * CANSM_BSM_S_FULLCOM exits with T_REPEAT_MAX. */
    if( ((COMM_NO_COMMUNICATION == CanSM_Internal.Networks[NetworkHandle].currentMode) ||
            (CANSM_BSM_S_PRE_NOCOM == CanSM_Internal.Networks[NetworkHandle].BsmState)) &&
            (COMM_SILENT_COMMUNICATION == ComM_Mode)) {
        /* @req CANSM403 */
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_REQUESTCOMMODE, CANSM_E_INVALID_COMM_REQUEST);
        return E_NOT_OK;
    }

    /* Translate below to busoff state */
    /* @req CANSM375 */
    /* @req CANSM376 */
    if( (CANSM_BSM_S_FULLCOM == CanSM_Internal.Networks[NetworkHandle].BsmState) &&
            (CANSM_FULLCOMM_S_BUSOFF_CHECK != CanSM_Internal.Networks[NetworkHandle].FullCommState) &&
            (CANSM_FULLCOMM_S_NO_BUSOFF != CanSM_Internal.Networks[NetworkHandle].FullCommState) ) {
        /* @req CANSM377 */
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_REQUESTCOMMODE, CANSM_E_BUSOFF_RECOVERY_ACTIVE);
        return E_NOT_OK;
    }
    /* Deny request if there is a busoff event. Do not report error here since upper layer cannot
     * know that there is a busoff event present. If busoff event has been handled in main function
     * we will never end up here as if should be handled by check above. */
    if( Network->busoffevent ) {
        return E_NOT_OK;
    }
    /* Request accepted! */
    Network->requestedMode = ComM_Mode;

    return E_OK;
}

/* @req CANSM063 */
Std_ReturnType CanSM_GetCurrentComMode( NetworkHandleType NetworkHandle, ComM_ModeType* ComM_ModePtr ) {

	/* @req CANSM188 */
	if( CANSM_STATUS_UNINIT == CanSM_Internal.InitStatus ) {
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_GETCURRENTCOMMODE, CANSM_E_UNINIT);
		return E_NOT_OK;
	}

	/* @req CANSM371 */
	/* @req CANSM372 */
	if( NetworkHandle > CANSM_NETWORK_COUNT) {
		/* @req CANSM187 */
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_GETCURRENTCOMMODE, CANSM_E_INVALID_NETWORK_HANDLE);
		return E_NOT_OK;
	}

	if( NULL == ComM_ModePtr ) {
		/* @req CANSM360 */
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_GETCURRENTCOMMODE, CANSM_E_PARAM_POINTER);
		return E_NOT_OK;
	}

	/* @req CANSM282 */
	if( FALSE == CanSM_Internal.Networks[NetworkHandle].initialNoComRun ) {
		return E_NOT_OK;
	}

	/* @req CANSM186 */
	*ComM_ModePtr = CanSM_Internal.Networks[NetworkHandle].currentMode;
	return E_OK;
}

static void CanSM_Internal_SetNetworkPduMode(NetworkHandleType NetworkHandle, CanIf_PduSetModeType pduMode)
{
	const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];

	for(uint8 ctrl = 0; ctrl < NetworkCfg->ControllerCount; ctrl++) {
		if( E_OK != CanIf_SetPduMode(NetworkCfg->Controllers[ctrl].CanIfControllerId, pduMode) ) {
			/* This is unexpected.. */
			CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
		}
	}
}

Std_ReturnType CanSM_Internal_SetNetworkControllerMode(NetworkHandleType NetworkHandle, CanIf_ControllerModeType controllerMode)
{
    Std_ReturnType totalStatus = E_OK;
    const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];
    for(uint8 ctrl = 0; ctrl < NetworkCfg->ControllerCount; ctrl++) {
        uint8 index;
        if( E_OK != CanSM_Internal_GetCanSMControllerIndex(NetworkCfg->Controllers[ctrl].CanIfControllerId, &index) ) {
            return E_NOT_OK;
        }
#if !defined(CANSM_REPEAT_ALL_REQUESTS_ON_TIMEOUT)
        /* Only request controller mode if it differs from the last indicated */
        CanSM_Internal.ControllerModeBuf[index].pendingCtrlMode = controllerMode;
        if(CanSM_Internal.ControllerModeBuf[index].indCtrlMode != controllerMode) {
            CanSM_Internal.ControllerModeBuf[index].hasPendingCtrlIndication = TRUE;
            if( E_OK != CanIf_SetControllerMode(NetworkCfg->Controllers[ctrl].CanIfControllerId, controllerMode)) {
                /* Not accepted. No mode indication pending */
                CanSM_Internal.ControllerModeBuf[index].hasPendingCtrlIndication = FALSE;
                totalStatus = E_NOT_OK;
            }
        }
#else
        CanSM_Internal.ControllerModeBuf[index].hasPendingCtrlIndication = TRUE;
        CanSM_Internal.ControllerModeBuf[index].pendingCtrlMode = controllerMode;
        if( E_OK != CanIf_SetControllerMode(NetworkCfg->Controllers[ctrl].CanIfControllerId, controllerMode)) {
            /* Not accepted. No mode indication pending */
            CanSM_Internal.ControllerModeBuf[index].hasPendingCtrlIndication = FALSE;
            totalStatus = E_NOT_OK;
        }
#endif
    }

    return totalStatus;
}

#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
Std_ReturnType CanSM_Internal_SetNetworkTransceiverMode(NetworkHandleType NetworkHandle, CanTrcv_TrcvModeType trcvMode)
{
	Std_ReturnType totalStatus = E_OK;

	CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.hasPendingTrcvIndication = TRUE;
	CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.pendingTrcvMode = trcvMode;

	if( E_OK != CanIf_SetTrcvMode(CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.trcvId, trcvMode)) {
			/* Not accepted. No mode indication pending */
		CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.hasPendingTrcvIndication  = FALSE;
		totalStatus = E_NOT_OK;
	}
	return totalStatus;
}
#endif

static void CanSM_Internal_ResetPendingIndications(NetworkHandleType NetworkHandle)
{
    const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];

    for(uint8 ctrl = 0; ctrl < NetworkCfg->ControllerCount; ctrl++) {
        uint8 ctrlIndex;
        if( E_OK == CanSM_Internal_GetCanSMControllerIndex(NetworkCfg->Controllers[ctrl].CanIfControllerId, &ctrlIndex) ) {
            CanSM_Internal.ControllerModeBuf[ctrlIndex].hasPendingCtrlIndication = FALSE;
        }
    }
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
    CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.hasPendingTrcvIndication = FALSE;
#endif
}

Std_ReturnType CanSM_Internal_GetNetworkControllerModeIndicated(NetworkHandleType NetworkHandle)
{
	Std_ReturnType totalStatus = E_OK;
	const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];

	for(uint8 ctrl = 0; ctrl < NetworkCfg->ControllerCount; ctrl++) {
		uint8 ctrlIndex;
		if( E_OK != CanSM_Internal_GetCanSMControllerIndex(NetworkCfg->Controllers[ctrl].CanIfControllerId, &ctrlIndex) ) {
			return E_NOT_OK;
		}

		if(CanSM_Internal.ControllerModeBuf[ctrlIndex].indCtrlMode != CanSM_Internal.ControllerModeBuf[ctrlIndex].pendingCtrlMode) {
		    /* Has not indicated the last requested mode */
		    totalStatus = E_NOT_OK;
		}
		if( CanSM_Internal.ControllerModeBuf[ctrlIndex].hasPendingCtrlIndication ) {
		    /* No indication received after last request */
			totalStatus = E_NOT_OK;
		}
	}
	return totalStatus;
}

#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
Std_ReturnType CanSM_Internal_GetTransceiverModeIndicated(NetworkHandleType NetworkHandle)
{
	Std_ReturnType totalStatus = E_OK;

	if( CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.hasPendingTrcvIndication) {
	    totalStatus = E_NOT_OK;
	}
	return totalStatus;
}
#endif

static void CanSM_Internal_ComM_BusSM_ModeIndication( NetworkHandleType NetworkHandle, ComM_ModeType ComM_Mode )
{
	/* Update internals */
	CanSM_Internal.Networks[NetworkHandle].currentMode = ComM_Mode;
	/* Indicate to ComM */
	ComM_BusSM_ModeIndication(CanSM_ConfigPtr->Networks[NetworkHandle].ComMNetworkHandle, &ComM_Mode);
}

static CanSM_Internal_PreNoCommExitPointType CanSm_Internal_BSM_S_PRE_NOCOM( NetworkHandleType NetworkHandle )
{
    // Sub state machine CANSM_BSM_S_PRE_NOCOM requirements

    // Guard: CANSM_BSM_G_PN_NOT_SUPPORTED
    /* !req CANSM436 */
    // Guard: CANSM_BSM_G_PN_SUPPORTED
    /* !req CANSM437*/


    /* Comment: Partial networking not supported so the following requirements are not fulfilled

    *
    * Deinit with PN support
    * !req CANSM438
    * !req CANSM439
    * !req CANSM440
    * !req CANSM443
    * !req CANSM441
    * !req CANSM442
    * !req CANSM444
    * !req CANSM445
    * !req CANSM446
    * !req CANSM447
    * !req CANSM448
    * !req CANSM449
    * !req CANSM450
    * !req CANSM451
    * !req CANSM452
    * !req CANSM454
    * !req CANSM453
    * !req CANSM455
    * !req CANSM456
    * !req CANSM457
    * !req CANSM458
    * !req CANSM459
    * !req CANSM460
    * !req CANSM461
    * !req CANSM462
    * !req CANSM463
    *  */

    /* Requirements when partial networking not supported. */

    /* !req CANSM472 */
    /* !req CANSM473 */
    /* !req CANSM474 */
    /* !req CANSM556 */
    /* !req CANSM475 */
    /* !req CANSM476 */
    /* !req CANSM477 */
    /* !req CANSM478 */
    /* !req CANSM557 */
    /* !req CANSM479 */
    /* And CANSM471 */

    CanSM_Internal_PreNoCommExitPointType exitPoint = CANSM_PRENOCOMM_EXITPOINT_NONE;
    CanSM_Internal_TransitionReturnType tRet = T_DONE;
    CanSM_Internal_NetworkType *Network = &CanSM_Internal.Networks[NetworkHandle];
    CanSM_Internal_PreNoCommStateType nextSubState = Network->PreNoCommState;

    switch(Network->PreNoCommState)
    {
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
    case CANSM_PRENOCOMM_S_TRCV_NORMAL:
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_TRCV_NORMAL_TRCV_STANDBY);
        if( T_DONE == tRet ) {
            nextSubState = CANSM_PRENOCOMM_S_TRCV_STANDBY;
        }
        break;

    case CANSM_PRENOCOMM_S_TRCV_STANDBY:
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_TRCV_STANDBY_NONE);
        if( T_DONE == tRet ) {
            exitPoint = CANSM_PRENOCOMM_EXITPOINT_NOCOM;
        }
        break;
#endif
    case CANSM_PRENOCOMM_S_CC_STOPPED:
        /* @req CANSM464 *//* @req CANSM465 *//* @req CANSM466 *//* @req CANSM467 */
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_STOPPED_CC_SLEEP);
        if( T_DONE == tRet ) {
            nextSubState = CANSM_PRENOCOMM_S_CC_SLEEP;
        }
        break;
    case CANSM_PRENOCOMM_S_CC_SLEEP:
        /* @req CANSM468 *//* @req CANSM469 *//* @req CANSM470 *//* @req CANSM471 */
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_SLEEP_TRCV_NORMAL);
#else
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_SLEEP_NONE);
#endif
        if( T_DONE == tRet ) {
            nextSubState = CANSM_PRENOCOMM_S_TRCV_NORMAL;
#if (CANSM_CAN_TRCV_SUPPORT == STD_OFF)
            /* No support for trcv so we are done in this state */
            exitPoint = CANSM_PRENOCOMM_EXITPOINT_NOCOM;
#endif
        }
        break;
    default:
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
        break;
    }

    if( T_WAIT_INDICATION != tRet ) {
        Network->subStateTimer = 0;
        if( T_REPEAT_MAX == tRet ) {
            CanSM_Internal_ResetPendingIndications(NetworkHandle);
            /* @req CANSM480 */
            /* @req CANSM385 */
            CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
            exitPoint = CANSM_PRENOCOMM_EXITPOINT_REPEAT_MAX;
        }
    }
    if( nextSubState != Network->PreNoCommState ) {
        /* Changing state */
        Network->PreNoCommState = nextSubState;
    }
    Network->subStateTimer++;
    return exitPoint;
}

static CanSM_Internal_PreFullCommExitPointType CanSm_Internal_BSM_S_PRE_FULLCOM( NetworkHandleType NetworkHandle )
{
    /* Sub state machine to prepare full communication requirements
     *
     * !req CANSM483
     * !req CANSM484
     * !req CANSM485
     * !req CANSM558
     * !req CANSM486
     *
     * */
    CanSM_Internal_PreFullCommExitPointType exitPoint = CANSM_PREFULLCOMM_EXITPOINT_NONE;
    CanSM_Internal_TransitionReturnType tRet = T_DONE;
    CanSM_Internal_NetworkType *Network = &CanSM_Internal.Networks[NetworkHandle];
    CanSM_Internal_PreFullCommStateType nextSubState = Network->PreFullCommState;

    switch(Network->PreFullCommState)
    {
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
    case CANSM_PREFULLCOMM_S_TRCV_NORMAL:
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_TRCV_NORMAL_CC_STOPPED);
        if( T_DONE == tRet ) {
            nextSubState = CANSM_PREFULLCOMM_S_CC_STOPPED;
        }
        break;
#endif
    case CANSM_PREFULLCOMM_S_CC_STOPPED:
        /* @req CANSM487 *//* @req CANSM488 *//* @req CANSM489 *//* @req CANSM490 */
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_STOPPED_CC_STARTED);
        if( T_DONE == tRet ) {
            nextSubState = CANSM_PREFULLCOMM_S_CC_STARTED;
        }
        break;
    case CANSM_PREFULLCOMM_S_CC_STARTED:
        /* @req CANSM491 *//* @req CANSM492 *//* @req CANSM493 *//* @req CANSM494 */
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_STARTED_NONE);
        if( T_DONE == tRet ) {
            exitPoint = CANSM_PREFULLCOMM_EXITPOINT_FULLCOMM;
        }
        break;
    default:
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
        break;
    }

    if( T_WAIT_INDICATION != tRet ) {
        Network->subStateTimer = 0;
        if( T_REPEAT_MAX == tRet ) {
            CanSM_Internal_ResetPendingIndications(NetworkHandle);
            /* @req CANSM495 */
            /* @req CANSM385 */
            CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
            exitPoint = CANSM_PREFULLCOMM_EXITPOINT_REPEAT_MAX;
        }
    }
    if( nextSubState != Network->PreFullCommState ) {
        /* Changing state */
        Network->PreFullCommState = nextSubState;
    }
    Network->subStateTimer++;
    return exitPoint;
}


static CanSM_Internal_FullCommExitPointType CanSm_Internal_BSM_S_FULLCOM( NetworkHandleType NetworkHandle )
{
    /* Sub state machine for requested full communication requirements
     *

     * !req CANSM507
     * !req CANSM528
     *  */
    CanSM_Internal_FullCommExitPointType exitPoint = CANSM_FULLCOMM_EXITPOINT_NONE;
    CanSM_Internal_NetworkType *Network = &CanSM_Internal.Networks[NetworkHandle];
    CanSM_Internal_TransitionReturnType tRet;
    const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[NetworkHandle];

    switch(Network->FullCommState)
    {
    case CANSM_FULLCOMM_S_BUSOFF_CHECK:
        Network->subStateTimer++;
        if( Network->busoffevent ) {
            /* @req CANSM500 */
            Network->busoffTime = 0;
            Network->busoffevent = FALSE;
            /* Reset indications so that all controllers are restarted. */
            CanSM_Internal_ResetControllerIndications(NetworkHandle);
            if( Network->busoffCounter < NetworkCfg->CanSMBorCounterL1ToL2 ) {
                Network->busoffCounter++;
            }

#if defined(USE_BSWM)
            /* @req CANSM508*/
            BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_BUS_OFF);
#endif
            /* @req CANSM521 */
            CanSM_Internal_ComM_BusSM_ModeIndication(NetworkHandle, COMM_SILENT_COMMUNICATION);

#if defined(USE_DEM)

            /* @req CANSM522 */
            if( DEM_EVENT_ID_NULL != NetworkCfg->CanSMDemEventId ) {
                DEM_REPORT_ERROR_STATUS(NetworkCfg->CanSMDemEventId, DEM_EVENT_STATUS_PREFAILED);
            }
#endif

            /* Restart controller */
            tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_STARTED_NONE);
            if( T_REPEAT_MAX != tRet ) {
                Network->FullCommState = CANSM_FULLCOMM_S_RESTART_CC;
            } else {
                CanSM_Internal_ResetPendingIndications(NetworkHandle);
                /* T_REPEAT_MAX */
                /* @req CANSM523 */
                /* @req CANSM385 */
                CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
                exitPoint = CANSM_FULLCOMM_EXITPOINT_PRENOCOMM;
            }
            Network->subStateTimer = 0;
        } else if( Network->subStateTimer >= NetworkCfg->CanSMBorTimeTxEnsured ) {
            /* @req CANSM496 */
            /* !req CANSM497 */
#if defined(USE_DEM)
            /* @req CANSM498 */
            if( DEM_EVENT_ID_NULL != NetworkCfg->CanSMDemEventId ) {
                DEM_REPORT_ERROR_STATUS(NetworkCfg->CanSMDemEventId, DEM_EVENT_STATUS_PASSED);
            }
#endif
            Network->busoffCounter = 0;
            Network->FullCommState = CANSM_FULLCOMM_S_NO_BUSOFF;
        }

        break;

    case CANSM_FULLCOMM_S_NO_BUSOFF:
        if( Network->busoffevent ) {
            /* @req CANSM500 */
            Network->busoffevent = FALSE;
            Network->busoffTime = 0;
            /* Reset indications so that all controllers are restarted. */
            CanSM_Internal_ResetControllerIndications(NetworkHandle);
            if( Network->busoffCounter < NetworkCfg->CanSMBorCounterL1ToL2 ) {
                Network->busoffCounter++;
            }

#if defined(USE_BSWM)
            /* @req CANSM508 */
            BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_BUS_OFF);
#endif
            /* @req CANSM521 */
            CanSM_Internal_ComM_BusSM_ModeIndication(NetworkHandle, COMM_SILENT_COMMUNICATION);

#if defined(USE_DEM)

            /* @req CANSM522 */
            if( DEM_EVENT_ID_NULL != NetworkCfg->CanSMDemEventId ) {
                Dem_ReportErrorStatus(NetworkCfg->CanSMDemEventId, DEM_EVENT_STATUS_PREFAILED);
            }
#endif

            /* Restart controller */
            tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_STARTED_NONE);
            if( T_REPEAT_MAX != tRet ) {
                Network->busoffevent = FALSE;
                Network->FullCommState = CANSM_FULLCOMM_S_RESTART_CC;
            } else {
                CanSM_Internal_ResetPendingIndications(NetworkHandle);
                /* T_REPEAT_MAX */
                /* @req CANSM523 */
                /* @req CANSM385 */
                CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
                exitPoint = CANSM_FULLCOMM_EXITPOINT_PRENOCOMM;
            }
            Network->subStateTimer = 0;
        } else if( (COMM_SILENT_COMMUNICATION == Network->requestedMode) ||
                ( COMM_NO_COMMUNICATION == Network->requestedMode ) ) {
            /* @req CANSM499 */
            /* @req CANSM554 */
            exitPoint = CANSM_FULLCOMM_EXITPOINT_SILENTCOMM;
        }
        break;
    case CANSM_FULLCOMM_S_RESTART_CC:
        Network->subStateTimer++;
        /* @req CANSM509 *//* @req CANSM510 *//* @req CANSM466 *//* @req CANSM467 */
        tRet = CanSM_Internal_ExecuteTransition(Network, NetworkHandle, T_CC_STARTED_NONE);
        if( T_DONE == tRet ) {
            /* @req CANSM511 */
            /* @req CANSM513 */
            CanSM_Internal_SetNetworkPduMode(NetworkHandle, CANIF_SET_TX_OFFLINE);
            Network->FullCommState = CANSM_FULLCOMM_S_TX_OFF;
            Network->subStateTimer = 0;
        } else if( T_FAIL == tRet || (T_REQ_ACCEPTED == tRet)) {
            /* Timeout or request not accepted */
            Network->subStateTimer = 0;
        } else if( T_REPEAT_MAX == tRet ) {
            CanSM_Internal_ResetPendingIndications(NetworkHandle);
            /* T_REPEAT_MAX */
            /* @req CANSM523 */
            /* @req CANSM385 */
            CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
            exitPoint = CANSM_FULLCOMM_EXITPOINT_PRENOCOMM;
            Network->subStateTimer = 0;
            Network->busoffevent = FALSE;
        } else {
            /* Pending */
        }
        break;

    case CANSM_FULLCOMM_S_TX_OFF:
        Network->subStateTimer++;
        /* @req CANSM514 */
        /* @req CANSM515 */
        if( ((Network->busoffTime >= NetworkCfg->CanSMBorTimeL1) &&
                (Network->busoffCounter < NetworkCfg->CanSMBorCounterL1ToL2)) ||
                ((Network->busoffTime >= NetworkCfg->CanSMBorTimeL2) &&
                (Network->busoffCounter >= NetworkCfg->CanSMBorCounterL1ToL2))) {
#if defined(USE_CANSM_EXTENSION)
            CanSM_Extension_WriteState(NetworkHandle, Network->busoffCounter, Network->subStateTimer, NetworkCfg);
#endif

            /* @req CANSM516 */
            CanSM_Internal_SetNetworkPduMode(NetworkHandle, CANIF_SET_ONLINE);
#if defined(USE_BSWM)
            /* @req CANSM517*/
            BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_FULL_COMMUNICATION);
#endif
            /* @req CANSM518 */
            CanSM_Internal_ComM_BusSM_ModeIndication(NetworkHandle, COMM_FULL_COMMUNICATION);
            Network->FullCommState = CANSM_FULLCOMM_S_BUSOFF_CHECK;
            Network->subStateTimer = 0;
        }
        break;
    default:
        CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
        break;
    }
    Network->busoffTime++;
    return exitPoint;
}

/* @req CANSM065 */
void CanSM_MainFunction() {

    /* @req CANSM167 */
    /* @req CANSM266 */
    /* @req CANSM284 */
    /* @req CANSM428 */
    /* !req CANSM422 */

    /* Main state machine requirements


     * Guard: G_FULL_COM_MODE_REQUESTED
     * !req CANSM427
     * Guard: G_SILENT_COM_MODE_REQUESTED
     * !req CANSM429

     * Effect: E_BR_END_FULL_COM
     * !req CANSM432
     * Effect: E_BR_END_SILENT_COM
     * !req CANSM433
     * */

    /* Sub state machine to operate a requested baud rate change
     *
     * !req CANSM524
     * !req CANSM525
     * !req CANSM526
     * !req CANSM527
     * !req CANSM529
     * !req CANSM531
     * !req CANSM532
     * !req CANSM533
     * !req CANSM534
     * !req CANSM535
     * !req CANSM536
     * !req CANSM542
     * !req CANSM543
     *  */


    if( CANSM_STATUS_UNINIT == CanSM_Internal.InitStatus ) {
        return;
    }
    for (uint8 networkHandle = 0; networkHandle < CANSM_NETWORK_COUNT; networkHandle++) {
        CanSM_Internal_NetworkType *Network = &CanSM_Internal.Networks[networkHandle];
#if defined(USE_BSWM)
        const CanSM_NetworkType *NetworkCfg = &CanSM_ConfigPtr->Networks[networkHandle];
#endif
        switch(Network->BsmState)
        {
        case CANSM_BSM_S_NOT_INITIALIZED:
        case CANSM_BSM_S_PRE_NOCOM:
        case CANSM_BSM_S_SILENTCOM:
            /* @req CANSM423 */
            /* @req CANSM426 */
            if( (CANSM_BSM_S_NOT_INITIALIZED == Network->BsmState) ||
                    ((CANSM_BSM_S_SILENTCOM == Network->BsmState) && (COMM_NO_COMMUNICATION == Network->requestedMode)) ) {
                CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_PRE_NOCOM);
#if defined(USE_BSWM)
                /* @req CANSM431 */
                BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_NO_COMMUNICATION);
#endif
            }
            if( CANSM_BSM_S_PRE_NOCOM == Network->BsmState ) {
                CanSM_Internal_PreNoCommExitPointType preNoComExit = CanSm_Internal_BSM_S_PRE_NOCOM(networkHandle);
                if( CANSM_PRENOCOMM_EXITPOINT_NOCOM == preNoComExit ) {
                    CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_NOCOM);
                    /* @req CANSM430 */
                    CanSM_Internal_ComM_BusSM_ModeIndication(networkHandle, COMM_NO_COMMUNICATION);
                } else if( CANSM_PRENOCOMM_EXITPOINT_REPEAT_MAX == preNoComExit ) {
                    CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_PRE_NOCOM);
                    /* Run a loop in PRE_NOCOM.
                     * NOTE: Not expected to exit that state here. */
                    if( CANSM_PRENOCOMM_EXITPOINT_NOCOM == CanSm_Internal_BSM_S_PRE_NOCOM(networkHandle) ) {
                        CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
                    }
                }
            } else if( (CANSM_BSM_S_SILENTCOM == Network->BsmState) && (COMM_FULL_COMMUNICATION == Network->requestedMode) ) {
                // Effect: E_SILENT_TO_FULL_COM
                /* Should be same as E_FULL_COMM. Spec. refers to CANSM435 but only obeying that
                 * requirement would not be the same as E_FULL_COMM. */
                CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_FULLCOM);
                /* @req CANSM550 */
#if defined(USE_BSWM)
                /* @req CANSM435 */ /* @req CANSM550 */
                BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_FULL_COMMUNICATION);
#endif

                /* @req CANSM435 */
                CanSM_Internal_SetNetworkPduMode(networkHandle, CANIF_SET_ONLINE);
                CanSM_Internal_ComM_BusSM_ModeIndication(networkHandle, COMM_FULL_COMMUNICATION);
            }
            break;
        case CANSM_BSM_S_NOCOM:
        case CANSM_BSM_S_PRE_FULLCOM:
        {
            /* @req CANSM425 */
            if( (CANSM_BSM_S_NOCOM == Network->BsmState) && (COMM_FULL_COMMUNICATION == Network->requestedMode) ) {
                /* Entered CANSM_BSM_S_PRE_NOCOM, reset pre full comm substate */
                CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_PRE_FULLCOM);
            }
            if(CANSM_BSM_S_PRE_FULLCOM == Network->BsmState) {
                CanSM_Internal_PreFullCommExitPointType preFullCommExit = CanSm_Internal_BSM_S_PRE_FULLCOM(networkHandle);
                if( CANSM_PREFULLCOMM_EXITPOINT_FULLCOMM == preFullCommExit ) {
                    CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_FULLCOM);
#if defined(USE_BSWM)
                    /* @req CANSM435 */
                    BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_FULL_COMMUNICATION);
#endif
                    /* @req CANSM539 */
                    CanSM_Internal_SetNetworkPduMode(networkHandle, CANIF_SET_ONLINE);
                    /* @req CANSM540 */
                    CanSM_Internal_ComM_BusSM_ModeIndication(networkHandle, COMM_FULL_COMMUNICATION);
                } else if( CANSM_PREFULLCOMM_EXITPOINT_REPEAT_MAX == preFullCommExit ) {
                    CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_PRE_NOCOM);
                    /* Run a loop in PRE_NOCOM.
                     * NOTE: Not expected to exit that state here. */
                    if( CANSM_PRENOCOMM_EXITPOINT_NOCOM == CanSm_Internal_BSM_S_PRE_NOCOM(networkHandle) ) {
                        CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
                    }
                }
            }
            break;
        }
        case CANSM_BSM_S_FULLCOM:
        {
            boolean enterPreNoCom = FALSE;
            CanSM_Internal_FullCommExitPointType fullCommExit = CanSm_Internal_BSM_S_FULLCOM(networkHandle);
            if(CANSM_FULLCOMM_EXITPOINT_PRENOCOMM == fullCommExit) {
                /* Exit to PreNoCom is only due to T_REPEAT_MAX */
                enterPreNoCom = TRUE;
            } else if( CANSM_FULLCOMM_EXITPOINT_SILENTCOMM == fullCommExit ) {
                /* Exit due to accepted ComMode request */
                CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_SILENTCOM);
#if defined(USE_BSWM)
                /* @req CANSM434 */
                BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_SILENT_COMMUNICATION);
#endif
                /* @req CANSM541 */
                CanSM_Internal_SetNetworkPduMode(networkHandle, CANIF_SET_ONLINE);
                /* @req CANSM537 */
                CanSM_Internal_SetNetworkPduMode(networkHandle, CANIF_SET_TX_OFFLINE);
                /* @req CANSM538 */
                CanSM_Internal_ComM_BusSM_ModeIndication(networkHandle, COMM_SILENT_COMMUNICATION);
            }
            if( enterPreNoCom ) {
                CanSM_Internal_EnterState(networkHandle, CANSM_BSM_S_PRE_NOCOM);
#if defined(USE_BSWM)
                /* @req CANSM431 */
                BswM_CanSM_CurrentState(NetworkCfg->ComMNetworkHandle, CANSM_BSWM_NO_COMMUNICATION);
#endif
                /* Run a loop in PRE_NOCOM.
                 * NOTE: Not expected to exit that state here. */
                if( CANSM_PRENOCOMM_EXITPOINT_NOCOM == CanSm_Internal_BSM_S_PRE_NOCOM(networkHandle) ) {
                    CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_UNEXPECTED_EXECUTION);
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

/* @req CANSM396 */
void CanSM_ControllerModeIndication( uint8 ControllerId, CanIf_ControllerModeType ControllerMode )
{

	if( CANSM_STATUS_UNINIT == CanSM_Internal.InitStatus ) {
		/* @req CANSM398 */
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_CONTROLLERMODEINDICATION, CANSM_E_UNINIT);
		return;
	}

	uint8 controllerIndex;
	if( E_OK == CanSM_Internal_GetCanSMControllerIndex(ControllerId, &controllerIndex) ) {
		/* Controller was valid */
	    /* Ignore indication of other modes than the requested */
		if( CanSM_Internal.ControllerModeBuf[controllerIndex].hasPendingCtrlIndication &&
				(ControllerMode == CanSM_Internal.ControllerModeBuf[controllerIndex].pendingCtrlMode) ) {
			CanSM_Internal.ControllerModeBuf[controllerIndex].hasPendingCtrlIndication = FALSE;
		}

		CanSM_Internal.ControllerModeBuf[controllerIndex].indCtrlMode = ControllerMode;
	} else {
		/* @req CANSM397 */
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_CONTROLLERMODEINDICATION, CANSM_E_PARAM_CONTROLLER);
	}

}

#if 0
Std_ReturnType CanSm_CheckBaudrate(NetworkHandleType network, const uint16 Baudrate) {
	/* !req CANSM564 */
	/* !req CANSM565 */
	/* !req CANSM562 */
	/* !req CANSM571 */
	/* !req CANSM563 */
	/* !req CANSM566 */
	/* !req CANSM567 */
	/* !req CANSM568 */
	/* !req CANSM572 */
	return E_NOT_OK;
}

Std_ReturnType CanSm_ChangeBaudrate(NetworkHandleType network, const uint16 Baudrate) {
	/* !req CANSM569 */
	/* !req CANSM570 */
	/* !req CANSM502 */
	/* !req CANSM504 */
	/* !req CANSM505 */
	/* !req CANSM530 */
	/* !req CANSM506 */
	/* !req CANSM573 */
	/* !req CANSM574 */
	/* !req CANSM503 */

	return E_NOT_OK;
}
void CanSM_TxTimeoutException( NetworkHandleType Channel )
{
	/* !req CANSM411 */
	/* !req CANSM412 */
}

void CanSM_ClearTrcvWufFlagIndication( uint8 Transceiver )
{
	/* !req CANSM414 */
	/* !req CANSM415 */
}

void CanSM_CheckTransceiverWakeFlagIndication( uint8 Transceiver )
{
	/* !req CANSM417 */
	/* !req CANSM418 */
}

void CanSM_ConfirmPnAvailability( uint8 Transceiver )
{
	/* !req CANSM546 */
	/* !req CANSM420 */
	/* !req CANSM421 */
}
#endif

#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
/* !req CANSM399 */
void CanSM_TransceiverModeIndication(uint8 TransceiverId, CanTrcv_TrcvModeType TransceiverMode)
{
	NetworkHandleType NetworkHandle;
	uint8 i = 0;

	if( CANSM_STATUS_UNINIT == CanSM_Internal.InitStatus ) {
			/* !req CANSM401 */
			CANSM_DET_REPORTERROR(CANSM_SERVICEID_TRANSCEIVERMODEINDICATION, CANSM_E_UNINIT);
			return;
	}
	for (; i < CANSM_NETWORK_COUNT; i++ )
	{
		if (CanSM_Internal.Networks[i].TransceiverModeBuf.trcvId == TransceiverId)
		{
			NetworkHandle = i;
			break;
		}
	}
	if (CANSM_NETWORK_COUNT == i)
	{
		/* !req CANSM400 */
		CANSM_DET_REPORTERROR(CANSM_SERVICEID_TRANSCEIVERMODEINDICATION, CANSM_E_PARAM_TRANSCEIVER);
		return;
	}

	if( CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.hasPendingTrcvIndication &&
		(TransceiverMode == CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.pendingTrcvMode))
	{
		CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.hasPendingTrcvIndication = FALSE;
	}

	CanSM_Internal.Networks[NetworkHandle].TransceiverModeBuf.indTrcvMode = TransceiverMode;

}
#endif
