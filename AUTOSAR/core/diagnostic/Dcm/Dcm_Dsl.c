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
 *  General requirements
 */
/** @req DCM030 */


#include <string.h>
#include "Dcm.h"
#include "Dcm_Internal.h"
#include "MemMap.h"
#if defined(USE_COMM)
#include "ComM_Dcm.h"
#endif
#include "PduR_Dcm.h"
#include "PduR_Types.h" // OBD: for preemption cancel PDUR tx
#include "PduR.h"       // OBD: for cancel
#include "ComStack_Types.h"
#include "Cpu.h"
#include "debug.h"
#ifndef DCM_NOT_SERVICE_COMPONENT
#include "Rte_Dcm.h"
#endif

#define DECREMENT(timer) { if (timer > 0){timer--;} }
#define DCM_CONVERT_MS_TO_MAIN_CYCLES(x)  ((x)/DCM_MAIN_FUNCTION_PERIOD_TIME_MS)
#define TO_INTERNAL_PERIODIC_PDU(_x) ((_x) - DCM_FIRST_PERIODIC_TX_PDU)


#if (DCM_PAGEDBUFFER_ENABLED)
#error "DCM_PAGEDBUFFER_ENABLED is set to STD_ON, this is not supported by the code."
#endif

#define DCM_TYPE2_TX_ROUTED_TO_IF
#if DCM_NOF_PERIODIC_TX_PDU > 0
#define DCM_USE_TYPE2_PERIODIC_TRANSMISSION
#endif
/*
 * Type definitions.
 */

typedef struct {
	boolean initRun;
	const Dcm_DslProtocolRowType *activeProtocol; // Points to the currently active protocol.
} DcmDsl_RunTimeDataType;

static DcmDsl_RunTimeDataType DcmDslRunTimeData = {
		.initRun = FALSE,
		.activeProtocol = NULL
};

#ifdef DCM_USE_TYPE2_PERIODIC_TRANSMISSION
typedef struct {
    PduIdType diagReqRxPduId;
    boolean pduInUse;
}PeriodicPduStatusType;
static PeriodicPduStatusType PeriodicPduStatus[DCM_NOF_PERIODIC_TX_PDU];
#endif

#if defined(USE_COMM)
static DcmComModeType NetWorkComMode[DCM_NOF_COMM_CHANNELS];
#endif

// OBD: define the status flag of processing done when preemption happens.
static boolean PreemptionNotProcessingDone = FALSE;

static void DslReleaseType2TxPdu(PduIdType txPduId);
static void DslReleaseAllType2TxPdus(void);
// ################# HELPER FUNCTIONS START #################

//
// This function reset/stars the session (S3) timer. See requirement
// DCM141 when that action should be taken.
//
static inline void startS3SessionTimer(Dcm_DslRunTimeProtocolParametersType *runtime, const Dcm_DslProtocolRowType *protocolRow) {
	const Dcm_DslProtocolTimingRowType *timeParams;
	timeParams = protocolRow->DslProtocolTimeLimit;
	runtime->S3ServerTimeoutCount = DCM_CONVERT_MS_TO_MAIN_CYCLES(timeParams->TimStrS3Server);
	runtime->S3ServerStarted = TRUE;
}

// - - - - - - - - - - -

//
// This function reset/stars the session (S3) timer. See requirement
// DCM141 when that action should be taken.
//
static inline void stopS3SessionTimer(Dcm_DslRunTimeProtocolParametersType *runtime) {
	runtime->S3ServerStarted = FALSE;
}

// - - - - - - - - - - -
// OBD: This function reset/stars the preempt timer.
//
static inline void startPreemptTimer(Dcm_DslRunTimeProtocolParametersType *runtime, const Dcm_DslProtocolRowType *protocolRow) {
	runtime->preemptTimeoutCount= DCM_CONVERT_MS_TO_MAIN_CYCLES(protocolRow->DslProtocolPreemptTimeout);
}

#if defined(USE_COMM)
static boolean findProtocolRx(PduIdType dcmRxPduId, const Dcm_DslProtocolRxType **protocolRx)
{
    boolean ret = FALSE;
    if (dcmRxPduId < DCM_DSL_RX_PDU_ID_LIST_LENGTH) {
        *protocolRx = &Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRxGlobalList[dcmRxPduId];
        ret = TRUE;
    }
    return ret;
}
#endif
// - - - - - - - - - - -
//
//	This function implements the requirement DCM139 when
// 	transition from one session to another.
//
static void changeDiagnosticSession(Dcm_DslRunTimeProtocolParametersType *runtime, Dcm_SesCtrlType newSession) {

    /** @req DCM139 */
#if defined(USE_COMM)
    const Dcm_DslProtocolRxType *protocolRx = NULL;
#endif

    if( DCM_DEFAULT_SESSION == runtime->sessionControl ) {
        /* to set the dsp buffer to default*/
        DspInit(FALSE);
    } else {
        runtime->securityLevel = DCM_SEC_LEV_LOCKED; // "0x00".
    }

    if( DCM_DEFAULT_SESSION == newSession ) {
#if defined(USE_COMM)
        if( (DCM_DEFAULT_SESSION != runtime->sessionControl) && findProtocolRx(runtime->diagReqestRxPduId, &protocolRx) ) {
            /* Changing from non-default session to default session */
            if( runtime->diagnosticActiveComM ) {
                ComM_DCM_InactiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM168 */
                runtime->diagnosticActiveComM = FALSE;
            }
        }
#endif
    } else {
#if defined(USE_COMM)
        if( (newSession != runtime->sessionControl) && findProtocolRx(runtime->diagReqestRxPduId, &protocolRx) ) {
            /* Changing session */
            if( !runtime->diagnosticActiveComM ) {
                ComM_DCM_ActiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM167 */
                runtime->diagnosticActiveComM = TRUE;
            }
        }
#endif
    }
    runtime->sessionControl = newSession;

    /* @req DCM628 */
    DspResetDiagnosticActivityOnSessionChange(newSession);

#ifndef DCM_NOT_SERVICE_COMPONENT
    /* IMPROVEMENT: Should be a call to SchM */
    (void)Rte_Switch_DcmDiagnosticSessionControl_DcmDiagnosticSessionControl(newSession);
#endif


}

// - - - - - - - - - - -

void DslResetSessionTimeoutTimer(void) {
	const Dcm_DslProtocolRowType *activeProtocol;
	Dcm_DslRunTimeProtocolParametersType *runtime;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	if( NULL != activeProtocol ) {
		runtime = activeProtocol->DslRunTimeProtocolParameters;
		startS3SessionTimer(runtime, activeProtocol); /** @req DCM141 */
	}
}

// - - - - - - - - - - -

void DslGetCurrentServiceTable(const Dcm_DsdServiceTableType **currentServiceTable) { /** @req DCM195 */
	const Dcm_DslProtocolRowType *activeProtocol;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	if (activeProtocol != NULL) {
		*currentServiceTable = activeProtocol->DslProtocolSIDTable;
	}
}

// - - - - - - - - - - -

Std_ReturnType DslGetActiveProtocol(Dcm_ProtocolType *protocolId) { /** @req DCM340 */
	Std_ReturnType ret = E_NOT_OK;
	const Dcm_DslProtocolRowType *activeProtocol;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	if (activeProtocol != NULL) {
		*protocolId = activeProtocol->DslProtocolID;
		ret = E_OK;
	}
	return ret;
}

// - - - - - - - - - - -

void DslSetSecurityLevel(Dcm_SecLevelType secLevel) { /** @req DCM020 */
	const Dcm_DslProtocolRowType *activeProtocol;
	Dcm_DslRunTimeProtocolParametersType *runtime;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	runtime = activeProtocol->DslRunTimeProtocolParameters;
	runtime->securityLevel = secLevel;
}

// - - - - - - - - - - -

Std_ReturnType DslGetSecurityLevel(Dcm_SecLevelType *secLevel) {  /** @req DCM020 *//** @req DCM338 */
	Std_ReturnType ret = E_NOT_OK;
	const Dcm_DslProtocolRowType *activeProtocol;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	if (activeProtocol != NULL) {
		runtime = activeProtocol->DslRunTimeProtocolParameters;
		*secLevel = runtime->securityLevel;
		ret = E_OK;
	}
	return ret;
}

// - - - - - - - - - - -

void DslSetSesCtrlType(Dcm_SesCtrlType sesCtrl) {  /** @req DCM022 */
	const Dcm_DslProtocolRowType *activeProtocol;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	if (activeProtocol != NULL) { /* This also checks that the module is initiated */
		runtime = activeProtocol->DslRunTimeProtocolParameters;
        changeDiagnosticSession(runtime, sesCtrl);
        DslResetSessionTimeoutTimer();

	}
}

// - - - - - - - - - - -

Std_ReturnType DslGetSesCtrlType(Dcm_SesCtrlType *sesCtrlType) { /** @req DCM022 *//** @req DCM339 */
	Std_ReturnType ret = E_NOT_OK;
	const Dcm_DslProtocolRowType *activeProtocol;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

	activeProtocol = DcmDslRunTimeData.activeProtocol;
	if (activeProtocol != NULL) {
		runtime = activeProtocol->DslRunTimeProtocolParameters;
		*sesCtrlType = runtime->sessionControl;
		ret = E_OK;
	}
	return ret;
}

const Dcm_DspSessionRowType *getActiveSessionRow(Dcm_DslRunTimeProtocolParametersType *runtime) {
	 const Dcm_DspSessionRowType *sessionRow = Dcm_ConfigPtr->Dsp->DspSession->DspSessionRow;

	while ((sessionRow->DspSessionLevel != runtime->sessionControl) && (!sessionRow->Arc_EOL) ) {
		sessionRow++;
	}
	if( TRUE == sessionRow->Arc_EOL ) {
		/* Since we are in a session with no configuration - take any session configuration and report error */
	    DCM_DET_REPORTERROR(DCM_CHANGE_DIAGNOSTIC_SESSION_ID, DCM_E_CONFIG_INVALID);
		sessionRow = Dcm_ConfigPtr->Dsp->DspSession->DspSessionRow;
	}

	return sessionRow;
}

// - - - - - - - - - - -

static boolean findRxPduIdParentConfigurationLeafs(PduIdType dcmRxPduId,
		const Dcm_DslProtocolRxType **protocolRx,
		const Dcm_DslMainConnectionType **mainConnection,
		const Dcm_DslConnectionType **connection,
		const Dcm_DslProtocolRowType **protocolRow,
		Dcm_DslRunTimeProtocolParametersType **runtime) {

	boolean ret = FALSE;
	if (dcmRxPduId < DCM_DSL_RX_PDU_ID_LIST_LENGTH) {
		*protocolRx = &Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRxGlobalList[dcmRxPduId];
		*mainConnection = (*protocolRx)->DslMainConnectionParent;
		*connection = (*mainConnection)->DslConnectionParent;
		*protocolRow = (*connection)->DslProtocolRow;
		*runtime = (*protocolRow)->DslRunTimeProtocolParameters;
		ret = TRUE;
	}
	return ret;
}

// - - - - - - - - - - -

static boolean findTxPduIdParentConfigurationLeafs(PduIdType dcmTxPduId,
		const Dcm_DslProtocolRowType **protocolRow,
		Dcm_DslRunTimeProtocolParametersType **runtime) {

	boolean ret = FALSE;
	if (dcmTxPduId < DCM_DSL_TX_PDU_ID_LIST_LENGTH) {
	    const Dcm_DslMainConnectionType *mainConnection;
	    mainConnection = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolTxGlobalList[dcmTxPduId].DslMainConnectionParent;
	    *protocolRow = mainConnection->DslConnectionParent->DslProtocolRow;
		*runtime = (*protocolRow)->DslRunTimeProtocolParameters;
		ret = TRUE;
	} else if(IS_PERIODIC_TX_PDU(dcmTxPduId)) {
	    *protocolRow = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolPeriodicTxGlobalList[TO_INTERNAL_PERIODIC_PDU(dcmTxPduId)].DslProtocolRow;
	    *runtime = (*protocolRow)->DslRunTimeProtocolParameters;
	    ret = TRUE;
	}
	return ret;
}

// - - - - - - - - - - -

static inline void releaseExternalRxTxBuffers(const Dcm_DslProtocolRowType *protocolRow,
		Dcm_DslRunTimeProtocolParametersType *runtime) {

	protocolRow->DslProtocolTxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
	if(NULL != protocolRow->DslProtocolRxBufferID) {
	    protocolRow->DslProtocolRxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
	}
	runtime->externalTxBufferStatus = NOT_IN_USE; // We are waiting for DSD to return the buffer. qqq.
	runtime->externalRxBufferStatus = NOT_IN_USE; // We are waiting for DSD to return the buffer. qqq.
	protocolRow->DslProtocolTxBufferID->externalBufferRuntimeData->DcmRxPduId = DCM_INVALID_PDU_ID;
	if(runtime->isType2Tx) {
	    /* Release xx buffer for the periodic protocol */
	    const Dcm_DslProtocolRxType *protocolRx = NULL;
	    const Dcm_DslMainConnectionType *mainConnection = NULL;
	    const Dcm_DslConnectionType *connection = NULL;
	    const Dcm_DslProtocolRowType *rxProtocolRow = NULL;
	    Dcm_DslRunTimeProtocolParametersType *rxRuntime = NULL;
	    if(findRxPduIdParentConfigurationLeafs(runtime->diagReqestRxPduId, &protocolRx, &mainConnection, &connection, &rxProtocolRow, &rxRuntime)) {
	        mainConnection->DslPeriodicTransmissionConRef->DslProtocolRow->DslProtocolTxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
	        mainConnection->DslPeriodicTransmissionConRef->DslProtocolRow->DslRunTimeProtocolParameters->externalTxBufferStatus = NOT_IN_USE;
	    }
	}
	runtime->isType2Tx = FALSE;
}

// - - - - - - - - - - -


static inline void releaseExternalRxTxBuffersHelper(PduIdType rxPduIdRef) {
	const Dcm_DslProtocolRxType *protocolRx = NULL;
	const Dcm_DslMainConnectionType *mainConnection = NULL;
	const Dcm_DslConnectionType *connection = NULL;
	const Dcm_DslProtocolRowType *protocolRow = NULL;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

	if (findRxPduIdParentConfigurationLeafs(rxPduIdRef, &protocolRx, &mainConnection, &connection, &protocolRow, &runtime)) {
		releaseExternalRxTxBuffers(protocolRow, runtime);
	}
}

// - - - - - - - - - - -

/*
 *  This function is called from the DSD module to the DSL when
 *  a response to a diagnostic request has been copied into the
 *  given TX-buffer and is ready for transmission.
 */
void DslDsdProcessingDone(PduIdType rxPduIdRef, DsdProcessingDoneResultType responseResult) {
    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *mainConnection = NULL;
    const Dcm_DslConnectionType *connection = NULL;
    const Dcm_DslProtocolRowType *protocolRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

    DEBUG( DEBUG_MEDIUM, "DslDsdProcessingDone rxPduIdRef=%d\n", rxPduIdRef);
    Dcm_DslBufferUserType *bufferStatus = NULL;
    if (findRxPduIdParentConfigurationLeafs(rxPduIdRef, &protocolRx, &mainConnection, &connection, &protocolRow, &runtime)) {
        imask_t state;
        Irq_Save(state);
        switch (responseResult) {
        case DSD_TX_RESPONSE_READY:
            if(runtime->isType2Tx) {
                bufferStatus = &mainConnection->DslPeriodicTransmissionConRef->DslProtocolRow->DslRunTimeProtocolParameters->externalTxBufferStatus;
            } else {
                bufferStatus = &runtime->externalTxBufferStatus;
            }
            *bufferStatus = DSD_PENDING_RESPONSE_SIGNALED; /** @req DCM114 */
            break;
        case DSD_TX_RESPONSE_SUPPRESSED: /** @req DCM238 */
            DEBUG( DEBUG_MEDIUM, "DslDsdProcessingDone called with DSD_TX_RESPONSE_SUPPRESSED.\n");
            if(runtime->isType2Tx) {
                /* Release the tx pdu */
                PduIdType dcmTxPduId = 0xffff;
                if(DslPduRPduUsedForType2Tx(runtime->diagResponseTxPduId, &dcmTxPduId)) {
                    /* Release the tx pdu */
                    DslReleaseType2TxPdu(dcmTxPduId);
                }
            }
            releaseExternalRxTxBuffersHelper(rxPduIdRef);
#if defined(USE_COMM)
            if( DCM_DEFAULT_SESSION == runtime->sessionControl ) {
                if( runtime->diagnosticActiveComM ) {
                    ComM_DCM_InactiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM166 *//** @req DCM697 */
                    runtime->diagnosticActiveComM = FALSE;
                }
            }
#endif
            break;
        default:
            DEBUG( DEBUG_MEDIUM, "Unknown response result from DslDsdProcessingDone!\n");
            break;
        }
        Irq_Restore(state);
    }
}

// - - - - - - - - - - -

/*
 *	This function preparing transmission of response
 *	pending message to tester.
 */
static void sendResponse(const Dcm_DslProtocolRowType *protocol,
		Dcm_NegativeResponseCodeType responseCode) {
	const Dcm_DslProtocolRxType *protocolRx = NULL;
	const Dcm_DslMainConnectionType *mainConnection = NULL;
	const Dcm_DslConnectionType *connection = NULL;
	const Dcm_DslProtocolRowType *protocolRow = NULL;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;
	Std_ReturnType transmitResult;
    imask_t state;

    Irq_Save(state);
	/** @req DCM119 */
	if (findRxPduIdParentConfigurationLeafs(protocol->DslRunTimeProtocolParameters->diagReqestRxPduId, &protocolRx,	&mainConnection, &connection, &protocolRow, &runtime)) {
		if (runtime->localTxBuffer.status == NOT_IN_USE) {
		    runtime->localTxBuffer.responseCode = responseCode;
			runtime->localTxBuffer.status = PROVIDED_TO_DSD;
			runtime->localTxBuffer.buffer[0] = SID_NEGATIVE_RESPONSE;
			runtime->localTxBuffer.buffer[1] = protocol->DslProtocolRxBufferID->pduInfo.SduDataPtr[0];
			runtime->localTxBuffer.buffer[2] = responseCode;
			runtime->localTxBuffer.PduInfo.SduDataPtr = runtime->localTxBuffer.buffer;
			runtime->localTxBuffer.PduInfo.SduLength = 3;
			runtime->localTxBuffer.status = DCM_TRANSMIT_SIGNALED; // In the DslProvideTxBuffer 'callback' this state signals it is the local buffer we are interested in sending.
			transmitResult = PduR_DcmTransmit(mainConnection->DslProtocolTx->DcmDslProtocolTxPduId, &(runtime->localTxBuffer.PduInfo));/** @req DCM115.Partially */ /* The P2ServerMin has not been implemented. */
			if (transmitResult != E_OK) {
				// IMPROVEMENT: What to do here?
			}
		}
	}
    Irq_Restore(state);
}

void DslDsdSendResponsePending(PduIdType rxPduId)
{
    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *mainConnection = NULL;
    const Dcm_DslConnectionType *connection = NULL;
    const Dcm_DslProtocolRowType *protocolRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *rxRuntime = NULL;
    const Dcm_DspSessionRowType *sessionRow;
    if (findRxPduIdParentConfigurationLeafs(rxPduId, &protocolRx, &mainConnection, &connection, &protocolRow, &rxRuntime)) {
        sendResponse(protocolRow, DCM_E_RESPONSEPENDING);
        sessionRow = getActiveSessionRow(rxRuntime);
        rxRuntime->stateTimeoutCount = DCM_CONVERT_MS_TO_MAIN_CYCLES(sessionRow->DspSessionP2StarServerMax); /* Reinitiate timer, see 9.2.2. */
        DECREMENT( rxRuntime->responsePendingCount );
    }
}
// - - - - - - - - - - -

static Std_ReturnType StartProtocolHelper(Dcm_ProtocolType protocolId) {
	Std_ReturnType res = E_OK;
	Std_ReturnType ret = E_OK;
	uint16 i;

	for (i = 0; !Dcm_ConfigPtr->Dsl->DslCallbackDCMRequestService[i].Arc_EOL; i++) {
		if (Dcm_ConfigPtr->Dsl->DslCallbackDCMRequestService[i].StartProtocol != NULL) {
			res = Dcm_ConfigPtr->Dsl->DslCallbackDCMRequestService[i]. StartProtocol(protocolId);
			if (res == E_NOT_OK || res == E_PROTOCOL_NOT_ALLOWED) {
			    ret = E_NOT_OK;
				break;
			}
		}
	}
	return ret;
}

// OBD: add stop protocol for stack preempting
static Std_ReturnType StopProtocolHelper(Dcm_ProtocolType protocolId) {
    Std_ReturnType res = E_OK;
    Std_ReturnType ret = E_OK;
    uint16 i;

    for (i = 0; !Dcm_ConfigPtr->Dsl->DslCallbackDCMRequestService[i].Arc_EOL; i++) {
        if (Dcm_ConfigPtr->Dsl->DslCallbackDCMRequestService[i].StopProtocol != NULL) {
            res = Dcm_ConfigPtr->Dsl->DslCallbackDCMRequestService[i].StopProtocol(protocolId);
            if (res == E_NOT_OK || res == E_PROTOCOL_NOT_ALLOWED) {
                ret = E_NOT_OK;
                break;
            }
        }
    }
    return ret;
}
// - - - - - - - - - - -

static boolean isTesterPresentCommand(const PduInfoType *rxPdu) {
	boolean ret = FALSE;
	if ((rxPdu->SduDataPtr[0] == SID_TESTER_PRESENT) && (rxPdu->SduDataPtr[1] & SUPPRESS_POS_RESP_BIT)) {
		ret = TRUE;
	}
	return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'void Dcm_Init(void)' for DSL.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DslInit(void) {
	const Dcm_DslProtocolRowType *listEntry;
#if defined(USE_COMM)
	const Dcm_DslProtocolRxType *protocolRx;
#endif
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

	listEntry = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRowList;

	while (listEntry->Arc_EOL == FALSE) {
		runtime = listEntry->DslRunTimeProtocolParameters;
		runtime->externalRxBufferStatus = NOT_IN_USE;
		runtime->externalTxBufferStatus = NOT_IN_USE;
		runtime->localRxBuffer.status = NOT_IN_USE;
		runtime->localTxBuffer.status = NOT_IN_USE;
		runtime->securityLevel = DCM_SEC_LEV_LOCKED; /** @req DCM033 */
		runtime->sessionControl = DCM_DEFAULT_SESSION; /** @req DCM034 */
		runtime->diagnosticActiveComM = FALSE;
		runtime->protocolStarted = FALSE;// OBD: close the protocol
		/* Check that there is an rx buffer.
		 * Will not be in case of periodic transmission protocol */
		if(NULL != listEntry->DslProtocolRxBufferID) {
		    listEntry->DslProtocolRxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
		}
		listEntry->DslProtocolTxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
		listEntry->DslProtocolTxBufferID->externalBufferRuntimeData->DcmRxPduId = DCM_INVALID_PDU_ID;
		runtime->localRxBuffer.DcmRxPduId = DCM_INVALID_PDU_ID;
		runtime->isType2Tx = FALSE;
		listEntry++;
	};
#if defined(USE_COMM)
	protocolRx = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRxGlobalList;
	while(!protocolRx->Arc_EOL) {
	    NetWorkComMode[protocolRx->ComMChannelInternalIndex] = DCM_NO_COM;
	    protocolRx++;
	}
#endif
	DcmDslRunTimeData.initRun = TRUE;
	// NOTE: Why?
	DcmDslRunTimeData.activeProtocol = NULL;// OBD: close the current active protocol

	DslReleaseAllType2TxPdus();

}

boolean DslCheckType2TxPduAvailable(const Dcm_DslMainConnectionType *mainConnection)
{
#ifdef DCM_USE_TYPE2_PERIODIC_TRANSMISSION
    boolean ret = false;
    const Dcm_DslPeriodicTransmissionType *periodicTrans = mainConnection->DslPeriodicTransmissionConRef;
    /* Find a pdu to use */
    uint8 i = 0;
    while(!periodicTrans->TxPduList[i].Arc_EOL && !ret) {
        if(!PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(periodicTrans->TxPduList[i].DcmTxPduId)].pduInUse) {
            /* Pdu available */
            ret = TRUE;
        }
        i++;
    }
    return ret;
#else
    (void)mainConnection;
    return FALSE;
#endif
}

static void DslReleaseAllType2TxPdus(void) {
#ifdef DCM_USE_TYPE2_PERIODIC_TRANSMISSION
    for(uint16 i = 0; i < DCM_NOF_PERIODIC_TX_PDU; i++) {
        PeriodicPduStatus[i].diagReqRxPduId = 0xFFFF;
        PeriodicPduStatus[i].pduInUse = FALSE;
    }
#endif
}
static void DslReleaseType2TxPdu(PduIdType txPduId) {
#ifdef DCM_USE_TYPE2_PERIODIC_TRANSMISSION
    if(IS_PERIODIC_TX_PDU(txPduId)) {
        PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(txPduId)].pduInUse = FALSE;
        PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(txPduId)].diagReqRxPduId = 0xffff;
    }
#else
    (void)txPduId;
#endif
}

#if defined(DCM_USE_TYPE2_PERIODIC_TRANSMISSION) && defined(DCM_TYPE2_TX_ROUTED_TO_IF)
static Std_ReturnType DslGetType2TxUserRxPdu(PduIdType type2TxPduId, PduIdType *rxPduId)
{
    Std_ReturnType ret = E_NOT_OK;
    if(IS_PERIODIC_TX_PDU(type2TxPduId) && PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(type2TxPduId)].pduInUse) {
        *rxPduId = PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(type2TxPduId)].diagReqRxPduId;
        ret = E_OK;
    }
    return ret;
}
#endif
static Std_ReturnType DslLockType2TxPdu(const Dcm_DslMainConnectionType *mainConnection, PduIdType requestRxPDuId, PduIdType *txPduId) {
#if defined(DCM_USE_TYPE2_PERIODIC_TRANSMISSION)
    Std_ReturnType ret = E_NOT_OK;
    const Dcm_DslPeriodicTransmissionType *periodicTrans = mainConnection->DslPeriodicTransmissionConRef;
    /* Find a pdu to use */
    uint8 i = 0;
    while(!periodicTrans->TxPduList[i].Arc_EOL && (E_OK != ret)) {
        if(!PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(periodicTrans->TxPduList[i].DcmTxPduId)].pduInUse) {
            /* Pdu available */
            PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(periodicTrans->TxPduList[i].DcmTxPduId)].pduInUse = TRUE;
            PeriodicPduStatus[TO_INTERNAL_PERIODIC_PDU(periodicTrans->TxPduList[i].DcmTxPduId)].diagReqRxPduId = requestRxPDuId;
            *txPduId = periodicTrans->TxPduList[i].PduRTxPduId;
            ret = E_OK;
        }
        i++;
    }
    return ret;
#else
    (void)mainConnection;
    (void)requestRxPDuId;
    (void)txPduId;
    return E_NOT_OK;
#endif
}

boolean DslPduRPduUsedForType2Tx(PduIdType pdurTxPduId, PduIdType *dcmTxPduId)
{
#if defined(DCM_USE_TYPE2_PERIODIC_TRANSMISSION)
    boolean isUsed = FALSE;
    const Dcm_DslPeriodicTxType *periodicTx = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolPeriodicTxGlobalList;

    while(!periodicTx->Arc_EOL && !isUsed) {
        isUsed = (pdurTxPduId == periodicTx->PduRTxPduId);
        *dcmTxPduId = periodicTx->DcmTxPduId;
        periodicTx++;
    }
    return isUsed;
#else
    (void)pdurTxPduId;
	(void)dcmTxPduId;
    return FALSE;
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'void DslInternal_ResponseOnOneDataByPeriodicId(uint8 PericodID)' for simulator a periodic did data.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Std_ReturnType DslInternal_ResponseOnOneDataByPeriodicId(uint8 PericodID, PduIdType rxPduId)
{
	Std_ReturnType ret = E_NOT_OK;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;


	if( NULL != DcmDslRunTimeData.activeProtocol ) {
		runtime =  DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters;
		if(runtime != NULL)	// find the runtime
		{
			PduLengthType rxBufferSize;
			if( BUFREQ_OK == DslStartOfReception(rxPduId, 3, &rxBufferSize, TRUE)){
				PduInfoType  periodData;
				uint8 data[3];
				periodData.SduDataPtr = data;
				periodData.SduDataPtr[0] = SID_READ_DATA_BY_PERIODIC_IDENTIFIER;
				periodData.SduDataPtr[1] = DCM_PERIODICTRANSMIT_DEFAULT_MODE;
				periodData.SduDataPtr[2] = PericodID;
				periodData.SduLength = 3;
				if( BUFREQ_OK == DslCopyDataToRxBuffer(rxPduId, &periodData, &rxBufferSize) ) {
					DslTpRxIndicationFromPduR(rxPduId, NTFRSLT_OK, TRUE );
				} else {
					/* Something went wrong. Indicate that the reception was
					 * not ok. */
					DslTpRxIndicationFromPduR(rxPduId, NTFRSLT_E_NOT_OK, TRUE);
				}
				ret = E_OK;
			}
		}
	}

	return ret;
}


Std_ReturnType DslInternal_ResponseOnOneEvent(PduIdType rxPduId, uint8* request, uint8 requestLength)
{
    Std_ReturnType ret = E_NOT_OK;
    Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

    if( NULL != DcmDslRunTimeData.activeProtocol ) {
        runtime =  DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters;
        if(runtime != NULL) // find the runtime
        {
            PduLengthType rxBufferSize;
            if( BUFREQ_OK == DslStartOfReception(rxPduId, requestLength, &rxBufferSize, TRUE)){
                PduInfoType  requestPdu;
                requestPdu.SduDataPtr = request;
                requestPdu.SduLength = requestLength;
                if( BUFREQ_OK == DslCopyDataToRxBuffer(rxPduId, &requestPdu, &rxBufferSize) ) {
                    DslTpRxIndicationFromPduR(rxPduId, NTFRSLT_OK, TRUE );
                } else {
                    /* Something went wrong. Indicate that the reception was
                     * not ok. */
                    DslTpRxIndicationFromPduR(rxPduId, NTFRSLT_E_NOT_OK, TRUE);
                }
                ret = E_OK;
            }
        }
    }

    return ret;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'void Dcm_MainFunction(void)' for DSL.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DslMain(void) {
    const Dcm_DslProtocolRowType *protocolRowEntry;
    const Dcm_DspSessionRowType *sessionRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

    protocolRowEntry = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRowList;
    while (protocolRowEntry->Arc_EOL == FALSE) {
        runtime = protocolRowEntry->DslRunTimeProtocolParameters;
        if (runtime != NULL) {
            // #### HANDLE THE TESTER PRESENT PRESENCE ####
            if (runtime->sessionControl != DCM_DEFAULT_SESSION) { // Timeout if tester present is lost.
                if( TRUE == runtime->S3ServerStarted ) {
                    DECREMENT(runtime->S3ServerTimeoutCount);
                }
                if (runtime->S3ServerTimeoutCount == 0) {
                    DslReleaseAllType2TxPdus();
                    /* @req DCM626 *//* Changing session to default will stop control */
                    changeDiagnosticSession(runtime, DCM_DEFAULT_SESSION); /** @req DCM140 */
                    if(DCM_OBD_ON_CAN == protocolRowEntry->DslProtocolID){
                        runtime->protocolStarted = FALSE;
                        if(DCM_OBD_ON_CAN == DcmDslRunTimeData.activeProtocol->DslProtocolID){
                            DcmDslRunTimeData.activeProtocol = NULL;
                        }
                    }
                }
            }
            switch (runtime->externalTxBufferStatus) { // #### TX buffer state. ####
            case NOT_IN_USE:
                DEBUG( DEBUG_MEDIUM, "state NOT_IN_USE!\n");
                break;
            case PROVIDED_TO_DSD: {
                DECREMENT(runtime->stateTimeoutCount);
                if (runtime->stateTimeoutCount == 0) {
                    sessionRow = getActiveSessionRow(runtime);
                    runtime->stateTimeoutCount = DCM_CONVERT_MS_TO_MAIN_CYCLES(sessionRow->DspSessionP2StarServerMax); /* Reinitiate timer, see 9.2.2. */
                    if (Dcm_ConfigPtr->Dsl->DslDiagResp != NULL) {
                        if (Dcm_ConfigPtr->Dsl->DslDiagResp->DslDiagRespForceRespPendEn == TRUE) {
                            if (runtime->responsePendingCount != 0) {
                                /** !req DCM203 suppressPosRspBit should be cleared */
                                sendResponse(protocolRowEntry, DCM_E_RESPONSEPENDING);  /** @req DCM024 */
                                DECREMENT( runtime->responsePendingCount );
                            } else {
                                DspCancelPendingRequests();
                                sendResponse(protocolRowEntry, DCM_E_GENERALREJECT); /** @req DCM120 */
                                releaseExternalRxTxBuffers(protocolRowEntry, runtime);
                            }
                        } else {
                            DEBUG( DEBUG_MEDIUM, "Not configured to send response pending, now sending general reject!\n");
                            DspCancelPendingRequests();
                            sendResponse(protocolRowEntry, DCM_E_GENERALREJECT);
                            releaseExternalRxTxBuffers(protocolRowEntry, runtime);
                        }
                    }
                }
                break;
            }
            case DSD_PENDING_RESPONSE_SIGNALED:
                // The DSD has signaled to DSL that the diagnostic response is available in the Tx buffer.
                // Make sure that response pending or general reject have not been issued,
                // if so we can not transmit to PduR because we would not know from where
                // the Tx confirmation resides later.
                DEBUG( DEBUG_MEDIUM, "state DSD_PENDING_RESPONSE_SIGNALED!\n");
                if (runtime->localTxBuffer.status == NOT_IN_USE) { // Make sure that no TxConfirm could be sent by the local buffer and mixed up with this transmission.
                    const Dcm_DslProtocolRxType *protocolRx = NULL;
                    const Dcm_DslMainConnectionType *mainConnection = NULL;
                    const Dcm_DslConnectionType *connection = NULL;
                    const Dcm_DslProtocolRowType *protocolRow = NULL;
                    Dcm_DslRunTimeProtocolParametersType *rxRuntime = NULL;
                    Std_ReturnType transmitResult;
                    if (findRxPduIdParentConfigurationLeafs(runtime->diagReqestRxPduId, &protocolRx, &mainConnection, &connection, &protocolRow, &rxRuntime)) {
                        Dcm_DslRunTimeProtocolParametersType *txRuntime;
                        if(rxRuntime->isType2Tx) {
                            txRuntime = mainConnection->DslPeriodicTransmissionConRef->DslProtocolRow->DslRunTimeProtocolParameters;
                        } else {
                            txRuntime = rxRuntime;
                        }
                        const PduIdType txPduId = txRuntime->diagResponseTxPduId;
                        DEBUG( DEBUG_MEDIUM, "runtime->externalTxBufferStatus enter state DCM_TRANSMIT_SIGNALED.\n" );
                        txRuntime->externalTxBufferStatus = DCM_TRANSMIT_SIGNALED;
                        transmitResult = PduR_DcmTransmit(txPduId, &txRuntime->diagnosticResponseFromDsd); /** @req DCM237 *//* Will trigger PduR (CanTP) to call DslProvideTxBuffer(). */
                        PduIdType dcmType2TxPduId = 0xffff;
                        if( E_OK != transmitResult ) {
                            /* Transmit failed.
                             * If it is a type2 transmit, release the pdu. */
                            if(DslPduRPduUsedForType2Tx(txPduId, &dcmType2TxPduId)) {
                                DslReleaseType2TxPdu(dcmType2TxPduId);
                            } else {
                                /* Not a periodic pdu. Start s3 timer since there will be
                                 * no tx confirmation of this transmit request */
                                startS3SessionTimer(rxRuntime, protocolRow);
                            }
                            /* Release buffers */
                            releaseExternalRxTxBuffers(protocolRow, rxRuntime);
                        }
#if defined(DCM_TYPE2_TX_ROUTED_TO_IF)
                        /* Assuming that the transmit was routed directly to lower If.
                         * This means that data is copied directly on transmit request
                         * and we can drop the buffers. */
                        else if( DslPduRPduUsedForType2Tx(txPduId, &dcmType2TxPduId) ) {
                            /* It is a type2 transmit. Release buffers. */
                            releaseExternalRxTxBuffers(protocolRow, rxRuntime);
                        }
#endif
                    } else {
                        DEBUG( DEBUG_MEDIUM, "***** WARNING, THIS IS UNEXPECTED !!! ********.\n" );
                        const PduIdType txPduId = protocolRowEntry->DslConnections->DslMainConnection->DslProtocolTx->DcmDslProtocolTxPduId;
                        DEBUG( DEBUG_MEDIUM, "runtime->externalTxBufferStatus enter state DSD_PENDING_RESPONSE_SIGNALED.\n", txPduId);
                        runtime->externalTxBufferStatus = DCM_TRANSMIT_SIGNALED;
                        DEBUG( DEBUG_MEDIUM, "Calling PduR_DcmTransmit with txPduId = %d from DslMain\n", txPduId);
                        transmitResult = PduR_DcmTransmit(txPduId, &runtime->diagnosticResponseFromDsd); /** @req DCM237 *//* Will trigger PduR (CanTP) to call DslProvideTxBuffer(). */
                        if (transmitResult != E_OK) {
                            /* Transmit request failed, release the buffers */
                            releaseExternalRxTxBuffers(protocolRow, runtime);
                            PduIdType dcmTxPduId = 0xffff;
                            if(DslPduRPduUsedForType2Tx(txPduId, &dcmTxPduId)) {
                                DslReleaseType2TxPdu(dcmTxPduId);
                            }
                            /* Start s3 timer since there will be no tx confirmation of this transmit request */
                            startS3SessionTimer(rxRuntime, protocolRow);
                        }
                    }
                }
                break;
            case DCM_TRANSMIT_SIGNALED:
                DEBUG( DEBUG_MEDIUM, "state DSD_PENDING_RESPONSE_SIGNALED!\n");
                break;
            case PROVIDED_TO_PDUR: // The valid data is being transmitted by TP-layer.
                DEBUG( DEBUG_MEDIUM, "state DSD_PENDING_RESPONSE_SIGNALED!\n");
                break;
                //IMPROVEMENT: Fix OBD stuff
            case PREEMPT_TRANSMIT_NRC: /* preemption has happened,send NRC 0x21 to OBD tester */
                if (TRUE == PreemptionNotProcessingDone){
                    /*sent NRC 0x21 till timeout or processing done*/
                    sendResponse(protocolRowEntry, DCM_E_BUSYREPEATREQUEST);

                    /*decrease preempt timeout count*/
                    DECREMENT(runtime->preemptTimeoutCount);
                    /*if processing done is finished,clear the flag*/
                    if (DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters->externalTxBufferStatus == NOT_IN_USE){
                        /*if processing done is finished,clear the flag*/
                        PreemptionNotProcessingDone = FALSE;
                        /*close the preempted protocol*/
                        DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters->protocolStarted = FALSE;
                        /*remove the active protocol and waiting for second OBD request*/
                        DcmDslRunTimeData.activeProtocol = NULL;
                        /*release current protocol buffer*/
                        releaseExternalRxTxBuffers(protocolRowEntry, runtime);
                        /* @req DCM627 */
                        DcmDspResetDiagnosticActivity();
                    } else if(runtime->preemptTimeoutCount == 0){
                        /*if preempt timeout,clear the flag*/
                        PreemptionNotProcessingDone = FALSE;
                        /*close the preempted protocol*/
                        DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters->protocolStarted = FALSE;
                        /*release the extrnal Rx and Tx buffters of the preempted protocol*/
                        releaseExternalRxTxBuffers(DcmDslRunTimeData.activeProtocol, DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters);
                        /*remove the active protocol and waiting for second OBD request*/
                        DcmDslRunTimeData.activeProtocol = NULL;
                        /*release the extrnal Rx and Tx buffters of the preempting protocol*/
                        releaseExternalRxTxBuffers(protocolRowEntry, runtime);
                        /* @req DCM627 */
                        DcmDspResetDiagnosticActivity();
                        /*initialize DSP*/
                        /* NOTE: Is this correct? */
                        DspInit(FALSE);
                    } else {
                    }
                }
                break;
            default:
                break;
            }
        }
        protocolRowEntry++;
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'BufReq_ReturnType Dcm_StartOfReception(PduIdType dcmRxPduId,
//	PduLengthType tpSduLength, PduLengthType *rxBufferSizePtr)'.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  This function is called called by the PduR typically when CanTp has
//  received a FF or a single frame and needs to obtain a buffer from the
//  receiver so that received data can be forwarded.
BufReq_ReturnType DslStartOfReception(PduIdType dcmRxPduId, PduLengthType tpSduLength, PduLengthType *rxBufferSizePtr, boolean internalRequest)
{
    BufReq_ReturnType ret = BUFREQ_NOT_OK;
    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *mainConnection = NULL;
    const Dcm_DslConnectionType *connection = NULL;
    const Dcm_DslProtocolRowType *protocolRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *runtime = NULL;
    boolean intType2Request = FALSE;
    imask_t state;
    DEBUG( DEBUG_MEDIUM, "DslProvideRxBufferToPdur(dcmRxPduId=%d) called!\n", dcmRxPduId);
    Irq_Save(state);
    if (findRxPduIdParentConfigurationLeafs(dcmRxPduId, &protocolRx, &mainConnection, &connection, &protocolRow, &runtime)) {
        const Dcm_DslBufferType *externalRxBuffer = protocolRow->DslProtocolRxBufferID;
        intType2Request = internalRequest && (DCM_PROTOCOL_TRANS_TYPE_2 == protocolRow->DslProtocolTransType);
        if( !(internalRequest && (DCM_PROTOCOL_TRANS_TYPE_2 == protocolRow->DslProtocolTransType)) || DslCheckType2TxPduAvailable(mainConnection) ) {
            if (externalRxBuffer->pduInfo.SduLength >= tpSduLength) { /** @req DCM443 */
                if ((runtime->externalRxBufferStatus == NOT_IN_USE) && (externalRxBuffer->externalBufferRuntimeData->status == BUFFER_AVAILABLE)) {
                    DEBUG( DEBUG_MEDIUM, "External buffer available!\n");
                    // ### EXTERNAL BUFFER IS AVAILABLE; GRAB IT AND REMEBER THAT WE OWN IT! ###
                    externalRxBuffer->externalBufferRuntimeData->status = BUFFER_BUSY;
                    runtime->diagnosticRequestFromTester.SduDataPtr = externalRxBuffer->pduInfo.SduDataPtr;
                    runtime->diagnosticRequestFromTester.SduLength = tpSduLength;
                    externalRxBuffer->externalBufferRuntimeData->nofBytesHandled = 0;
                    runtime->externalRxBufferStatus = PROVIDED_TO_PDUR; /** @req DCM342 */
                    //If external buffer is provided memorize DcmPduId
                    externalRxBuffer->externalBufferRuntimeData->DcmRxPduId = dcmRxPduId;
                    *rxBufferSizePtr = externalRxBuffer->pduInfo.SduLength;
                    ret = BUFREQ_OK;
                } else {
                    if ((NOT_IN_USE == runtime->localRxBuffer.status) && (!internalRequest)) {
                        /* ### EXTERNAL BUFFER IS IN USE BY THE DSD OR PDUR, TRY TO USE LOCAL BUFFER ONLY FOR FUNCTIONAL REQUEST ( ONLY FOR TESTER PRESENT)! ###
                         * Note: There is no way to predict that request is Tester present because pduInfoPtr->SduDataPtr not available.
                         * This is handled in DslTpRxIndicationFromPduR().
                         * But if it is an internal request we now that it is not Tester present.
                         */
                        if (tpSduLength <= DCM_DSL_LOCAL_BUFFER_LENGTH) {
                            runtime->localRxBuffer.status = PROVIDED_TO_PDUR;
                            runtime->localRxBuffer.PduInfo.SduDataPtr = runtime->localRxBuffer.buffer;
                            runtime->localRxBuffer.PduInfo.SduLength = tpSduLength;
                            runtime->localRxBuffer.nofBytesHandled = 0;
                            //If local buffer is provided memorize DcmPduId
                            runtime->localRxBuffer.DcmRxPduId = dcmRxPduId;
                            *rxBufferSizePtr = DCM_DSL_LOCAL_BUFFER_LENGTH;
                            ret = BUFREQ_OK;
                        } else if (externalRxBuffer->externalBufferRuntimeData->DcmRxPduId == dcmRxPduId) {
                            /* This case avoids receiving a new FF is received while MF transmission is ongoing as a response to previous SF request.
                             * Half Duplex case - runtime->externalRxBufferStatus == PROVIDED_TO_DSD
                             */
                            ret = BUFREQ_E_NOT_OK;
                        } else {
                            /* Half duplex - This case avoids a condition where External buffer is locked by a
                             * Functional request and we receive a FF as a physical request */
                            ret = BUFREQ_E_NOT_OK; /** This is a multi-frame reception and discard request. */
                        }
                    }  else {
                        /* Internal request or there is no buffer available, wait until it is free. */
                        ret = BUFREQ_BUSY;
                    }
                }
            } else {
                ret = BUFREQ_OVFL; /** @req DCM444 */
            }
        } else {
            /* Internal request for type2 transmission and no pdu available right now */
            ret = BUFREQ_BUSY;
        }
        /* Do not stop s3 timer on internal type2 request. */
        if (!intType2Request && (ret == BUFREQ_OK)) {
            stopS3SessionTimer(runtime); /** @req DCM141 */
        }
    }
    Irq_Restore(state);
    return ret;
}

BufReq_ReturnType DslCopyDataToRxBuffer(PduIdType dcmRxPduId, PduInfoType *pduInfoPtr, PduLengthType *rxBufferSizePtr)
{
    BufReq_ReturnType returnCode = BUFREQ_NOT_OK;
    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *mainConnection = NULL;
    const Dcm_DslConnectionType *connection = NULL;
    const Dcm_DslProtocolRowType *protocolRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *runtime = NULL;
    imask_t state;
    Irq_Save(state);

    if (findRxPduIdParentConfigurationLeafs(dcmRxPduId, &protocolRx, &mainConnection, &connection, &protocolRow, &runtime)) {
        const Dcm_DslBufferType *externalRxBuffer = protocolRow->DslProtocolRxBufferID;
        if ((PROVIDED_TO_PDUR == runtime->externalRxBufferStatus ) && (externalRxBuffer->externalBufferRuntimeData->DcmRxPduId == dcmRxPduId)){
            /* Buffer is provided to PDUR. Copy data to buffer */
            uint8 *destPtr = &(runtime->diagnosticRequestFromTester.SduDataPtr[externalRxBuffer->externalBufferRuntimeData->nofBytesHandled]);
            if( 0 == pduInfoPtr->SduLength ) {
                /* Just polling remaining buffer size */
                returnCode = BUFREQ_OK;
            } else if( pduInfoPtr->SduLength <= (externalRxBuffer->pduInfo.SduLength - externalRxBuffer->externalBufferRuntimeData->nofBytesHandled) ) {
                /* Enough room in buffer. Copy data. */
                /** @req DCM443 */
                memcpy(destPtr, pduInfoPtr->SduDataPtr, pduInfoPtr->SduLength);
                /* Inc nof bytes handled byte */
                externalRxBuffer->externalBufferRuntimeData->nofBytesHandled += pduInfoPtr->SduLength;
                returnCode = BUFREQ_OK;
            } else {
                /* Length exceeds number of bytes left in buffer */
                DCM_DET_REPORTERROR(DCM_COPY_RX_DATA_ID, DCM_E_INTERFACE_BUFFER_OVERFLOW);
                returnCode = BUFREQ_NOT_OK;
            }
                *rxBufferSizePtr =
                            externalRxBuffer->pduInfo.SduLength - externalRxBuffer->externalBufferRuntimeData->nofBytesHandled;
        } else if ((runtime->localRxBuffer.status == PROVIDED_TO_PDUR) && (runtime->localRxBuffer.DcmRxPduId == dcmRxPduId)){
            /* Local buffer provided to pdur */
            if(0 == pduInfoPtr->SduLength) {
                /* Just polling remaining buffer size */
                returnCode = BUFREQ_OK;
            } else if(pduInfoPtr->SduLength <= (DCM_DSL_LOCAL_BUFFER_LENGTH - runtime->localRxBuffer.nofBytesHandled)) {
                uint8 *destPtr = &(runtime->localRxBuffer.PduInfo.SduDataPtr[runtime->localRxBuffer.nofBytesHandled]);
                memcpy(destPtr, pduInfoPtr->SduDataPtr, pduInfoPtr->SduLength);
                runtime->localRxBuffer.nofBytesHandled += pduInfoPtr->SduLength;
                returnCode = BUFREQ_OK;
            } else {
                /* Length exceeds buffer size */
                DCM_DET_REPORTERROR(DCM_COPY_RX_DATA_ID, DCM_E_INTERFACE_BUFFER_OVERFLOW);
                returnCode = BUFREQ_NOT_OK;
            }
            *rxBufferSizePtr = DCM_DSL_LOCAL_BUFFER_LENGTH - runtime->localRxBuffer.nofBytesHandled;
        }
    }
    Irq_Restore(state);

    return returnCode;

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'void Dcm_TpRxIndication(PduIdType dcmRxPduId, NotifResultType result)'.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 	This function is called called by the PduR typically when CanTp has
//	received the diagnostic request, copied it to the provided buffer and need to indicate
//	this to the DCM (DSL) module via proprietary API.

void DslTpRxIndicationFromPduR(PduIdType dcmRxPduId, NotifResultType result, boolean internalRequest) {
    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *mainConnection = NULL;
    const Dcm_DslConnectionType *connection = NULL;
    const Dcm_DslProtocolRowType *protocolRow = NULL;
    const Dcm_DspSessionRowType *sessionRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *rxRuntime = NULL;
    Std_ReturnType higherLayerResp;
    imask_t state;

    /** @req DCM345, this needs to be verified when connection to CanIf works. */

    if (findRxPduIdParentConfigurationLeafs(dcmRxPduId, &protocolRx, &mainConnection, &connection, &protocolRow, &rxRuntime)) {
        // We need to find out in what buffer we can find our Rx data (it can
        // be either in the normal RX-buffer or the 'extra' buffer for implementing
        // the Concurrent "Test Present" functionality.
        const Dcm_DslBufferType *externalRxBuffer = protocolRow->DslProtocolRxBufferID;
        Irq_Save(state);
        if ((rxRuntime->externalRxBufferStatus == PROVIDED_TO_PDUR) && (externalRxBuffer->externalBufferRuntimeData->DcmRxPduId == dcmRxPduId)) {
            boolean receivedLenOK =
                    (externalRxBuffer->externalBufferRuntimeData->nofBytesHandled == rxRuntime->diagnosticRequestFromTester.SduLength) ||
                    (0 == rxRuntime->diagnosticRequestFromTester.SduLength);
            PduIdType type2TxPduId = 0xffff;
            boolean type2Tx = (DCM_PROTOCOL_TRANS_TYPE_2 == protocolRow->DslProtocolTransType) && internalRequest;
            if ( (result == NTFRSLT_OK) && receivedLenOK &&
                    (!type2Tx || (E_OK == DslLockType2TxPdu(mainConnection, dcmRxPduId, &type2TxPduId))) ) { /** @req DCM111 */
                if (isTesterPresentCommand(&(protocolRow->DslProtocolRxBufferID->pduInfo))) {
                    startS3SessionTimer(rxRuntime, protocolRow); /** @req DCM141 *//** @req DCM112 *//** @req DCM113 */
                    rxRuntime->externalRxBufferStatus = NOT_IN_USE;
                    externalRxBuffer->externalBufferRuntimeData->DcmRxPduId = DCM_INVALID_PDU_ID;
                    protocolRow->DslProtocolRxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
#if defined(USE_COMM)
                    /* @req DCM169 Not calling ComM_DCM_InactiveDiagnostic when not in default session */
                    if( DCM_DEFAULT_SESSION == rxRuntime->sessionControl ) {
                        if( !rxRuntime->diagnosticActiveComM  ) {
                            ComM_DCM_ActiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM163 */
                            ComM_DCM_InactiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM166 */
                            rxRuntime->diagnosticActiveComM = FALSE;
                        }
                    }
#endif
                } else {
                    if (rxRuntime->protocolStarted == FALSE) {
                        //if there is no active protocol,then start the current protocol
                        if(DcmDslRunTimeData.activeProtocol == NULL){
                            higherLayerResp = StartProtocolHelper(protocolRow->DslProtocolID); /** @req DCM036 */
                            if (higherLayerResp == E_OK) {
                                /** @req DCM146 */ /*Reset security state */
                                /** @req DCM147*/ /* Reset to default session and switch mode */
                                changeDiagnosticSession(rxRuntime, DCM_DEFAULT_SESSION);
                                rxRuntime->protocolStarted = TRUE;
                                DcmDslRunTimeData.activeProtocol = protocolRow;
                                    //if it's OBD diagnostic,change session to DCM_OBD_SESSION
                                    if(DCM_OBD_ON_CAN == protocolRow->DslProtocolID){
                                        rxRuntime->sessionControl = DCM_OBD_SESSION;
                                        //startS3SessionTimer(runtime, protocolRow); // init s3.
                                    }
                            }
                        } else{
                            /*if there is a active protocol and the priority of the current protocol is higher than the priority of the active protocol,
                            then preemption will happen.*/
                            if((protocolRow->DslProtocolPriority < DcmDslRunTimeData.activeProtocol->DslProtocolPriority) ||
                            		DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters->sessionControl == DCM_DEFAULT_SESSION)/** @req DCM625 */{
                                /*@req OBD_DCM_REQ_31*/
                                higherLayerResp = StopProtocolHelper(DcmDslRunTimeData.activeProtocol->DslProtocolID);
                                if (higherLayerResp == E_OK) {
                                    if (DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters->externalTxBufferStatus == NOT_IN_USE){
                                        higherLayerResp = StartProtocolHelper(protocolRow->DslProtocolID); /** @req DCM036 */
                                        if (higherLayerResp == E_OK) {
                                            DcmDslRunTimeData.activeProtocol->DslRunTimeProtocolParameters->protocolStarted = FALSE;	/* Close the last protocol*/
                                            rxRuntime->protocolStarted = TRUE;
                                            DcmDslRunTimeData.activeProtocol = protocolRow;
                                            /** @req DCM146 */ /*Reset security state */
                                            /** @req DCM147*/ /* Reset to default session and switch mode */
                                            changeDiagnosticSession(rxRuntime, DCM_DEFAULT_SESSION);
                                            /*if it's OBD diagnostic,change session to DCM_OBD_SESSION*/
                                            if(DCM_OBD_ON_CAN == protocolRow->DslProtocolID){
                                                rxRuntime->sessionControl = DCM_OBD_SESSION;
                                            }
                                        }
                                    } else {
                                        /*set the flag,activate preemption mechanism*/
                                        PreemptionNotProcessingDone = TRUE;
                                        /*set Tx buffer status PREEMPT_TRANSMIT_NRC*/
                                        rxRuntime->externalTxBufferStatus = PREEMPT_TRANSMIT_NRC;
                                        rxRuntime->responsePendingCount = Dcm_ConfigPtr->Dsl->DslDiagResp->DslDiagRespMaxNumRespPend;
                                        rxRuntime->diagReqestRxPduId = dcmRxPduId;

                                        /*start preemption timer*/
                                        startPreemptTimer(rxRuntime, protocolRow);
                                        /*request PduR to cancel transmit of preempted protocol*/
                                        /*@req OBD_DCM_REQ_32*//*@req OBD_DCM_REQ_33*/
                                        higherLayerResp = PduR_DcmCancelTransmit(DsdDslGetCurrentTxPduId());
                                        (void)higherLayerResp; /* Just to remove lint warning */
                                    }
                                }
                            } else {
                                /*if the priority of the current protocol is lower than the priority of the active protocol, we do nothing.*/
                                rxRuntime->externalRxBufferStatus = NOT_IN_USE;
                                protocolRow->DslProtocolRxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
                            }
                        }
                    }
                    if (rxRuntime->protocolStarted == TRUE) {
#if defined(USE_COMM)
                        if( DCM_DEFAULT_SESSION == rxRuntime->sessionControl ) {
                            if( !rxRuntime->diagnosticActiveComM ) {
                                ComM_DCM_ActiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM163 */
                                rxRuntime->diagnosticActiveComM = TRUE;
                            }
                        }
#endif
                        sessionRow = getActiveSessionRow(rxRuntime);
                        rxRuntime->externalRxBufferStatus = PROVIDED_TO_DSD; /** @req DCM241 */
                        const Dcm_DslBufferType *txBuffer = NULL;
                        rxRuntime->isType2Tx = type2Tx;
                        Dcm_DslRunTimeProtocolParametersType *txRuntime;
                        if( type2Tx ) {
                            /* @req DCM122 */
                            txRuntime = mainConnection->DslPeriodicTransmissionConRef->DslProtocolRow->DslRunTimeProtocolParameters;
                            txBuffer = mainConnection->DslPeriodicTransmissionConRef->DslProtocolRow->DslProtocolTxBufferID;
                            txRuntime->diagResponseTxPduId = type2TxPduId;
                            txRuntime->diagReqestRxPduId = dcmRxPduId;

                        } else {
                            txRuntime = rxRuntime;
                            txRuntime->diagResponseTxPduId = mainConnection->DslProtocolTx->DcmDslProtocolTxPduId;
                            txBuffer = protocolRow->DslProtocolTxBufferID;
                        }
                        rxRuntime->diagResponseTxPduId = txRuntime->diagResponseTxPduId;
                        txRuntime->stateTimeoutCount = DCM_CONVERT_MS_TO_MAIN_CYCLES(sessionRow->DspSessionP2ServerMax); /* See 9.2.2. */
                        if(txRuntime->stateTimeoutCount > 0){
                            txRuntime->stateTimeoutCount--; /* must be sent cycle before timeout */
                        }
                        if (txRuntime->externalTxBufferStatus == NOT_IN_USE) {
                            DEBUG( DEBUG_MEDIUM, "External Tx buffer available, we can pass it to DSD.\n");
                        } else {
                            DEBUG( DEBUG_MEDIUM, "External buffer not available, a response is being transmitted?\n");
                        }
                        if( 0 == rxRuntime->diagnosticRequestFromTester.SduLength ) {
                            /* The original transfer request on call of Dcm_StartOfReception was 0. Set length
                            * of diagnostic request to the number of bytes received. */
                            rxRuntime->diagnosticRequestFromTester.SduLength = externalRxBuffer->externalBufferRuntimeData->nofBytesHandled;
                        }
                        txRuntime->externalTxBufferStatus = PROVIDED_TO_DSD; /** @req DCM241 */
                        txRuntime->responsePendingCount = Dcm_ConfigPtr->Dsl->DslDiagResp->DslDiagRespMaxNumRespPend;
                        txRuntime->diagnosticResponseFromDsd.SduDataPtr = txBuffer->pduInfo.SduDataPtr;
                        txRuntime->diagnosticResponseFromDsd.SduLength = txBuffer->pduInfo.SduLength;
                        DEBUG( DEBUG_MEDIUM, "DsdDslDataIndication(DcmDslProtocolTxPduId=%d, dcmRxPduId=%d)\n", mainConnection->DslProtocolTx->DcmDslProtocolTxPduId, dcmRxPduId);
                        rxRuntime->diagReqestRxPduId = dcmRxPduId;
                        DEBUG(DEBUG_MEDIUM,"\n\n runtime->diagnosticRequestFromTester.SduDataPtr[2]  %x\n\n ",rxRuntime->diagnosticRequestFromTester.SduDataPtr[2]);
                        DsdDslDataIndication(  // qqq: We are inside a critical section.
                            &(rxRuntime->diagnosticRequestFromTester),
                            protocolRow->DslProtocolSIDTable,	/** @req DCM035 */
                            protocolRx->DslProtocolAddrType,
                            txRuntime->diagResponseTxPduId,
                            &(txRuntime->diagnosticResponseFromDsd),
                            dcmRxPduId,
                            protocolRow->DslProtocolTransType,
                            internalRequest,
                            protocolRow->DslSendRespPendOnTransToBoot);
                    }
                }
            } else { /** @req DCM344 */
                /* The indication was not equal to NTFRSLT_OK, the length did not match or no type2 pdu was available,
                 * release the resources and no forward to DSD.*/
                if(!receivedLenOK) {
                    /* Number of bytes received does not match the original request */
                    DCM_DET_REPORTERROR(DCM_TP_RX_INDICATION_ID, DCM_E_TP_LENGTH_MISMATCH);
                }
                DslResetSessionTimeoutTimer(); /** @req DCM141 */
                rxRuntime->externalRxBufferStatus = NOT_IN_USE;
                externalRxBuffer->externalBufferRuntimeData->DcmRxPduId = DCM_INVALID_PDU_ID;
                protocolRow->DslProtocolRxBufferID->externalBufferRuntimeData->status = BUFFER_AVAILABLE;
            }
        } else {
            // It is the local buffer that was provided to the PduR, that buffer
            // is only used for tester present reception in parallel to diagnostic
            // requests.
            if ((rxRuntime->localRxBuffer.status == PROVIDED_TO_PDUR) && (rxRuntime->localRxBuffer.DcmRxPduId == dcmRxPduId)) {
                boolean receivedLenOK = ((0 == rxRuntime->localRxBuffer.PduInfo.SduLength) ||
                        (rxRuntime->localRxBuffer.PduInfo.SduLength == rxRuntime->localRxBuffer.nofBytesHandled));
                if ( (result == NTFRSLT_OK) && receivedLenOK ) { // Make sure that the data in buffer is valid.
                    if (isTesterPresentCommand(&(rxRuntime->localRxBuffer.PduInfo))) {
                        startS3SessionTimer(rxRuntime, protocolRow); /** @req DCM141 *//** @req DCM112 *//** @req DCM113 */
                    }
                } else {
                    if(!receivedLenOK) {
                        /* Number of bytes received does not match the original request */
                        DCM_DET_REPORTERROR(DCM_TP_RX_INDICATION_ID, DCM_E_TP_LENGTH_MISMATCH);
                    }
                    DslResetSessionTimeoutTimer(); /** @req DCM141 */
                }
                rxRuntime->localRxBuffer.status = NOT_IN_USE;
                rxRuntime->localRxBuffer.DcmRxPduId = DCM_INVALID_PDU_ID;
            }
        }
        Irq_Restore(state);
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'BufReq_ReturnType Dcm_CopyTxData(PduIdType dcmTxPduId,
//		PduInfoType *pduInfoPtr, RetryInfoType *retryInfoPtr, PduLengthType *txDataCntPtr)'.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  This TX-buffer request is likely triggered by the transport layer (i.e. CanTp)
//  after PduR_DcmTransmit() has been called (via PduR to CanTp) indicating that something
//  is to be sent. The PduR_DcmTransmit() call is done from the DSL main function when
//  it has detected that the pending request has been answered by DSD
//  (or any other module?).

BufReq_ReturnType DslCopyTxData(PduIdType dcmTxPduId, PduInfoType *pduInfoPtr, RetryInfoType *periodData, PduLengthType *txDataCntPtr)
{
	BufReq_ReturnType ret = BUFREQ_NOT_OK;
	const Dcm_DslProtocolRowType *protocolRow = NULL;
	Dcm_DslRunTimeProtocolParametersType *runtime = NULL;
	//lint -estring(920,pointer)  /* cast to void */
	(void) periodData;//Not used
	//lint +estring(920,pointer)  /* cast to void */

	DEBUG( DEBUG_MEDIUM, "DslProvideTxBuffer=%d\n", dcmTxPduId);
	if (findTxPduIdParentConfigurationLeafs(dcmTxPduId, &protocolRow, &runtime)) {
		const Dcm_DslBufferType *externalTxBuffer = protocolRow->DslProtocolTxBufferID;
		ret = BUFREQ_OK;
		switch (runtime->externalTxBufferStatus) { // ### EXTERNAL TX BUFFER ###
		case DCM_TRANSMIT_SIGNALED:
			externalTxBuffer->externalBufferRuntimeData->nofBytesHandled = 0;
			runtime->externalTxBufferStatus = PROVIDED_TO_PDUR; /** @req DCM349 */
			break;
		case PROVIDED_TO_PDUR:
			break;
		default:
			DEBUG( DEBUG_MEDIUM, "DCM_TRANSMIT_SIGNALED was not signaled in the external buffer\n");
			ret = BUFREQ_NOT_OK;
			break;
		}
		if( BUFREQ_OK == ret ) {
			if( 0 == pduInfoPtr->SduLength ) {
				/* Just polling size of data left to send */
			} else if( pduInfoPtr->SduLength > (runtime->diagnosticResponseFromDsd.SduLength - externalTxBuffer->externalBufferRuntimeData->nofBytesHandled) ) {
				/* Length exceeds the number of bytes still to be sent. */
				ret = BUFREQ_NOT_OK;
			} else {
				/** @req DCM346 */ /* Length verification is already done if this state is reached. */
				memcpy(pduInfoPtr->SduDataPtr, &runtime->diagnosticResponseFromDsd.SduDataPtr[externalTxBuffer->externalBufferRuntimeData->nofBytesHandled], pduInfoPtr->SduLength);
				externalTxBuffer->externalBufferRuntimeData->nofBytesHandled += pduInfoPtr->SduLength;
			}
			*txDataCntPtr = runtime->diagnosticResponseFromDsd.SduLength - externalTxBuffer->externalBufferRuntimeData->nofBytesHandled;
		} else if (ret == BUFREQ_NOT_OK) {
			ret = BUFREQ_OK;
			switch (runtime->localTxBuffer.status) { // ### LOCAL TX BUFFER ###
			case DCM_TRANSMIT_SIGNALED: {
				runtime->localTxBuffer.PduInfo.SduDataPtr = runtime->localTxBuffer.buffer;
				runtime->localTxBuffer.nofBytesHandled = 0;
				runtime->localTxBuffer.status = PROVIDED_TO_PDUR; // Now the DSL should not touch this Tx-buffer anymore.
				break;
			}
			case PROVIDED_TO_PDUR:
				break;
			default:
				DEBUG( DEBUG_MEDIUM, "DCM_TRANSMIT_SIGNALED was not signaled for the local buffer either\n");
				ret = BUFREQ_NOT_OK;
				break;
			}
			if( BUFREQ_OK == ret ) {
				if( 0 == pduInfoPtr->SduLength ) {
					/* Just polling size of data left to send */
				} else if( pduInfoPtr->SduLength > (runtime->localTxBuffer.PduInfo.SduLength - runtime->localTxBuffer.nofBytesHandled) ) {
					/* Length exceeds the number of bytes still to be sent. */
					ret = BUFREQ_NOT_OK;
				} else {
					memcpy(pduInfoPtr->SduDataPtr, &runtime->localTxBuffer.PduInfo.SduDataPtr[runtime->localTxBuffer.nofBytesHandled], pduInfoPtr->SduLength);
					runtime->localTxBuffer.nofBytesHandled += pduInfoPtr->SduLength;
				}
				*txDataCntPtr = runtime->localTxBuffer.PduInfo.SduLength - runtime->localTxBuffer.nofBytesHandled;
			}
		}
	}
	return ret;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//	Implements 'void Dcm_TpTxConfirmation(PduIdType dcmTxPduId, NotifResultType result))'.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 	This function is called by the PduR (which has been trigged by i.e. CanTp)
// 	when a transmission has been successfully finished, have had errors or
// 	is even stopped.

void DslTpTxConfirmation(PduIdType dcmTxPduId, NotifResultType result) {

    /* @req DCM170 Not calling ComM_DCM_InactiveDiagnostic */

    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *rxMainConnection = NULL;
    const Dcm_DslConnectionType *rxConnection = NULL;
    const Dcm_DslProtocolRowType *txProtocolRow = NULL;
    const Dcm_DslProtocolRowType *rxProtocolRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *txRuntime = NULL;
    Dcm_DslRunTimeProtocolParametersType *rxRuntime = NULL;
    imask_t state;

    DEBUG( DEBUG_MEDIUM, "DslTxConfirmation=%d, result=%d\n", dcmTxPduId, result);
    if( !IS_PERIODIC_TX_PDU(dcmTxPduId)
#if !(defined(DCM_USE_TYPE2_PERIODIC_TRANSMISSION) && defined(DCM_TYPE2_TX_ROUTED_TO_IF))
            || TRUE
#endif
            ) {
        if (findTxPduIdParentConfigurationLeafs(dcmTxPduId, &txProtocolRow, &txRuntime) &&
                findRxPduIdParentConfigurationLeafs(txRuntime->diagReqestRxPduId, &protocolRx, &rxMainConnection, &rxConnection, &rxProtocolRow, &rxRuntime)) {
            boolean externalBufferReleased = FALSE;
            const Dcm_DslBufferType *externalTxBuffer = txProtocolRow->DslProtocolTxBufferID;
            // Free the buffer and free the Pdu runtime data buffer.
            Irq_Save(state);
            switch (txRuntime->externalTxBufferStatus) { // ### EXTERNAL TX BUFFER ###
            case PROVIDED_TO_PDUR: {
                if( txRuntime->diagnosticResponseFromDsd.SduLength != externalTxBuffer->externalBufferRuntimeData->nofBytesHandled ) {
                    DCM_DET_REPORTERROR(DCM_TP_TX_CONFIRMATION_ID, DCM_E_TP_LENGTH_MISMATCH);
                }
#if defined(USE_COMM)
                if(  DCM_DEFAULT_SESSION == rxRuntime->sessionControl ) {
                    if( findProtocolRx(rxRuntime->diagReqestRxPduId, &protocolRx) ) {
                        if( rxRuntime->diagnosticActiveComM ) {
                            ComM_DCM_InactiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM164 */
                            rxRuntime->diagnosticActiveComM = FALSE;
                        }
                    }
                }
#endif
                releaseExternalRxTxBuffers(rxProtocolRow, rxRuntime); /** @req DCM118 *//** @req DCM352 *//** @req DCM353 *//** @req DCM354 */
                externalBufferReleased = TRUE;
                if(IS_PERIODIC_TX_PDU(dcmTxPduId)) {
                    DsdDataConfirmation(Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolPeriodicTxGlobalList[TO_INTERNAL_PERIODIC_PDU(dcmTxPduId)].PduRTxPduId, result);
                    DslReleaseType2TxPdu(dcmTxPduId);
                } else {
                    /* Only start s3 timer if non-periodic */
                    startS3SessionTimer(rxRuntime, rxProtocolRow); /** @req DCM141 */
                    DsdDataConfirmation(rxMainConnection->DslProtocolTx->DcmDslProtocolTxPduId, result); /** @req DCM117 *//** @req DCM235 */
                }
                break;
            }
            default:
                break;
            }
            if (!externalBufferReleased) {
                switch (txRuntime->localTxBuffer.status) { // ### LOCAL TX BUFFER ###
                case PROVIDED_TO_PDUR:
                    if( txRuntime->localTxBuffer.PduInfo.SduLength != txRuntime->localTxBuffer.nofBytesHandled ) {
                        DCM_DET_REPORTERROR(DCM_TP_TX_CONFIRMATION_ID, DCM_E_TP_LENGTH_MISMATCH);
                    }
#if defined(USE_COMM)
                    if( DCM_E_RESPONSEPENDING != txRuntime->localTxBuffer.responseCode ) {
                        if( DCM_DEFAULT_SESSION == rxRuntime->sessionControl ) {
                            if( findProtocolRx(rxRuntime->diagReqestRxPduId, &protocolRx) ) {
                                if( rxRuntime->diagnosticActiveComM ) {
                                    ComM_DCM_InactiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM165 */
                                    rxRuntime->diagnosticActiveComM = FALSE;
                                }
                            }
                        }
                    }
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
                    else {
                        /* Tx confirmation on response pending */
                        DsdResponsePendingConfirmed(rxMainConnection->DslProtocolTx->DcmDslProtocolTxPduId, result);
                    }
#endif
#endif
                    DEBUG( DEBUG_MEDIUM, "Released local buffer buffer OK!\n");
                    txRuntime->localTxBuffer.status = NOT_IN_USE;
                    break;
                default:
                    DEBUG( DEBUG_MEDIUM, "WARNING! DslTxConfirmation could not release external or local buffer!\n");
                    break;
                }
            }
            Irq_Restore(state);
        }
    }
#if defined(DCM_USE_TYPE2_PERIODIC_TRANSMISSION) && defined(DCM_TYPE2_TX_ROUTED_TO_IF)
    else {
        /* It is a periodic pdu. It is assumed that for these the buffers are released as soon as lower layer accepts
         * transmit request.  */
        PduIdType requestRxPduId = 0;
        Irq_Save(state);
        if( (E_OK == DslGetType2TxUserRxPdu(dcmTxPduId, &requestRxPduId)) && findRxPduIdParentConfigurationLeafs(requestRxPduId, &protocolRx, &rxMainConnection, &rxConnection, &rxProtocolRow, &rxRuntime)) {
#if defined(USE_COMM)
            if(  DCM_DEFAULT_SESSION == rxRuntime->sessionControl ) {
                if( rxRuntime->diagnosticActiveComM ) {
                    ComM_DCM_InactiveDiagnostic(protocolRx->DcmDslProtocolRxComMChannel); /** @req DCM164 */
                    rxRuntime->diagnosticActiveComM = FALSE;
                }
            }
#endif
            DsdDataConfirmation(Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolPeriodicTxGlobalList[TO_INTERNAL_PERIODIC_PDU(dcmTxPduId)].PduRTxPduId, result);
        }
        Irq_Restore(state);
        DslReleaseType2TxPdu(dcmTxPduId);
    }
#endif
}


#if (DCM_MANUFACTURER_NOTIFICATION == STD_ON) || (DCM_USE_JUMP_TO_BOOT == STD_ON)
void Arc_DslGetRxConnectionParams(PduIdType rxPduId, uint16* sourceAddress, Dcm_ProtocolAddrTypeType* reqType) {
    const Dcm_DslProtocolRxType *protocolRx = NULL;
    const Dcm_DslMainConnectionType *mainConnection = NULL;
    const Dcm_DslConnectionType *connection = NULL;
    const Dcm_DslProtocolRowType *protocolRow = NULL;
    Dcm_DslRunTimeProtocolParametersType *runtime = NULL;

    if (findRxPduIdParentConfigurationLeafs(rxPduId, &protocolRx, &mainConnection, &connection, &protocolRow, &runtime)) {
        *sourceAddress = mainConnection->DslRxTesterSourceAddress;
        *reqType = protocolRx->DslProtocolAddrType;
    }
    else {
        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
    }
}

#endif

/**
 * Tries to find a physical protocol given a protocol id and a tester source address
 * @param protocolId
 * @param testerSourceAddress
 * @param protocolRow
 * @param protocolPhysRx
 * @return TRUE: Protocol found, FALSE: Protocol not found
 */
static boolean findPhysRxProtocol(uint8 protocolId, uint16 testerSourceAddress, const Dcm_DslProtocolRowType **protocolRow, const Dcm_DslProtocolRxType **protocolPhysRx)
{
    const Dcm_DslProtocolRowType *protocolRowEntry;
    const Dcm_DslConnectionType *connection = NULL;
    boolean found = FALSE;
    protocolRowEntry = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRowList;
    while(!protocolRowEntry->Arc_EOL && !found) {
        /* Find matching protocol */
        if(protocolId == protocolRowEntry->DslProtocolID) {
            if( NULL != protocolRowEntry->DslConnections) {
                /* Find connection with matching source address */
                connection = protocolRowEntry->DslConnections;
                while(!connection->Arc_EOL && !found) {
                    if( (NULL != connection->DslMainConnection) &&
                            (testerSourceAddress == connection->DslMainConnection->DslRxTesterSourceAddress) &&
                            (NULL != connection->DslMainConnection->DslPhysicalProtocolRx)) {
                        /* Match. */
                        *protocolRow = connection->DslProtocolRow;
                        *protocolPhysRx = connection->DslMainConnection->DslPhysicalProtocolRx;
                        found = TRUE;
                    }
                    connection++;
                }
            }
        }
        protocolRowEntry++;
    }
    return found;
}

/**
 * Starts a protocol when starting up as a consequence of jump from bootloader/application.
 * This function should only be used for that purpose
 * @param session
 * @param protocolId
 * @param testerSourceAddress
 * @param requestFullComm
 * @return E_OK: Protocol could be started, E_NOT_OK: Protocol start failed
 */
Std_ReturnType DslDspSilentlyStartProtocol(uint8 session, uint8 protocolId, uint16 testerSourceAddress, boolean requestFullComm)
{
    /* IMPROVEMENT: Check if session is supported */
    const Dcm_DslProtocolRxType *protocolPhysRx = NULL;
    const Dcm_DslProtocolRowType *rxProtocolRow = NULL;
    Std_ReturnType ret = E_NOT_OK;
    if( findPhysRxProtocol(protocolId, testerSourceAddress, &rxProtocolRow, &protocolPhysRx) ) {
        /* Check if the session is supported */
        if( DspDslCheckSessionSupported(session) ) {
            rxProtocolRow->DslRunTimeProtocolParameters->protocolStarted = TRUE;
            rxProtocolRow->DslRunTimeProtocolParameters->sessionControl = session;
            rxProtocolRow->DslRunTimeProtocolParameters->securityLevel = DCM_SEC_LEV_LOCKED;
            DcmDslRunTimeData.activeProtocol = rxProtocolRow;
            if( DCM_DEFAULT_SESSION != session ) {
                startS3SessionTimer(rxProtocolRow->DslRunTimeProtocolParameters, rxProtocolRow);
            }
        } else {
            /* Session is not supported.  */
            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_INTEGRATION_ERROR);
        }
        if(requestFullComm) {
            /* Should request full communication from ComM */
            /* @req DCM537 */
#if defined(USE_COMM)
            ComM_DCM_ActiveDiagnostic(protocolPhysRx->DcmDslProtocolRxComMChannel);
            rxProtocolRow->DslRunTimeProtocolParameters->diagnosticActiveComM = TRUE;
#endif
        }
        ret = E_OK;
    }
    return ret;
}

/**
 * Updates communication mode for a network
 * @param network
 * @param comMode
 */
#if defined(USE_COMM)
void DslComModeEntered(uint8 network, DcmComModeType comMode)
{
    const Dcm_DslProtocolRxType *protocolRx;
    boolean done = FALSE;
    protocolRx = Dcm_ConfigPtr->Dsl->DslProtocol->DslProtocolRxGlobalList;
    while(!protocolRx->Arc_EOL && !done) {
        if( protocolRx->DcmDslProtocolRxComMChannel == network ) {
            NetWorkComMode[protocolRx->ComMChannelInternalIndex] = comMode;
            done = TRUE;
        }
        protocolRx++;
    }
}
#endif
/**
 * Injects diagnostic request on Dcm startup
 * @param sid
 * @param subFnc
 * @param protocolId
 * @param testerSourceAddress
 * @return E_OK: Request processed, E_PENDING: Further calls required, E_NOT_OK: request failed
 */
Std_ReturnType DslDspResponseOnStartupRequest(uint8 sid, uint8 subFnc, uint8 protocolId, uint16 testerSourceAddress)
{
    Std_ReturnType ret = E_NOT_OK;
    const Dcm_DslProtocolRxType *protocolPhysRx = NULL;
    const Dcm_DslProtocolRowType *rxProtocolRow = NULL;
    if( findPhysRxProtocol(protocolId, testerSourceAddress, &rxProtocolRow, &protocolPhysRx) ) {
        ret = E_PENDING;
#if defined(USE_COMM)
        /* @req DCM767 */
        if( DCM_FULL_COM == NetWorkComMode[protocolPhysRx->ComMChannelInternalIndex] )

#endif
        {
            PduLengthType rxBufferSize;
            if(BUFREQ_OK == DslStartOfReception(protocolPhysRx->DcmDslProtocolRxPduId, 2, &rxBufferSize, TRUE)) {
                PduInfoType request;
                uint8 data[2];
                request.SduDataPtr = data;
                request.SduDataPtr[0] = sid;
                request.SduDataPtr[1] = subFnc;
                request.SduLength = 2;
                if( BUFREQ_OK == DslCopyDataToRxBuffer(protocolPhysRx->DcmDslProtocolRxPduId, &request, &rxBufferSize) ) {
                    DslTpRxIndicationFromPduR(protocolPhysRx->DcmDslProtocolRxPduId, NTFRSLT_OK, TRUE );
                } else {
                    /* Something went wrong. Indicate that the reception was
                     * not ok. */
                    DslTpRxIndicationFromPduR(protocolPhysRx->DcmDslProtocolRxPduId, NTFRSLT_E_NOT_OK, TRUE);
                }
                ret = E_OK;
            }
        }
    }
    return ret;
}
