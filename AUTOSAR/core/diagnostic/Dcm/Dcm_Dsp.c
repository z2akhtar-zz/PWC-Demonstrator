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

/* Disable MISRA 2004 rule 16.2, MISRA 2012 rule 17.2.
 * This because of recursive calls to readDidData.
 *  */
//lint -estring(974,*recursive*)

/** @req DCM273 */ /** @req DCM272 */
/** @req DCM039 */ /** @req DCM038 */ /** @req DCM269 */
/** @req DCM271 */ /** @req DCM274 */ /** @req DCM275 */
/** @req DCM007 */
#include <string.h>
#include "Dcm.h"
#include "Dcm_Internal.h"
#if defined(DCM_USE_SERVICE_CLEARDIAGNOSTICINFORMATION) || defined(DCM_USE_SERVICE_READDTCINFORMATION) || defined(DCM_USE_SERVICE_CONTROLDTCSETTING)
#if defined(USE_DEM)
#include "Dem.h"
#else
#warning Dcm: UDS services ClearDiagnosticInformation, ReadDTCInformation and/or ControlDTCSetting will not work without Dem.
#endif
#endif
#include "MemMap.h"
#if defined(USE_MCU)
#include "Mcu.h"
#endif
#ifndef DCM_NOT_SERVICE_COMPONENT
#include "Rte_Dcm.h"
#endif

/*
 * Macros
 */
#define ZERO_SUB_FUNCTION				0x00
#define DCM_FORMAT_LOW_MASK			0x0F
#define DCM_FORMAT_HIGH_MASK			0xF0
#define DCM_MEMORY_ADDRESS_MASK		0x00FFFFFF
#define DCM_DID_HIGH_MASK 				0xFF00			
#define DCM_DID_LOW_MASK				0xFF
#define DCM_PERODICDID_HIGH_MASK		0xF200
#define SID_AND_DDDID_LEN   0x4
#define SDI_AND_MS_LEN   0x4

#define SID_AND_SDI_LEN   0x6
#define SID_AND_PISDR_LEN   0x7

/* == Parser macros == */
/* General */
#define SID_INDEX 0
#define SID_LEN 1
#define SF_INDEX 1
#define SF_LEN 1
#define PID_BUFFER_SIZE 255 // OBD
#define DTC_LEN 3
#define FF_REC_NUM_LEN 1
/* Read/WriteMemeoryByAddress */
#define ALFID_INDEX 1
#define ALFID_LEN 1
#define ADDR_START_INDEX 2
/* DynamicallyDefineDataByIdentifier */
#define DDDDI_INDEX 2
#define DDDDI_LEN 2
#define DYNDEF_ALFID_INDEX 4
#define DYNDEF_ADDRESS_START_INDEX 5
/* InputOutputControlByIdentifier */
#define IOI_INDEX 1
#define IOI_LEN 2
#define IOCP_INDEX 3
#define IOCP_LEN 1
#define COR_INDEX 4
#define IS_VALID_IOCTRL_PARAM(_x) ((_x) <= DCM_SHORT_TERM_ADJUSTMENT)
#define TO_SIGNAL_BIT(_x) (uint8)(1u<<(7u-((_x)%8u)))

/*OBD RequestCurrentPowertrainDiagnosticData*/
#define PIDZERO								0
#define DATAZERO							0
#define INFOTYPE_ZERO						0
#define PID_LEN								1
#define RECORD_NUM 							0
#define HALF_BYTE 							4
#define OFFSET_ONE_BYTE						8
#define OFFSET_TWO_BYTES 					16
#define OFFSET_THREE_BYTES					24
#define SUPPRTED_PIDS_DATA_LEN				4
#define LEAST_BIT_MASK  					((uint8)0x01u)
#define OBD_DATA_LSB_MASK 					((uint32)0x000000FFu)
#define OBD_REQ_MESSAGE_LEN_ONE_MIN 		2
#define OBD_REQ_MESSAGE_LEN_MAX  			7
#define AVAIL_TO_SUPPORTED_PID_OFFSET_MIN  	0x01
#define AVAIL_TO_SUPPORTED_PID_OFFSET_MAX  	0x20
#define AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MIN  	0x01
#define AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MAX  	0x20
#define MAX_REQUEST_PID_NUM 				6
#define LENGTH_OF_DTC  						2

/* CommunicationControl */
#define CC_CTP_INDEX 2
#define COMM_CTRL_ISO_RES_SF_LOW    0x04
#define COMM_CTRL_ISO_RES_SF_HIGH   0x3F
#define COMM_CTRL_ISO_RES_SF        0x7F
#define IS_IN_ISO_RESERVED_RANGE(_x)    ((_x >= COMM_CTRL_ISO_RES_SF_LOW) && (_x <= COMM_CTRL_ISO_RES_SF_HIGH))
#define IS_ISO_RESERVED(_x) (IS_IN_ISO_RESERVED_RANGE(_x) || (COMM_CTRL_ISO_RES_SF == _x))

/*OBD RequestCurrentPowertrainDiagnosticData*/
#define FF_NUM_LEN							1
#define OBD_DTC_LEN							2
#define OBD_SERVICE_TWO 					((uint8)0x02u)
#define MAX_PID_FFNUM_NUM					3
#define OBD_REQ_MESSAGE_LEN_TWO_MIN			3
#define DATA_ELEMENT_INDEX_OF_PID_NOT_SUPPORTED  0

#if defined(DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA)
#define DCM_MAX_PID_NUM_IN_FF				DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA
#else
#define DCM_MAX_PID_NUM_IN_FF				0
#endif

/*OBD RequestEmissionRelatedDiagnosticTroubleCodes service03 07*/
#define EMISSION_DTCS_HIGH_BYTE(dtc)		(((uint32)(dtc) >> 8) & 0xFFu)
#define EMISSION_DTCS_LOW_BYTE(dtc)			((uint32)(dtc) & 0xFFu)
#define OBD_RESPONSE_DTC_MAX_NUMS			126

/*OBD Requestvehicleinformation service09*/
#define OBD_TX_MAXLEN						0xFF
#define MAX_REQUEST_VEHINFO_NUM				6
#define OBD_SERVICE_FOUR 					0x04
#define OBD_VIN_LENGTH						17

#define IS_AVAILABILITY_PID(_x) ( (0 == (_x % 0x20)) && (_x <= 0xE0))
#define IS_AVAILABILITY_INFO_TYPE(_x) IS_AVAILABILITY_PID(_x)

#define BYTES_TO_DTC(hb, mb, lb)	(((uint32)(hb) << 16) | ((uint32)(mb) << 8) | (uint32)(lb))
#define DTC_HIGH_BYTE(dtc)			(((uint32)(dtc) >> 16) & 0xFFu)
#define DTC_MID_BYTE(dtc)			(((uint32)(dtc) >> 8) & 0xFFu)
#define DTC_LOW_BYTE(dtc)			((uint32)(dtc) & 0xFFu)

/* UDS ReadDataByPeriodicIdentifier */
#define TO_PERIODIC_DID(_x) (DCM_PERODICDID_HIGH_MASK + (uint16)(_x))
/* Maximum length for periodic Dids */
/* CAN */
#define MAX_TYPE2_PERIODIC_DID_LEN_CAN 7
#define MAX_TYPE1_PERIODIC_DID_LEN_CAN 5

/* Flexray */
/* IMPROVEMENT: Maximum length for flexray? */
#define MAX_TYPE2_PERIODIC_DID_LEN_FLEXRAY 0
#define MAX_TYPE1_PERIODIC_DID_LEN_FLEXRAY 0

/* Ip */
/* IMPROVEMENT: Maximum length for ip? */
#define MAX_TYPE2_PERIODIC_DID_LEN_IP 0
#define MAX_TYPE1_PERIODIC_DID_LEN_IP 0

#define TIMER_DECREMENT(timer) \
        if (timer >= DCM_MAIN_FUNCTION_PERIOD_TIME_MS) { \
            timer = timer - DCM_MAIN_FUNCTION_PERIOD_TIME_MS; \
        } \

typedef enum {
	DCM_READ_MEMORY = 0,
	DCM_WRITE_MEMORY,
} DspMemoryServiceType;

typedef enum {
	DCM_DSP_RESET_NO_RESET,
	DCM_DSP_RESET_PENDING,
	DCM_DSP_RESET_WAIT_TX_CONF,
} DcmDspResetStateType;

typedef struct {
	DcmDspResetStateType resetPending;
	PduIdType resetPduId;
	PduInfoType *pduTxData;
	Dcm_EcuResetType resetType;
} DspUdsEcuResetDataType;

typedef enum {
    DCM_JTB_IDLE,
    DCM_JTB_WAIT_RESPONSE_PENDING_TX_CONFIRM,
    DCM_JTB_EXECUTE
}DspJumpToBootState;

typedef struct {
	boolean sessionPending;
	PduIdType sessionPduId;
	Dcm_SesCtrlType session;
	DspJumpToBootState jumpToBootState;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
} DspUdsSessionControlDataType;


typedef enum {
    DCM_WRITE_DID_IDLE,
    DCM_WRITE_DID_PENDING,
} WriteDidPendingStateType;

typedef struct {
	ReadDidPendingStateType state;
	const PduInfoType* pduRxData;
	PduInfoType* pduTxData;
	uint16 txWritePos;
	uint16 nofReadDids;
	uint16 reqDidIndex;
	uint16 pendingDid;
	uint16 pendingDataLength;
	uint16 pendingSignalIndex;
	uint16 pendingDataStartPos;
} DspUdsReadDidPendingType;

typedef struct {
    WriteDidPendingStateType state;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
} DspUdsWriteDidPendingType;

typedef enum {
    DCM_GENERAL_IDLE,
    DCM_GENERAL_PENDING,
    DCM_GENERAL_FORCE_RCRRP_AWAITING_SEND,
    DCM_GENERAL_FORCE_RCRRP,
} GeneralPendingStateType;

typedef enum {
    DCM_ROUTINE_CONTROL_IDLE,
    DCM_ROUTINE_CONTROL_PENDING,
} RoutineControlPendingStateType;
typedef struct {
    RoutineControlPendingStateType state;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
} DspUdsRoutineControlPendingType;

static DspUdsEcuResetDataType dspUdsEcuResetData;
static DspUdsSessionControlDataType dspUdsSessionControlData;
static DspUdsReadDidPendingType dspUdsReadDidPending;
#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
static DspUdsWriteDidPendingType dspUdsWriteDidPending;
#endif
static DspUdsRoutineControlPendingType dspUdsRoutineControlPending;


typedef enum {
    DELAY_TIMER_DEACTIVE,
    DELAY_TIMER_ON_BOOT_ACTIVE,
    DELAY_TIMER_ON_EXCEEDING_LIMIT_ACTIVE
}DelayTimerActivation;

typedef struct {
    uint8                           secAcssAttempts; //Counter for number of false attempts
    uint32                          timerSecAcssAttempt; //Timer after exceededNumberOfAttempts
    DelayTimerActivation            startDelayTimer; //Flag to indicate Delay timer is active
}DspUdsSecurityAccessChkParam;

typedef struct {
	boolean 						reqInProgress;
	Dcm_SecLevelType				reqSecLevel;
#if (DCM_SECURITY_EOL_INDEX != 0)
	DspUdsSecurityAccessChkParam    secFalseAttemptChk[DCM_SECURITY_EOL_INDEX];
    uint8                           currSecLevIdx; //Current index for secFalseAttemptChk
#endif
	const Dcm_DspSecurityRowType	*reqSecLevelRef;
} DspUdsSecurityAccessDataType;

static DspUdsSecurityAccessDataType dspUdsSecurityAccesData;

typedef enum{
	DCM_MEMORY_UNUSED,
	DCM_MEMORY_READ,
	DCM_MEMORY_WRITE,
	DCM_MEMORY_FAILED	
}Dcm_DspMemoryStateType;
static Dcm_DspMemoryStateType dspMemoryState;

typedef enum{
	DCM_DDD_SOURCE_DEFAULT,
	DCM_DDD_SOURCE_DID,
	DCM_DDD_SOURCE_ADDRESS
}Dcm_DspDDDSourceKindType;

typedef struct{
	uint32 PDidTxCounter;
	uint32 PDidTxPeriod;
	PduIdType PDidRxPduID;
	uint8  PeriodicDid;
}Dcm_pDidType;/* a type to save  the periodic DID and cycle */

typedef struct{
	Dcm_pDidType dspPDid[DCM_LIMITNUMBER_PERIODDATA];	/*a buffer to save the periodic DID and cycle   */
	uint8 PDidNofUsed;										/* note the number of periodic DID is used */
	uint8 nextStartIndex;
}Dsp_pDidRefType;

static Dsp_pDidRefType dspPDidRef;

typedef struct{
	uint8   formatOrPosition;						/*note the formate of address and size*/
	uint8	memoryIdentifier;
	uint32 SourceAddressOrDid;								/*note the memory address */
	uint16 Size;										/*note the memory size */
	Dcm_DspDDDSourceKindType DDDTpyeID;
}Dcm_DspDDDSourceType;

typedef struct{
	uint16 DynamicallyDid;
	Dcm_DspDDDSourceType DDDSource[DCM_MAX_DDDSOURCE_NUMBER];
}Dcm_DspDDDType;

#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static Dcm_DspDDDType dspDDD[DCM_MAX_DDD_NUMBER];
#endif

#ifdef DCM_USE_CONTROL_DIDS
typedef uint8 Dcm_DspIOControlVector[(DCM_MAX_IOCONTROL_DID_SIGNALS + 7) / 8];
typedef struct {
    uint16 did;
    boolean controlActive;
    Dcm_DspIOControlVector activeSignalBitfield;
}Dcm_DspIOControlStateType;
static Dcm_DspIOControlStateType IOControlStateList[DCM_NOF_IOCONTROL_DIDS];
#endif

static Dcm_ProgConditionsType GlobalProgConditions;

static GeneralPendingStateType ProgConditionStartupResponseState;
/*
 * * static Function
 */
#ifndef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
#define LookupDDD(_x,  _y ) FALSE
#define readDDDData(_x, _y, _z) DCM_E_GENERALREJECT
#else
static boolean LookupDDD(uint16 didNr, const Dcm_DspDDDType **DDid);
#endif
static Dcm_NegativeResponseCodeType checkAddressRange(DspMemoryServiceType serviceType, uint8 memoryIdentifier, uint32 memoryAddress, uint32 length);
static const Dcm_DspMemoryRangeInfo* findRange(const Dcm_DspMemoryRangeInfo *memoryRangePtr, uint32 memoryAddress, uint32 length);
static Dcm_NegativeResponseCodeType writeMemoryData(Dcm_OpStatusType* OpStatus, uint8 memoryIdentifier, uint32 MemoryAddress, uint32 MemorySize, uint8 *SourceData);
static void DspCancelPendingDid(uint16 didNr, uint16 signalIndex, ReadDidPendingStateType pendingState, PduInfoType *pduTxData );
static void DspCancelPendingRoutine(const PduInfoType *pduRxData, PduInfoType *pduTxData);

#ifdef DCM_USE_CONTROL_DIDS
static void DspStopInputOutputControl(boolean checkSessionAndSecLevel);
#endif
#ifdef DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER
static boolean checkPDidSupported(uint16 pDid, uint16 *didLength, Dcm_NegativeResponseCodeType *responseCode);
static void DspPdidRemove(uint8 pDid, PduIdType rxPduId);
static void DspStopPeriodicDids(boolean checkSessionAndSecLevel);
#endif
/*
*   end  
*/

//
// This function reset diagnostic activity on session transition.
//This function should be called after the session and security level have been changed
//
//
void DspResetDiagnosticActivityOnSessionChange(Dcm_SesCtrlType newSession)
{
    (void)newSession;
#ifdef DCM_USE_CONTROL_DIDS
    /* DCM628 says that active control should be stopped on transition
     * to default session only. But stop it if active control is not
     * supported in the new session (which should be the current session
     * as it is assumed that session is changed before calling this function) or
     * in the new security level. */
    DspStopInputOutputControl(TRUE);
#endif
#ifdef DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER
    DspStopPeriodicDids(TRUE);
#endif

#ifdef DCM_USE_SERVICE_RESPONSEONEVENT
    /* Stop the response on event service */
    if (DCM_ROE_IsActive()) {
        (void)Dcm_StopROE();
    }
#endif

    dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
    dspUdsSessionControlData.sessionPending = FALSE;
    ProgConditionStartupResponseState = DCM_GENERAL_IDLE;
}
/* Resets active diagnostics on protocol preemtion */
void DcmDspResetDiagnosticActivity(void)
{
#ifdef DCM_USE_CONTROL_DIDS
    DspStopInputOutputControl(FALSE);
#endif
#ifdef DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER
    DspStopPeriodicDids(FALSE);
#endif
#ifdef DCM_USE_SERVICE_RESPONSEONEVENT
    /* Stop the response on event service */
    if (DCM_ROE_IsActive()) {
        (void)Dcm_StopROE();
    }
#endif

    dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
    dspUdsSessionControlData.sessionPending = FALSE;
    ProgConditionStartupResponseState = DCM_GENERAL_IDLE;
}

#ifdef DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER
typedef struct {
    PduIdType rxPduId;
    uint8 pDid;
}pDidType;

static void DspStopPeriodicDids(boolean checkSessionAndSecLevel)
{
    uint16 didLength;
    Dcm_NegativeResponseCodeType resp;
    pDidType pDidsToRemove[DCM_LIMITNUMBER_PERIODDATA];
    uint8 nofPDidsToRemove = 0;
    memset(pDidsToRemove, 0, sizeof(pDidsToRemove));
    if( checkSessionAndSecLevel && DsdDspCheckServiceSupportedInActiveSessionAndSecurity(SID_READ_DATA_BY_PERIODIC_IDENTIFIER) ) {
        for(uint8 i = 0; i < dspPDidRef.PDidNofUsed; i++) {
            resp = DCM_E_REQUESTOUTOFRANGE;
            if( !(checkPDidSupported(TO_PERIODIC_DID(dspPDidRef.dspPDid[i].PeriodicDid), &didLength, &resp) && (DCM_E_POSITIVERESPONSE == resp)) ) {
                /* Not supported */
                pDidsToRemove[nofPDidsToRemove].pDid = dspPDidRef.dspPDid[i].PeriodicDid;
                pDidsToRemove[nofPDidsToRemove++].rxPduId = dspPDidRef.dspPDid[i].PDidRxPduID;
            }
        }
        for( uint8 i = 0; i < nofPDidsToRemove; i++ ) {
            DspPdidRemove(pDidsToRemove[i].pDid, pDidsToRemove[i].rxPduId);
        }
    } else {
        /* Should not check session and security or service not supported in the current session or security.
         * Clear all. */
        memset(&dspPDidRef,0,sizeof(dspPDidRef));
    }
}
#endif

void DspInit(boolean firstCall)
{
    dspUdsSecurityAccesData.reqInProgress = FALSE;
    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
    dspUdsSessionControlData.sessionPending = FALSE;

    dspMemoryState=DCM_MEMORY_UNUSED;
    /* clear periodic send buffer */
    memset(&dspPDidRef,0,sizeof(dspPDidRef));
#ifdef DCM_USE_CONTROL_DIDS
    if(firstCall) {
        memset(IOControlStateList, 0, sizeof(IOControlStateList));
    }
#endif
#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
    /* clear dynamically Did buffer */
    memset(&dspDDD[0],0,sizeof(dspDDD));
#endif

#if (DCM_SECURITY_EOL_INDEX != 0)
    uint8 temp = 0;
    if (firstCall) {
        //Reset the security access attempts
        do {
            dspUdsSecurityAccesData.secFalseAttemptChk[temp].secAcssAttempts = 0;
            if (Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[temp].DspSecurityDelayTimeOnBoot >= DCM_MAIN_FUNCTION_PERIOD_TIME_MS) {
                dspUdsSecurityAccesData.secFalseAttemptChk[temp].timerSecAcssAttempt = Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[temp].DspSecurityDelayTimeOnBoot;
                dspUdsSecurityAccesData.secFalseAttemptChk[temp].startDelayTimer = DELAY_TIMER_ON_BOOT_ACTIVE;
            }
            else {
                dspUdsSecurityAccesData.secFalseAttemptChk[temp].startDelayTimer = DELAY_TIMER_DEACTIVE;
            }
            temp++;
        } while (temp < DCM_SECURITY_EOL_INDEX);
        dspUdsSecurityAccesData.currSecLevIdx = 0;
    }
#else
    (void)firstCall;
#endif

    dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
    ProgConditionStartupResponseState = DCM_GENERAL_IDLE;
    if(firstCall) {
        /* @req DCM536 */
        if(DCM_WARM_START == Dcm_GetProgConditions(&GlobalProgConditions)) {
            /* Jump from bootloader */
#if 0
#if defined(USE_BSWM)
            if( progConditions.ApplUpdated ) {
                BswM_Dcm_ApplicationUpdated();
            }
#endif
#endif
            GlobalProgConditions.ApplUpdated = FALSE;
            if( SID_DIAGNOSTIC_SESSION_CONTROL == GlobalProgConditions.Sid ) {
                if(E_OK == DslDspSilentlyStartProtocol(GlobalProgConditions.SubFncId, (uint16)GlobalProgConditions.ProtocolId, GlobalProgConditions.TesterSourceAdd, GlobalProgConditions.ResponseRequired) ) {
                    if( GlobalProgConditions.ResponseRequired ) {
                        /* Wait until full communication has been indicated, then send a response to service */
                        ProgConditionStartupResponseState = DCM_GENERAL_PENDING;
                    }
                } else {
                    /* Starting protocol failed */
                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                }
            }
        }
    }
}

void DspResetMainFunction(void)
{
    Std_ReturnType result = E_NOT_OK;
	if( DCM_DSP_RESET_PENDING == dspUdsEcuResetData.resetPending )
	{
        /* IMPROVEMENT: Should be a call to SchM */
        result = Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(dspUdsEcuResetData.resetType);

		switch( result )
		{
		case E_OK:
			dspUdsEcuResetData.resetPending = DCM_DSP_RESET_WAIT_TX_CONF;
			// Create positive response
			dspUdsEcuResetData.pduTxData->SduDataPtr[1] = dspUdsEcuResetData.resetType;
			dspUdsEcuResetData.pduTxData->SduLength = 2;
			DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
			break;
		case E_PENDING:
			dspUdsEcuResetData.resetPending = DCM_DSP_RESET_PENDING;
			break;
		case E_NOT_OK:
		default:
			dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
			DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
			break;
		}
	}
}

void DspMemoryMainFunction(void)
{
	Dcm_ReturnWriteMemoryType WriteRet;
	Dcm_ReturnReadMemoryType ReadRet;
	switch(dspMemoryState)
	{
		case DCM_MEMORY_UNUSED:
			break;
		case DCM_MEMORY_READ:
			ReadRet = Dcm_ReadMemory(DCM_PENDING,0,0,0,0);
			if(ReadRet == DCM_READ_OK)/*asynchronous writing is ok*/
			{
				DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
				dspMemoryState = DCM_MEMORY_UNUSED;
			}
			if(ReadRet == DCM_READ_FAILED)
			{
				DsdDspProcessingDone(DCM_E_GENERALPROGRAMMINGFAILURE);
				dspMemoryState = DCM_MEMORY_UNUSED;
			}
			break;
		case DCM_MEMORY_WRITE:
			WriteRet = Dcm_WriteMemory(DCM_PENDING,0,0,0,0);
			if(WriteRet == DCM_WRITE_OK)/*asynchronous writing is ok*/
			{
				DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
				dspMemoryState = DCM_MEMORY_UNUSED;
			}
			if(WriteRet == DCM_WRITE_FAILED)
			{
				DsdDspProcessingDone(DCM_E_GENERALPROGRAMMINGFAILURE);
				dspMemoryState = DCM_MEMORY_UNUSED;
			}
			break;

			default:
			break;
			
	}
}

void DspPeriodicDIDMainFunction()
{
    boolean sentResponseThisLoop = FALSE;
    uint8 pDidIndex = dspPDidRef.nextStartIndex;
    if( 0 != dspPDidRef.PDidNofUsed ) {
        dspPDidRef.nextStartIndex %= dspPDidRef.PDidNofUsed;
    }
    for(uint8 i = 0; i < dspPDidRef.PDidNofUsed; i++) {
        if(dspPDidRef.dspPDid[pDidIndex].PDidTxPeriod > dspPDidRef.dspPDid[pDidIndex].PDidTxCounter) {
            dspPDidRef.dspPDid[pDidIndex].PDidTxCounter++;
        }
        if( dspPDidRef.dspPDid[pDidIndex].PDidTxPeriod <= dspPDidRef.dspPDid[pDidIndex].PDidTxCounter ) {
            if( sentResponseThisLoop  == FALSE ) {
                if (E_OK == DslInternal_ResponseOnOneDataByPeriodicId(dspPDidRef.dspPDid[pDidIndex].PeriodicDid, dspPDidRef.dspPDid[pDidIndex].PDidRxPduID)){
                    dspPDidRef.dspPDid[pDidIndex].PDidTxCounter = 0;
                    /*AutoSar  DCM  8.10.5 */
                    sentResponseThisLoop = TRUE;
                    dspPDidRef.nextStartIndex = (pDidIndex + 1) % dspPDidRef.PDidNofUsed;
                }
            } else {
                /* Don't do anything - PDid will be sent next loop */
            }
        }
        pDidIndex++;
        pDidIndex %= dspPDidRef.PDidNofUsed;
    }
}

void DspReadDidMainFunction(void) {
    if( DCM_READ_DID_IDLE != dspUdsReadDidPending.state ) {
        DspUdsReadDataByIdentifier(dspUdsReadDidPending.pduRxData, dspUdsReadDidPending.pduTxData);
    }
#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
    if( DCM_WRITE_DID_PENDING == dspUdsWriteDidPending.state ) {
        DspUdsWriteDataByIdentifier(dspUdsWriteDidPending.pduRxData, dspUdsWriteDidPending.pduTxData);
    }
#endif
}
void DspRoutineControlMainFunction(void) {
    if( DCM_ROUTINE_CONTROL_PENDING == dspUdsRoutineControlPending.state ) {
        DspUdsRoutineControl(dspUdsRoutineControlPending.pduRxData, dspUdsRoutineControlPending.pduTxData);
    }
}
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
void DspJumpToBootMainFunction(void) {
    if( DCM_JTB_EXECUTE == dspUdsSessionControlData.jumpToBootState ) {
         DspUdsDiagnosticSessionControl(dspUdsSessionControlData.pduRxData, dspUdsSessionControlData.sessionPduId,dspUdsSessionControlData.pduTxData, FALSE, FALSE);
    }
}
#endif

void DspStartupServiceResponseMainFunction(void)
{
    if(DCM_GENERAL_PENDING == ProgConditionStartupResponseState) {
        Std_ReturnType reqRet = DslDspResponseOnStartupRequest(GlobalProgConditions.Sid, GlobalProgConditions.SubFncId, (uint16)GlobalProgConditions.ProtocolId, GlobalProgConditions.TesterSourceAdd);
        if(E_PENDING != reqRet) {
            if(E_OK != reqRet) {
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
            }
            ProgConditionStartupResponseState = DCM_GENERAL_IDLE;
        }
    }
}

void DspPreDsdMain(void) {
    /* Should be called before DsdMain so that an internal request
     * may be processed directly */
    DspPeriodicDIDMainFunction();

#if  defined(DCM_USE_SERVICE_RESPONSEONEVENT) && DCM_ROE_INTERNAL_DIDS == STD_ON
    DCM_ROE_PollDataIdentifiers();
#endif
    /* Should be done before DsdMain so that we can fulfill
     * DCM719 (mode switch DcmEcuReset to EXECUTE next main function) */
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
    DspJumpToBootMainFunction();
#endif

    DspStartupServiceResponseMainFunction();

}
void DspMain(void)
{
	DspResetMainFunction();
	DspMemoryMainFunction();
	DspReadDidMainFunction();
	DspRoutineControlMainFunction();

#if (DCM_SECURITY_EOL_INDEX != 0)

	for (uint8 i = 0; i < DCM_SECURITY_EOL_INDEX; i++)
	{
	    //Check if a wait is required before accepting a request
	   switch (dspUdsSecurityAccesData.secFalseAttemptChk[i].startDelayTimer) {

	       case DELAY_TIMER_ON_BOOT_ACTIVE:
	       case DELAY_TIMER_ON_EXCEEDING_LIMIT_ACTIVE:
	           TIMER_DECREMENT(dspUdsSecurityAccesData.secFalseAttemptChk[i].timerSecAcssAttempt);
	           if (dspUdsSecurityAccesData.secFalseAttemptChk[i].timerSecAcssAttempt < DCM_MAIN_FUNCTION_PERIOD_TIME_MS) {
	               dspUdsSecurityAccesData.secFalseAttemptChk[i].startDelayTimer = DELAY_TIMER_DEACTIVE;
	           }
	           break;

	       case DELAY_TIMER_DEACTIVE:
	       default:
	           break;
	   }
	}
#endif
}

void DspCancelPendingRequests(void)
{
    if( DCM_READ_DID_IDLE != dspUdsReadDidPending.state ) {
        /* DidRead was in pending state, cancel it */
        DspCancelPendingDid(dspUdsReadDidPending.pendingDid, dspUdsReadDidPending.pendingSignalIndex ,dspUdsReadDidPending.state, dspUdsReadDidPending.pduTxData);
    }
    dspMemoryState = DCM_MEMORY_UNUSED;
    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
    dspUdsReadDidPending.state = DCM_READ_DID_IDLE;
#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
    dspUdsWriteDidPending.state = DCM_WRITE_DID_IDLE;
#endif
    if( DCM_ROUTINE_CONTROL_IDLE != dspUdsRoutineControlPending.state ) {
        DspCancelPendingRoutine(dspUdsRoutineControlPending.pduRxData, dspUdsRoutineControlPending.pduTxData);
    }
    dspUdsRoutineControlPending.state = DCM_ROUTINE_CONTROL_IDLE;
}

boolean DspCheckSessionLevel(Dcm_DspSessionRowType const* const* sessionLevelRefTable)
{
	Std_ReturnType returnStatus;
	boolean levelFound = FALSE;
	Dcm_SesCtrlType currentSession;

	returnStatus = DslGetSesCtrlType(&currentSession);
	if (returnStatus == E_OK) {
	    if( (*sessionLevelRefTable)->Arc_EOL ) {
	        /* No session reference configured, no check should be done. */
	        levelFound = TRUE;
	    } else {
            while ( ((*sessionLevelRefTable)->DspSessionLevel != DCM_ALL_SESSION_LEVEL) && ((*sessionLevelRefTable)->DspSessionLevel != currentSession) && (!(*sessionLevelRefTable)->Arc_EOL) ) {
                sessionLevelRefTable++;
            }

            if (!(*sessionLevelRefTable)->Arc_EOL) {
                levelFound = TRUE;
            }
	    }
	}

	return levelFound;
}


boolean DspCheckSecurityLevel(Dcm_DspSecurityRowType const* const* securityLevelRefTable)
{
	Std_ReturnType returnStatus;
	boolean levelFound = FALSE;
	Dcm_SecLevelType currentSecurityLevel;

	returnStatus = DslGetSecurityLevel(&currentSecurityLevel);
	if (returnStatus == E_OK) {
	    if( (*securityLevelRefTable)->Arc_EOL ) {
            /* No security level reference configured, no check should be done. */
            levelFound = TRUE;
	    } else {
            while ( ((*securityLevelRefTable)->DspSecurityLevel != currentSecurityLevel) && (!(*securityLevelRefTable)->Arc_EOL) ) {
                securityLevelRefTable++;
            }
            if (!(*securityLevelRefTable)->Arc_EOL) {
                levelFound = TRUE;
            }
	    }
	}

	return levelFound;
}

/**
 * Checks if a session is supported
 * @param session
 * @return TRUE: Session supported, FALSE: Session not supported
 */
boolean DspDslCheckSessionSupported(uint8 session) {
    const Dcm_DspSessionRowType *sessionRow = Dcm_ConfigPtr->Dsp->DspSession->DspSessionRow;
    while ((sessionRow->DspSessionLevel != session) && (!sessionRow->Arc_EOL) ) {
        sessionRow++;
    }
    return (FALSE == sessionRow->Arc_EOL);
}

void DspUdsDiagnosticSessionControl(const PduInfoType *pduRxData, PduIdType txPduId, PduInfoType *pduTxData, boolean respPendOnTransToBoot, boolean internalStartupRequest)
{
    /** @req DCM250 */
    const Dcm_DspSessionRowType *sessionRow = Dcm_ConfigPtr->Dsp->DspSession->DspSessionRow;
    Dcm_SesCtrlType reqSessionType;
    Std_ReturnType result = E_OK;
    Dcm_ProtocolType activeProtocolID;
    if( DCM_JTB_IDLE == dspUdsSessionControlData.jumpToBootState ) {
        if (pduRxData->SduLength == 2) {
            reqSessionType = pduRxData->SduDataPtr[1];
            // Check if type exist in session table
            while ((sessionRow->DspSessionLevel != reqSessionType) && (!sessionRow->Arc_EOL) ) {
                sessionRow++;
            }
            if (!sessionRow->Arc_EOL) {
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
                if(!internalStartupRequest) {
                    switch(sessionRow->DspSessionForBoot) {
                        case DCM_OEM_BOOT:
                            /* @req DCM532 */
                            result = Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_JUMPTOBOOTLOADER);
                            break;
                        case DCM_SYS_BOOT:
                            /* @req DCM592 */
                            result = Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_JUMPTOSYSSUPPLIERBOOTLOADER);
                            break;
                        case DCM_NO_BOOT:
                            result = E_OK;
                            break;
                        default:
                            result = E_NOT_OK;
                            break;
                    }
                }
#endif
                if (result == E_OK) {
                    dspUdsSessionControlData.sessionPending = TRUE;
                    dspUdsSessionControlData.session = reqSessionType;
                    dspUdsSessionControlData.sessionPduId = txPduId;
                    dspUdsSessionControlData.pduRxData = pduRxData;
                    dspUdsSessionControlData.pduTxData = pduTxData;
                    Std_ReturnType activeProtocolStatus = DslGetActiveProtocol(&activeProtocolID);
                    if( (DCM_NO_BOOT == sessionRow->DspSessionForBoot) || internalStartupRequest) {
                        pduTxData->SduDataPtr[1] = reqSessionType;
                        if( E_OK == activeProtocolStatus ) {
                            // Create positive response
                            if( DCM_UDS_ON_CAN == activeProtocolID ) {
                                pduTxData->SduDataPtr[2] = sessionRow->DspSessionP2ServerMax >> 8;
                                pduTxData->SduDataPtr[3] = sessionRow->DspSessionP2ServerMax;
                                uint16 p2ServerStarMax10ms = sessionRow->DspSessionP2StarServerMax / 10;
                                pduTxData->SduDataPtr[4] = p2ServerStarMax10ms >> 8;
                                pduTxData->SduDataPtr[5] = p2ServerStarMax10ms;
                                pduTxData->SduLength = 6;
                            } else {
                                pduTxData->SduLength = 2;
                            }
                        } else {
                            pduTxData->SduLength = 2;
                        }
                        DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
                    } else {
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
                        GlobalProgConditions.ReprogramingRequest = (DCM_PROGRAMMING_SESSION == reqSessionType);
                        GlobalProgConditions.ResponseRequired = DsdDspGetResponseRequired();
                        GlobalProgConditions.Sid = SID_DIAGNOSTIC_SESSION_CONTROL;
                        GlobalProgConditions.SubFncId = reqSessionType;
                        if( E_OK == activeProtocolStatus ) {
                            GlobalProgConditions.ProtocolId = activeProtocolID;
                        }
                        uint16 srcAddr = DsdDspGetTesterSourceAddress();
                        GlobalProgConditions.TesterSourceAdd = (uint8)(srcAddr & 0xFF);
                        if( respPendOnTransToBoot ) {
                            /* Force response pending next main function and
                             * wait for tx confirmation*/
                            dspUdsSessionControlData.jumpToBootState = DCM_JTB_WAIT_RESPONSE_PENDING_TX_CONFIRM;
                            /* @req DCM654 */
                            DsdDspForceResponsePending();
                        } else {
                            /* Trigger mode switch next main function */
                            /* @req DCM719 */
                            /* @req DCM720 */
                            /* IMPROVEMENT: Add support for pending */
                            if(E_OK == Dcm_SetProgConditions(&GlobalProgConditions)) {
                                dspUdsSessionControlData.jumpToBootState = DCM_JTB_EXECUTE;
                            } else {
                                /* @req DCM715 */
                                DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
                            }
                        }
                        if( (E_OK != activeProtocolStatus) || (0 != (srcAddr & (uint16)~0xFF)) ) {
                            /* Failed to get the protocol id or the source address was to large */
                            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
                        }
#else
                        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
                        DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
#endif
                    }
                } else {
                    // IMPROVEMENT: Add handling of special case of E_FORCE_RCRRP
                    DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
                }
            } else {
                DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED);	/** @req DCM307 */
            }
        } else {
            // Wrong length
            DsdDspProcessingDone(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
        }
    }
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
    else if(DCM_JTB_EXECUTE == dspUdsSessionControlData.jumpToBootState) {
        if(E_OK != Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_EXECUTE)) {
            DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
        }
        dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
    }
#else
    (void)respPendOnTransToBoot;
#endif
}


void DspUdsEcuReset(const Dcm_DsdServiceType *sidConfPtr, const PduInfoType *pduRxData, PduIdType txPduId, PduInfoType *pduTxData)
{
    /** @req DCM260 */
    uint8 reqResetType;

    if (pduRxData->SduLength == 2) {
        reqResetType = pduRxData->SduDataPtr[1];
        const Dcm_DsdSubServiceType *subService = NULL;
        Dcm_NegativeResponseCodeType respCode = DCM_E_POSITIVERESPONSE;
        /** @req DCM373 */
        if( DsdLookupSubService(sidConfPtr, reqResetType, &subService, &respCode) && (NULL != subService) ) {
            if( DCM_E_POSITIVERESPONSE == respCode ) {
                Std_ReturnType result = E_NOT_OK;
                /* @req DCM373 */
                /* IMPROVEMENT: Should be a call to SchM */
                if( (DCM_DISABLE_RAPID_POWER_SHUTDOWN != reqResetType) && (DCM_ENABLE_RAPID_POWER_SHUTDOWN != reqResetType) ) {
                    result = Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(reqResetType);
                }
#if 0
                else {
                    result = Rte_Switch_Dcm_dcm_DcmRapidPowerShutDown_DcmRapidPowerShutDown(reqResetType);
                }
#endif

                dspUdsEcuResetData.resetPduId = txPduId;
                dspUdsEcuResetData.pduTxData = pduTxData;
                dspUdsEcuResetData.resetType = reqResetType;

                switch( result )
                {
                case E_OK:
                    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_WAIT_TX_CONF;
                    // Create positive response
                    pduTxData->SduDataPtr[1] = reqResetType;
                    pduTxData->SduLength = 2;
                    DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
                    break;
                case E_PENDING:
                    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_PENDING;
                    break;
                case E_NOT_OK:
                default:
                    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
                    DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
                    break;
                }
            } else {
                DsdDspProcessingDone(respCode);
            }
        } else {
            DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED);
        }
    } else {
        // Wrong length
        DsdDspProcessingDone(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
    }
}

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_CLEARDIAGNOSTICINFORMATION)
void DspUdsClearDiagnosticInformation(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM247 */
	uint32 dtc;
	Dem_ReturnClearDTCType result;

	if (pduRxData->SduLength == 4) {
		dtc = BYTES_TO_DTC(pduRxData->SduDataPtr[1], pduRxData->SduDataPtr[2], pduRxData->SduDataPtr[3]);

		result = Dem_ClearDTC(dtc, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY); /** @req DCM005 */

		switch (result)
		{
		case DEM_CLEAR_OK:
			// Create positive response
			pduTxData->SduLength = 1;
			DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
			break;

		default:
			DsdDspProcessingDone(DCM_E_REQUESTOUTOFRANGE);
			break;
		}
	}
	else {
		// Wrong length
		DsdDspProcessingDone(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
	}
}
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_READDTCINFORMATION)
static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x01_0x07_0x11_0x12(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	typedef struct {
		uint8		SID;
		uint8		reportType;
		uint8 		dtcStatusAvailabilityMask;
		uint8		dtcFormatIdentifier;
		uint8		dtcCountHighByte;
		uint8		dtcCountLowByte;
	} TxDataType;

	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	Dem_ReturnSetFilterType setDtcFilterResult;

	// Setup the DTC filter
	switch (pduRxData->SduDataPtr[1]) 	/** @req DCM293 */
	{
	case 0x01:	// reportNumberOfDTCByStatusMask
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[2], DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x07:	// reportNumberOfDTCBySeverityMaskRecord
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[3], DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_YES, pduRxData->SduDataPtr[2], DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x11:	// reportNumberOfMirrorMemoryDTCByStatusMask
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[2], DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_MIRROR_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x12:	// reportNumberOfEmissionRelatedOBDDTCByStatusMask
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[2], DEM_DTC_KIND_EMISSION_REL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	default:
		setDtcFilterResult = DEM_WRONG_FILTER;
		break;
	}

	if (setDtcFilterResult == DEM_FILTER_ACCEPTED) {
		Std_ReturnType result;
		Dem_ReturnGetNumberOfFilteredDTCType getNumerResult;
		uint16 numberOfFilteredDtc;
		uint8 dtcStatusMask;
		TxDataType *txData = (TxDataType*)pduTxData->SduDataPtr;

		/** @req DCM376 */
		getNumerResult = Dem_GetNumberOfFilteredDtc(&numberOfFilteredDtc);
		if (getNumerResult == DEM_NUMBER_OK) {
			result = Dem_GetDTCStatusAvailabilityMask(&dtcStatusMask);
			if (result != E_OK) {
				dtcStatusMask = 0;
			}

			// Create positive response (ISO 14229-1 table 251)
			txData->reportType = pduRxData->SduDataPtr[1];						// reportType
			txData->dtcStatusAvailabilityMask = dtcStatusMask;					// DTCStatusAvailabilityMask
			txData->dtcFormatIdentifier = Dem_GetTranslationType();				// DTCFormatIdentifier
			txData->dtcCountHighByte = (numberOfFilteredDtc >> 8);				// DTCCount high byte
			txData->dtcCountLowByte = (numberOfFilteredDtc & 0xFFu);			// DTCCount low byte
			pduTxData->SduLength = 6;
		} else {
			// NOTE: What to do?
			responseCode = DCM_E_GENERALREJECT;
		}
	}
	else {
		responseCode = DCM_E_REQUESTOUTOFRANGE;
	}

	return responseCode;
}


static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x02_0x0A_0x0F_0x13_0x15(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	Dem_ReturnSetFilterType setDtcFilterResult;

	typedef struct {
		uint8		dtcHighByte;
		uint8		dtcMiddleByte;
		uint8		dtcLowByte;
		uint8		statusOfDtc;
	} dtcAndStatusRecordType;

	typedef struct {
		uint8					SID;
		uint8					reportType;
		uint8 					dtcStatusAvailabilityMask;
		dtcAndStatusRecordType	dtcAndStatusRecord[];
	} TxDataType;

	// Setup the DTC filter
	switch (pduRxData->SduDataPtr[1]) 	/** @req DCM378 */
	{
	case 0x02:	// reportDTCByStatusMask
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[2], DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x0A:	// reportSupportedDTC
		setDtcFilterResult = Dem_SetDTCFilter(DEM_DTC_STATUS_MASK_ALL, DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x0F:	// reportMirrorMemoryDTCByStatusMask
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[2], DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_MIRROR_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x13:	// reportEmissionRelatedOBDDTCByStatusMask
		setDtcFilterResult = Dem_SetDTCFilter(pduRxData->SduDataPtr[2], DEM_DTC_KIND_EMISSION_REL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	case 0x15:	// reportDTCWithPermanentStatus
		setDtcFilterResult = Dem_SetDTCFilter(DEM_DTC_STATUS_MASK_ALL, DEM_DTC_KIND_ALL_DTCS, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PERMANENT_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);
		break;

	default:
		setDtcFilterResult = DEM_WRONG_FILTER;
		break;
	}

	if (setDtcFilterResult == DEM_FILTER_ACCEPTED) {
		uint8 dtcStatusMask;
		TxDataType *txData = (TxDataType*)pduTxData->SduDataPtr;
		Dem_ReturnGetNextFilteredDTCType getNextFilteredDtcResult;
		uint32 dtc;
		Dem_EventStatusExtendedType dtcStatus;
		uint16 nrOfDtcs = 0;
		Std_ReturnType result;

		/** @req DCM377 */
		result = Dem_GetDTCStatusAvailabilityMask(&dtcStatusMask);
		if (result != E_OK) {
			dtcStatusMask = 0;
		}

		// Create positive response (ISO 14229-1 table 252)
		txData->reportType = pduRxData->SduDataPtr[1];
		txData->dtcStatusAvailabilityMask = dtcStatusMask;

		if (dtcStatusMask != 0x00) {	/** @req DCM008 */
			getNextFilteredDtcResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
			while (getNextFilteredDtcResult == DEM_FILTERED_OK) {
				txData->dtcAndStatusRecord[nrOfDtcs].dtcHighByte = DTC_HIGH_BYTE(dtc);
				txData->dtcAndStatusRecord[nrOfDtcs].dtcMiddleByte = DTC_MID_BYTE(dtc);
				txData->dtcAndStatusRecord[nrOfDtcs].dtcLowByte = DTC_LOW_BYTE(dtc);
				txData->dtcAndStatusRecord[nrOfDtcs].statusOfDtc = dtcStatus;
				nrOfDtcs++;
				getNextFilteredDtcResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
			}

			if (getNextFilteredDtcResult != DEM_FILTERED_NO_MATCHING_DTC) {
				responseCode = DCM_E_REQUESTOUTOFRANGE;
			}
		}
		pduTxData->SduLength = (PduLengthType)(3 + (nrOfDtcs * sizeof(dtcAndStatusRecordType)));
	}
	else {
		responseCode = DCM_E_REQUESTOUTOFRANGE;
	}

	return responseCode;
}

static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x08(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	(void)pduTxData;
	//lint +estring(920,pointer)  /* cast to void */
	// IMPROVEMENT: Not supported yet, (DEM module does not currently support severity).
	responseCode = DCM_E_REQUESTOUTOFRANGE;

	return responseCode;
}


static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x09(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	(void)pduTxData;
	//lint +estring(920,pointer)  /* cast to void */
	// IMPROVEMENT: Not supported yet, (DEM module does not currently support severity).
	responseCode = DCM_E_REQUESTOUTOFRANGE;

	return responseCode;
}


static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x06_0x10(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    Dem_DTCOriginType dtcOrigin;
    uint8 startRecNum;
    uint8 endRecNum;

    // Switch on sub function
    switch (pduRxData->SduDataPtr[1]) 	/** @req DCM378 */
    {
    case 0x06:	// reportDTCExtendedDataRecordByDTCNumber
        dtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
        break;

    case 0x10:	// reportMirrorMemoryDTCExtendedDataRecordByDTCNumber
        dtcOrigin = DEM_DTC_ORIGIN_MIRROR_MEMORY;
        break;

    default:
        responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
        dtcOrigin = 0;
        break;
    }

    // Switch on record number
    switch (pduRxData->SduDataPtr[5])
    {
    case 0xFF:	// Report all Extended Data Records for a particular DTC
        startRecNum = 0x00;
        endRecNum = 0xEF;
        break;

    case 0xFE:	// Report all OBD Extended Data Records for a particular DTC
        startRecNum = 0x90;
        endRecNum = 0xEF;
        break;

    default:	// Report one specific Extended Data Records for a particular DTC
        startRecNum = pduRxData->SduDataPtr[5];
        endRecNum = startRecNum;
        break;
    }

    if (responseCode == DCM_E_POSITIVERESPONSE) {
        Dem_ReturnGetStatusOfDTCType getStatusOfDtcResult;
        uint32 dtc;
        Dem_EventStatusExtendedType statusOfDtc;

        dtc = BYTES_TO_DTC(pduRxData->SduDataPtr[2], pduRxData->SduDataPtr[3], pduRxData->SduDataPtr[4]);
        getStatusOfDtcResult = Dem_GetStatusOfDTC(dtc, dtcOrigin, &statusOfDtc); /** @req DCM295 */ /** @req DCM475 */
        if (getStatusOfDtcResult == DEM_STATUS_OK) {
            Dem_ReturnGetExtendedDataRecordByDTCType getExtendedDataRecordByDtcResult;
            uint8 recNum;
            uint16 recLength;
            uint16 txIndex = 6;
            boolean foundValidRecordNumber = FALSE;

            /** @req DCM297 */ /** @req DCM474 */ /** @req DCM386 */
            pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];			// Sub function
            pduTxData->SduDataPtr[2] = DTC_HIGH_BYTE(dtc);					// DTC high byte
            pduTxData->SduDataPtr[3] = DTC_MID_BYTE(dtc);					// DTC mid byte
            pduTxData->SduDataPtr[4] = DTC_LOW_BYTE(dtc);					// DTC low byte
            pduTxData->SduDataPtr[5] = statusOfDtc;							// DTC status
            for (recNum = startRecNum; recNum <= endRecNum; recNum++) {
                recLength = pduTxData->SduLength - (txIndex + 1);	// Calculate what's left in buffer
                /** @req DCM296 */ /** @req DCM476 */ /** @req DCM382 */
                getExtendedDataRecordByDtcResult = Dem_GetExtendedDataRecordByDTC(dtc, dtcOrigin, recNum, &pduTxData->SduDataPtr[txIndex+1], &recLength);
                if (getExtendedDataRecordByDtcResult == DEM_RECORD_OK) {
                    foundValidRecordNumber = TRUE;
                    if (recLength > 0) {
                        pduTxData->SduDataPtr[txIndex++] = recNum;
                        /* Instead of calling Dem_GetSizeOfExtendedDataRecordByDTC() the result from Dem_GetExtendedDataRecordByDTC() is used */
                        /** @req DCM478 */ /** @req DCM479 */ /** @req DCM480 */
                        txIndex += recLength;
                    }
                }
            }

            pduTxData->SduLength = txIndex;

            if (!foundValidRecordNumber) {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        }
        else {
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        }
    }

    return responseCode;
}

static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x03(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

	uint16 numFilteredRecords = 0;
	uint32 dtc = 0;
	uint8 recordNumber = 0;
	uint16 nofBytesCopied = 0;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	//lint +estring(920,pointer)  /* cast to void */
	/* @req DCM298 */
	if( (DEM_FILTER_ACCEPTED == Dem_SetFreezeFrameRecordFilter(DEM_DTC_FORMAT_UDS, &numFilteredRecords)) &&
	        ( (SID_LEN + SF_LEN + (DTC_LEN + FF_REC_NUM_LEN)*numFilteredRecords) <= pduTxData->SduLength )) {
	    for( uint16 i = 0; (i < numFilteredRecords) && (DCM_E_POSITIVERESPONSE == responseCode); i++ ) {
	        /* @req DCM299 */
	        if( DEM_FILTERED_OK == Dem_GetNextFilteredRecord(&dtc, &recordNumber) ) {
	            /* @req DCM300 */
	            pduTxData->SduDataPtr[SID_LEN + SF_LEN + nofBytesCopied++] = DTC_HIGH_BYTE(dtc);
	            pduTxData->SduDataPtr[SID_LEN + SF_LEN + nofBytesCopied++] = DTC_MID_BYTE(dtc);
	            pduTxData->SduDataPtr[SID_LEN + SF_LEN + nofBytesCopied++] = DTC_LOW_BYTE(dtc);
	            pduTxData->SduDataPtr[SID_LEN + SF_LEN + nofBytesCopied++] = recordNumber;
	        } else {
	            responseCode = DCM_E_REQUESTOUTOFRANGE;
	        }
	    }
	} else {
	    responseCode = DCM_E_REQUESTOUTOFRANGE;
	}

    pduTxData->SduDataPtr[0] = 0x59;    // positive response
    pduTxData->SduDataPtr[1] = 0x03;    // subid
    pduTxData->SduLength = SID_LEN + SF_LEN + nofBytesCopied;

	return responseCode;
}

static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x04(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	// 1. Only consider Negative Response 0x10

	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	uint32 DtcNumber = 0;
	uint8 RecordNumber = 0;
	uint16 SizeOfTxBuf = pduTxData->SduLength;
	Dem_ReturnGetFreezeFrameDataByDTCType GetFFbyDtcReturnCode = DEM_GET_FFDATABYDTC_OK;
	Dem_ReturnGetStatusOfDTCType GetStatusOfDtc = DEM_STATUS_OK;
	Dem_EventStatusExtendedType DtcStatus = 0;

	// Now let's assume DTC has 3 bytes.
	DtcNumber = (((uint32)pduRxData->SduDataPtr[2])<<16) +
				(((uint32)pduRxData->SduDataPtr[3])<<8) +
				((uint32)pduRxData->SduDataPtr[4]);

    GetStatusOfDtc = Dem_GetStatusOfDTC(DtcNumber, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &DtcStatus); /** @req DCM383 */
    switch (GetStatusOfDtc) {
        case DEM_STATUS_OK:
            break;
        default:
            return DCM_E_REQUESTOUTOFRANGE;
    }

	RecordNumber = pduRxData->SduDataPtr[5];

	if( 0xFF == RecordNumber ) {
		/* Request for all freeze frames */
		GetFFbyDtcReturnCode = DEM_GET_FFDATABYDTC_WRONG_DTC;
		uint16 nofBytesCopied = 0;
		uint16 bufSizeLeft = 0;
		Dem_ReturnGetFreezeFrameDataByDTCType ret = DEM_GET_FFDATABYDTC_OK;
		for(uint8 record = 0; record < RecordNumber; record++) { /* @req DCM385 */
			bufSizeLeft = pduTxData->SduLength - 6 - nofBytesCopied;
			ret = Dem_GetFreezeFrameDataByDTC(DtcNumber, DEM_DTC_ORIGIN_PRIMARY_MEMORY,
					record, &pduTxData->SduDataPtr[6 + nofBytesCopied], &bufSizeLeft);
			if( DEM_GET_FFDATABYDTC_OK == ret ) {
				nofBytesCopied += bufSizeLeft;
				/* At least one OK! */
				GetFFbyDtcReturnCode = DEM_GET_FFDATABYDTC_OK;
			}
		}
		SizeOfTxBuf = nofBytesCopied;
	} else {
		GetFFbyDtcReturnCode = Dem_GetFreezeFrameDataByDTC(DtcNumber, DEM_DTC_ORIGIN_PRIMARY_MEMORY,
			RecordNumber, &pduTxData->SduDataPtr[6], &SizeOfTxBuf);
	}
	// Negative response
	switch (GetFFbyDtcReturnCode) {
		case DEM_GET_FFDATABYDTC_OK:
			pduTxData->SduLength = SizeOfTxBuf + 6;
			break;
		default:
			return DCM_E_REQUESTOUTOFRANGE;
	}

	// Positive response
	// See ISO 14229(2006) Table 254
	pduTxData->SduDataPtr[0] = 0x59;	// positive response
	pduTxData->SduDataPtr[1] = 0x04;	// subid
	pduTxData->SduDataPtr[2] = pduRxData->SduDataPtr[2];	// DTC
	pduTxData->SduDataPtr[3] = pduRxData->SduDataPtr[3];
	pduTxData->SduDataPtr[4] = pduRxData->SduDataPtr[4];
	pduTxData->SduDataPtr[5] = (uint8)DtcStatus;	//status
	return responseCode;
}

static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x05(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	(void)pduTxData;
	//lint +estring(920,pointer)  /* cast to void */
	// IMPROVEMENT: Add support
	responseCode = DCM_E_REQUESTOUTOFRANGE;

	return responseCode;
}

static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x0B_0x0C_0x0D_0x0E(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	(void)pduTxData;
	//lint +estring(920,pointer)  /* cast to void */
	// IMPROVEMENT: Add support
	responseCode = DCM_E_REQUESTOUTOFRANGE;

	return responseCode;
}

static Dcm_NegativeResponseCodeType udsReadDtcInfoSub_0x14(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData; /* Avoid compiler warning */
	(void)pduTxData; /* Avoid compiler warning */
	//lint +estring(920,pointer)  /* cast to void */
	// IMPROVEMENT: Add support
	/** !464 */
	responseCode = DCM_E_REQUESTOUTOFRANGE;

	return responseCode;
}


void DspUdsReadDtcInformation(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
    /** @req DCM248 */
    // Sub function number         0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15
    const uint8 sduLength[0x16] = {0, 3, 3, 2, 6, 3, 6, 4, 4, 5, 2, 2, 2, 2, 2, 3, 6, 3, 3, 3, 2, 2};
    /* Sub function number                 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 */
    const boolean subFncSupported[0x16] = {0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,};

    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

    uint8 subFunctionNumber = pduRxData->SduDataPtr[1];

    // Check length
    if (subFunctionNumber <= 0x15) {
        if(subFncSupported[subFunctionNumber]) {
            if (pduRxData->SduLength == sduLength[subFunctionNumber]) {
                switch (subFunctionNumber)
                {
                case 0x01:	// reportNumberOfDTCByStatusMask
                case 0x07:	// reportNumberOfDTCBySeverityMaskRecord
                case 0x11:	// reportNumberOfMirrorMemoryDTCByStatusMask
                case 0x12:	// reportNumberOfEmissionRelatedOBDDTCByStatusMask
                    responseCode = udsReadDtcInfoSub_0x01_0x07_0x11_0x12(pduRxData, pduTxData);
                    break;

                case 0x02:	// reportDTCByStatusMask
                case 0x0A:	// reportSupportedDTC
                case 0x0F:	// reportMirrorMemoryDTCByStatusMask
                case 0x13:	// reportEmissionRelatedOBDDTCByStatusMask
                case 0x15:	// reportDTCWithPermanentStatus
                    responseCode = udsReadDtcInfoSub_0x02_0x0A_0x0F_0x13_0x15(pduRxData, pduTxData);
                    break;

                case 0x08:	// reportDTCBySeverityMaskRecord
                    responseCode = udsReadDtcInfoSub_0x08(pduRxData, pduTxData);
                    break;

                case 0x09:	// reportSeverityInformationOfDTC
                    responseCode = udsReadDtcInfoSub_0x09(pduRxData, pduTxData);
                    break;

                case 0x06:	// reportDTCExtendedDataRecordByDTCNumber
                case 0x10:	// reportMirrorMemoryDTCExtendedDataRecordByDTCNumber
                    responseCode = udsReadDtcInfoSub_0x06_0x10(pduRxData, pduTxData);
                    break;

                case 0x03:	// reportDTCSnapshotIdentidication
                    responseCode = udsReadDtcInfoSub_0x03(pduRxData, pduTxData);
                    break;

                case 0x04:	// reportDTCSnapshotByDtcNumber
                    responseCode = udsReadDtcInfoSub_0x04(pduRxData, pduTxData);
                    break;

                case 0x05:	// reportDTCSnapshotRecordNumber
                    responseCode = udsReadDtcInfoSub_0x05(pduRxData, pduTxData);
                    break;

                case 0x0B:	// reportFirstTestFailedDTC
                case 0x0C:	// reportFirstConfirmedDTC
                case 0x0D:	// reportMostRecentTestFailedDTC
                case 0x0E:	// reportMostRecentConfirmedDTC
                    responseCode = udsReadDtcInfoSub_0x0B_0x0C_0x0D_0x0E(pduRxData, pduTxData);
                    break;

                case 0x14:	// reportDTCFaultDetectionCounter
                    responseCode = udsReadDtcInfoSub_0x14(pduRxData, pduTxData);
                    break;

                default:
                    // Unknown sub function
                    responseCode = DCM_E_REQUESTOUTOFRANGE;
                    break;
                }
            }
            else {
                // Wrong length
                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
            }
        } else {
            /* Subfunction not supported */
            responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
        }
    }
    else {
        /* Subfunction not supported */
        responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
    }

    DsdDspProcessingDone(responseCode);
}
#endif

/**
**		This Function for check the pointer of Dynamically Did Sourced by Did buffer using a didNr
**/
#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static boolean LookupDDD(uint16 didNr,  const Dcm_DspDDDType **DDid )	
{
	uint8 i;
	boolean ret = FALSE;
	const Dcm_DspDDDType* DDidptr = &dspDDD[0];
	

	if (didNr < DCM_PERODICDID_HIGH_MASK) {
		return ret;
	}


	for(i = 0;((i < DCM_MAX_DDD_NUMBER) && (ret == FALSE)); i++)
	{
		if(DDidptr->DynamicallyDid == didNr)
		{
			ret = TRUE;
		
		}
		else
		{
			DDidptr++;
		}
	}
	if(ret == TRUE)
	{
		*DDid = DDidptr;
	}

	return ret;
}
#endif


static void DspCancelPendingDid(uint16 didNr, uint16 signalIndex, ReadDidPendingStateType pendingState, PduInfoType *pduTxData )
{
    const Dcm_DspDidType *didPtr = NULL;
    if( lookupNonDynamicDid(didNr, &didPtr) ) {
        if( signalIndex < didPtr->DspNofSignals ) {
            const Dcm_DspDataType *dataPtr = didPtr->DspSignalRef[signalIndex].DspSignalDataRef;
            if( DCM_READ_DID_PENDING_COND_CHECK == pendingState ) {
                if( dataPtr->DspDataConditionCheckReadFnc != NULL ) {
                    (void)dataPtr->DspDataConditionCheckReadFnc (DCM_CANCEL, pduTxData->SduDataPtr);
                }
            } else if( DCM_READ_DID_PENDING_READ_DATA == pendingState ) {
                if( dataPtr->DspDataReadDataFnc.AsynchDataReadFnc != NULL ) {
                    if( DATA_PORT_ASYNCH == dataPtr->DspDataUsePort ) {
                        (void)dataPtr->DspDataReadDataFnc.AsynchDataReadFnc(DCM_CANCEL, pduTxData->SduDataPtr);
                    }
                }
            } else {
                /* Not in a pending state */
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
            }
        } else {
            /* Invalid signal index */
            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
        }
    }
}


/**
**		This Function for read Dynamically Did data buffer Sourced by Memory address using a didNr
**/
#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static Dcm_NegativeResponseCodeType readDDDData(Dcm_DspDDDType *DDidPtr, uint8 *Data, uint16 *Length)
{
    uint8 i;
    uint8 dataCount;
    uint16 SourceDataLength = 0;
    const Dcm_DspDidType *SourceDidPtr = NULL;
    const Dcm_DspSignalType *signalPtr;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    *Length = 0;
    uint8* nextDataSlot = Data;

    for(i = 0;(i < DCM_MAX_DDDSOURCE_NUMBER) && (DDidPtr->DDDSource[i].formatOrPosition != 0)
        &&(responseCode == DCM_E_POSITIVERESPONSE);i++)
    {
        if(DDidPtr->DDDSource[i].DDDTpyeID == DCM_DDD_SOURCE_ADDRESS) {
            responseCode = checkAddressRange(DCM_READ_MEMORY, DDidPtr->DDDSource[i].memoryIdentifier, DDidPtr->DDDSource[i].SourceAddressOrDid, DDidPtr->DDDSource[i].Size);
            if( responseCode == DCM_E_POSITIVERESPONSE ) {
                (void)Dcm_ReadMemory(DCM_INITIAL,DDidPtr->DDDSource[i].memoryIdentifier,
                                        DDidPtr->DDDSource[i].SourceAddressOrDid,
                                        DDidPtr->DDDSource[i].Size,
                                        nextDataSlot);
                nextDataSlot += DDidPtr->DDDSource[i].Size;
                *Length += DDidPtr->DDDSource[i].Size;
            }
        }
        else if(DDidPtr->DDDSource[i].DDDTpyeID == DCM_DDD_SOURCE_DID) {
            
            if(lookupNonDynamicDid(DDidPtr->DDDSource[i].SourceAddressOrDid,&SourceDidPtr) && (NULL != SourceDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead) ) {
                if(DspCheckSecurityLevel(SourceDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSecurityLevelRef) != TRUE) {
                    responseCode = DCM_E_SECURITYACCESSDENIED;
                } else {
                    for( uint16 signal = 0; signal < SourceDidPtr->DspNofSignals; signal++ ) {
                        uint8 *didDataStart = nextDataSlot;
                        signalPtr = &SourceDidPtr->DspSignalRef[signal];
                        if( signalPtr->DspSignalDataRef->DspDataInfoRef->DspDidFixedLength ) {
                            SourceDataLength = signalPtr->DspSignalDataRef->DspDataSize;
                        } else if( NULL != signalPtr->DspSignalDataRef->DspDataReadDataLengthFnc ) {
                            (void)signalPtr->DspSignalDataRef->DspDataReadDataLengthFnc(&SourceDataLength);
                        }
                        if( (NULL != signalPtr->DspSignalDataRef->DspDataReadDataFnc.SynchDataReadFnc) && (SourceDataLength != 0) && (DCM_E_POSITIVERESPONSE == responseCode) ) {
                            if((DATA_PORT_SYNCH == signalPtr->DspSignalDataRef->DspDataUsePort) || (DATA_PORT_ECU_SIGNAL == signalPtr->DspSignalDataRef->DspDataUsePort)) {
                                (void)signalPtr->DspSignalDataRef->DspDataReadDataFnc.SynchDataReadFnc(didDataStart + signalPtr->DspSignalsPosition);
                            } else if(DATA_PORT_ASYNCH == signalPtr->DspSignalDataRef->DspDataUsePort) {
                                (void)signalPtr->DspSignalDataRef->DspDataReadDataFnc.AsynchDataReadFnc(DCM_INITIAL, didDataStart + signalPtr->DspSignalsPosition);
                            }

                        } else {
                            responseCode = DCM_E_REQUESTOUTOFRANGE;
                        }
                    }
                    if( DCM_E_POSITIVERESPONSE == responseCode ) {
                        for(dataCount = 0; dataCount < DDidPtr->DDDSource[i].Size; dataCount++) {
                            /* Shifting the data left by position (position 1 means index 0) */
                            nextDataSlot[dataCount] = nextDataSlot[dataCount + DDidPtr->DDDSource[i].formatOrPosition - 1];
                        }
                        nextDataSlot += DDidPtr->DDDSource[i].Size;
                        *Length += DDidPtr->DDDSource[i].Size;
                    }
                }
            }
            else
            {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        }
        else
        {
            
            responseCode = DCM_E_REQUESTOUTOFRANGE;	
        }
    }
    return responseCode;
}
#endif

#ifdef DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER
static Dcm_NegativeResponseCodeType checkDDDConditions(Dcm_DspDDDType *DDidPtr, uint16 *Length)
{
    const Dcm_DspDidType *SourceDidPtr = NULL;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    *Length = 0;

    for(uint8 i = 0;(i < DCM_MAX_DDDSOURCE_NUMBER) && (DDidPtr->DDDSource[i].formatOrPosition != 0)
        &&(responseCode == DCM_E_POSITIVERESPONSE);i++)
    {
        *Length += DDidPtr->DDDSource[i].Size;
        if(DDidPtr->DDDSource[i].DDDTpyeID == DCM_DDD_SOURCE_ADDRESS) {
            responseCode = checkAddressRange(DCM_READ_MEMORY, DDidPtr->DDDSource[i].memoryIdentifier, DDidPtr->DDDSource[i].SourceAddressOrDid, DDidPtr->DDDSource[i].Size);
        } else if(DDidPtr->DDDSource[i].DDDTpyeID == DCM_DDD_SOURCE_DID) {
            if( lookupNonDynamicDid(DDidPtr->DDDSource[i].SourceAddressOrDid,&SourceDidPtr) && (NULL != SourceDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead) ) {
                if( DspCheckSessionLevel(SourceDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSessionRef) ) {
                    if( !DspCheckSecurityLevel(SourceDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSecurityLevelRef) ) {
                        responseCode = DCM_E_SECURITYACCESSDENIED;
                    }
                } else {
                    responseCode = DCM_E_REQUESTOUTOFRANGE;
                }
            } else {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        } else {
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        }
    }
    return responseCode;
}
#endif

void DspUdsReadDataByIdentifier(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM253 */
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	uint16 nrOfDids;
	uint16 didNr;
	const Dcm_DspDidType *didPtr = NULL;
	Dcm_DspDDDType *DDidPtr=NULL;
	uint16 txPos = 1;
	uint16 i;
	uint16 Length;
	boolean noRequestedDidSupported = TRUE;
	ReadDidPendingStateType pendingState = DCM_READ_DID_IDLE;
	uint16 nofDidsRead = 0;
	uint16 reqDidStartIndex = 0;
	uint16 nofDidsReadInPendingReq = 0;
	uint16 pendingDid = 0;
	uint16 pendingDataLen = 0;
	uint16 pendingSigIndex = 0;
	uint16 pendingDataStartPos = 0;

	if ( (((pduRxData->SduLength - 1) % 2) == 0) && ( 0 != (pduRxData->SduLength - 1))) {
		nrOfDids = (pduRxData->SduLength - 1) / 2;
	    if( DCM_READ_DID_IDLE != dspUdsReadDidPending.state ) {
	        pendingState = dspUdsReadDidPending.state;
	        txPos = dspUdsReadDidPending.txWritePos;
	        nofDidsReadInPendingReq = dspUdsReadDidPending.nofReadDids;
	        reqDidStartIndex = dspUdsReadDidPending.reqDidIndex;
	        pendingDataLen = dspUdsReadDidPending.pendingDataLength;
	        pendingSigIndex = dspUdsReadDidPending.pendingSignalIndex;
	        pendingDataStartPos = dspUdsReadDidPending.pendingDataStartPos;
	    }
		/* IMPROVEMENT: Check security level and session for all dids before trying to read data */
	    for (i = reqDidStartIndex; (i < nrOfDids) && (responseCode == DCM_E_POSITIVERESPONSE); i++) {
			didNr = (uint16)((uint16)pduRxData->SduDataPtr[1 + (i * 2)] << 8) + pduRxData->SduDataPtr[2 + (i * 2)];
            if (lookupNonDynamicDid(didNr, &didPtr)) {	/** @req DCM438 */
                noRequestedDidSupported = FALSE;
                /* IMPROVEMENT: Check if the did has data or did ref. If not NRC here? */
                responseCode = readDidData(didPtr, pduTxData, &txPos, &pendingState, &pendingDid, &pendingSigIndex, &pendingDataLen, &nofDidsRead, nofDidsReadInPendingReq, &pendingDataStartPos);
                if( DCM_E_RESPONSEPENDING == responseCode ) {
                    dspUdsReadDidPending.reqDidIndex = i;
                } else {
                    /* No pending response */
                    nofDidsReadInPendingReq = 0;
                    nofDidsRead = 0;
                }
            } else if(LookupDDD(didNr,(const Dcm_DspDDDType **)&DDidPtr) == TRUE) {
				noRequestedDidSupported = FALSE;
				/*@req DCM651 *//* @req DCM652 *//* @req DCM653 */
				pduTxData->SduDataPtr[txPos] = (uint8)((DDidPtr->DynamicallyDid>>8) & 0xFF);
				txPos++;
				pduTxData->SduDataPtr[txPos] = (uint8)(DDidPtr->DynamicallyDid & 0xFF);
				txPos++;
				responseCode = readDDDData(DDidPtr,&(pduTxData->SduDataPtr[txPos]), &Length);
				txPos = txPos + Length;
			} else {
				/* DID not found. */
			}
		}
	} else {
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}
	if( (responseCode != DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT) && noRequestedDidSupported ) {
		/** @req DCM438 */
		/* None of the Dids in the request found. */
		responseCode = DCM_E_REQUESTOUTOFRANGE;
	}
	if (DCM_E_POSITIVERESPONSE == responseCode) {
		pduTxData->SduLength = txPos;
	}

	if( DCM_E_RESPONSEPENDING == responseCode) {
		dspUdsReadDidPending.state = pendingState;
		dspUdsReadDidPending.pduRxData = (PduInfoType*)pduRxData;
		dspUdsReadDidPending.pduTxData = pduTxData;
		dspUdsReadDidPending.nofReadDids = nofDidsRead;
		dspUdsReadDidPending.txWritePos = txPos;
		dspUdsReadDidPending.pendingDid = pendingDid;
		dspUdsReadDidPending.pendingDataLength = pendingDataLen;
		dspUdsReadDidPending.pendingSignalIndex = pendingSigIndex;
		dspUdsReadDidPending.pendingDataStartPos = pendingDataStartPos;
	} else {
		dspUdsReadDidPending.state = DCM_READ_DID_IDLE;
		dspUdsReadDidPending.nofReadDids = 0;
		DsdDspProcessingDone(responseCode);
	}
}


static Dcm_NegativeResponseCodeType readDidScalingData(const Dcm_DspDidType *didPtr, const PduInfoType *pduTxData, uint16 *txPos)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	const Dcm_DspDataType *dataPtr;

	if( (*txPos + didPtr->DspDidDataScalingInfoSize + 2) <= pduTxData->SduLength ) {
        pduTxData->SduDataPtr[(*txPos)++] = (didPtr->DspDidIdentifier >> 8) & 0xFFu;
        pduTxData->SduDataPtr[(*txPos)++] = didPtr->DspDidIdentifier & 0xFFu;

        for( uint16 i = 0; (i < didPtr->DspNofSignals) && (DCM_E_POSITIVERESPONSE == responseCode); i++ ) {
            Std_ReturnType result;
            Dcm_NegativeResponseCodeType errorCode;
            dataPtr = didPtr->DspSignalRef[i].DspSignalDataRef;
            if( NULL != dataPtr->DspDataGetScalingInfoFnc ) {
                /** @req DCM394 */
                result = dataPtr->DspDataGetScalingInfoFnc(DCM_INITIAL, &pduTxData->SduDataPtr[*txPos], &errorCode);
                *txPos += dataPtr->DspDataInfoRef->DspDidScalingInfoSize;
                if ((result != E_OK) || (errorCode != DCM_E_POSITIVERESPONSE)) {
                    responseCode = DCM_E_CONDITIONSNOTCORRECT;
                }
            } else {
                /* No scaling info function. Is it correct to give negative response here? */
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        }
	} else {
	    /* Not enough room in tx buffer */
	    responseCode = DCM_E_REQUESTOUTOFRANGE;
	}

	return responseCode;
}

void DspUdsReadScalingDataByIdentifier(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM258 */
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	uint16 didNr;
	const Dcm_DspDidType *didPtr = NULL;

	uint16 txPos = 1;

	if (pduRxData->SduLength == 3) {
		didNr = (uint16)((uint16)pduRxData->SduDataPtr[1] << 8) + pduRxData->SduDataPtr[2];
		if (lookupNonDynamicDid(didNr, &didPtr)) {
			responseCode = readDidScalingData(didPtr, pduTxData, &txPos);
		}
		else { // DID not found
			responseCode = DCM_E_REQUESTOUTOFRANGE;
		}

		if (responseCode == DCM_E_POSITIVERESPONSE) {
			pduTxData->SduLength = txPos;
		}
	}
	else {
		// Length not ok
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

	DsdDspProcessingDone(responseCode);
}

#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
static Dcm_NegativeResponseCodeType writeDidData(const Dcm_DspDidType *didPtr, const PduInfoType *pduRxData, uint16 writeDidLen)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    const Dcm_DspSignalType *signalPtr;
    if (didPtr->DspDidInfoRef->DspDidAccess.DspDidWrite != NULL ) {	/** @req DCM468 */
        if (DspCheckSessionLevel(didPtr->DspDidInfoRef->DspDidAccess.DspDidWrite->DspDidWriteSessionRef)) { /** @req DCM469 */
            if (DspCheckSecurityLevel(didPtr->DspDidInfoRef->DspDidAccess.DspDidWrite->DspDidWriteSecurityLevelRef)) { /** @req DCM470 */
                uint16 dataLen = 0;
                /* Check the size */
                /** @req DCM473 */
                if( (NULL != didPtr->DspSignalRef) &&
                        ( (didPtr->DspSignalRef[0].DspSignalDataRef->DspDataInfoRef->DspDidFixedLength && (writeDidLen == didPtr->DspDidDataSize)) ||
                          (!didPtr->DspSignalRef[0].DspSignalDataRef->DspDataInfoRef->DspDidFixedLength)) ) {
                    for( uint16 i = 0; i < (didPtr->DspNofSignals) && (DCM_E_POSITIVERESPONSE == responseCode); i++ ) {
                        signalPtr = &didPtr->DspSignalRef[i];
                        if( !signalPtr->DspSignalDataRef->DspDataInfoRef->DspDidFixedLength ) {
                            dataLen = writeDidLen;
                        }
                        Std_ReturnType result;
                        if(signalPtr->DspSignalDataRef->DspDataUsePort == DATA_PORT_BLOCK_ID){
#if defined(USE_NVM)
                            /* @req DCM541 */
                            if(dspUdsWriteDidPending.state == DCM_WRITE_DID_PENDING){
                                NvM_RequestResultType errorStatus = NVM_REQ_NOT_OK;
                                if (E_OK != NvM_GetErrorStatus(signalPtr->DspSignalDataRef->DspNvmUseBlockID, &errorStatus)){
                                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                    responseCode = DCM_E_GENERALREJECT;
                                } else {
                                    if (errorStatus == NVM_REQ_PENDING){
                                        responseCode = DCM_E_RESPONSEPENDING;
                                    }else if (errorStatus == NVM_REQ_OK){
                                        responseCode = DCM_E_POSITIVERESPONSE;
                                    }else{
                                        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                        responseCode = DCM_E_GENERALREJECT;
                                    }
                                }
                                if( DCM_E_RESPONSEPENDING != responseCode ) {
                                    /* Done or something failed. Lock block. */
                                    NvM_SetBlockLockStatus(signalPtr->DspSignalDataRef->DspNvmUseBlockID, TRUE);
                                }
                            }else{
                                NvM_SetBlockLockStatus(signalPtr->DspSignalDataRef->DspNvmUseBlockID, FALSE);
                                if (E_OK == NvM_WriteBlock(signalPtr->DspSignalDataRef->DspNvmUseBlockID, &pduRxData->SduDataPtr[3 + signalPtr->DspSignalsPosition])) {
                                    responseCode = DCM_E_RESPONSEPENDING;
                                }else{
                                    NvM_SetBlockLockStatus(signalPtr->DspSignalDataRef->DspNvmUseBlockID, TRUE);
                                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                    responseCode = DCM_E_GENERALREJECT;
                                }
                            }
#endif
                        }else{
                            if( NULL != signalPtr->DspSignalDataRef->DspDataWriteDataFnc.FixLenDataWriteFnc ) {
                                if(signalPtr->DspSignalDataRef->DspDataInfoRef->DspDidFixedLength) { /** @req DCM794 */
                                    result = signalPtr->DspSignalDataRef->DspDataWriteDataFnc.FixLenDataWriteFnc(&pduRxData->SduDataPtr[3 + signalPtr->DspSignalsPosition], DCM_INITIAL, &responseCode);  /** @req DCM395 */
                                } else {
                                    result = signalPtr->DspSignalDataRef->DspDataWriteDataFnc.DynLenDataWriteFnc(&pduRxData->SduDataPtr[3 + signalPtr->DspSignalsPosition], dataLen, DCM_INITIAL, &responseCode);  /** @req DCM395 */
                                }
                                if( result != E_OK && responseCode == DCM_E_POSITIVERESPONSE ) {
                                    responseCode = DCM_E_CONDITIONSNOTCORRECT;
                                } else if( DCM_E_RESPONSEPENDING == responseCode || E_PENDING == result ) {
                                    responseCode = DCM_E_RESPONSEPENDING;
                                }
                            } else {
                                /* No write function */
                                responseCode = DCM_E_REQUESTOUTOFRANGE;
                            }
                        }
                    }
                } else { // Length incorrect
                    responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                }
            } else {    // Not allowed in current security level
                responseCode = DCM_E_SECURITYACCESSDENIED;
            }
        } else {    // Not allowed in current session
            responseCode = DCM_E_SERVICENOTSUPPORTEDINACTIVESESSION;
        }
    } else {    // Read access not configured
        responseCode = DCM_E_REQUESTOUTOFRANGE;
    }

    return responseCode;
}

void DspUdsWriteDataByIdentifier(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM255 */
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	uint16 didNr;
	const Dcm_DspDidType *didPtr = NULL;
	uint16 didDataLength;

	didDataLength = pduRxData->SduLength - 3;
	didNr = (uint16)((uint16)pduRxData->SduDataPtr[1] << 8) + pduRxData->SduDataPtr[2];
	/* Check that did is in ok range. DEVIATION: ASR only allows 0x00FF-0xF1FF. */
	if ( (DID_IS_IN_ASR_WRITE_RANGE(didNr) || DID_IS_IN_SYS_SUPPLIER_SPECIFIC_RANGE(didNr)) &&
	        lookupNonDynamicDid(didNr, &didPtr)) {	/** @req DCM467 */
		responseCode = writeDidData(didPtr, pduRxData, didDataLength);
	} else { /* DID not found or in invalid range */
		responseCode = DCM_E_REQUESTOUTOFRANGE;
	}

	if( DCM_E_RESPONSEPENDING != responseCode ) {
		if (responseCode == DCM_E_POSITIVERESPONSE) {
			pduTxData->SduLength = 3;
			pduTxData->SduDataPtr[1] = (didNr >> 8) & 0xFFu;
			pduTxData->SduDataPtr[2] = didNr & 0xFFu;
		}

		dspUdsWriteDidPending.state = DCM_WRITE_DID_IDLE;
		DsdDspProcessingDone(responseCode);
	} else {
		dspUdsWriteDidPending.state = DCM_WRITE_DID_PENDING;
		dspUdsWriteDidPending.pduRxData = pduRxData;
		dspUdsWriteDidPending.pduTxData = pduTxData;
	}
}
#endif

Dcm_NegativeResponseCodeType DspUdsSecurityAccessGetSeedSubFnc (const PduInfoType *pduRxData, PduInfoType *pduTxData, Dcm_SecLevelType requestedSecurityLevel) {

    Dcm_NegativeResponseCodeType getSeedErrorCode;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    uint8 cntSecRow = 0;

    // requestSeed message
    // Check if type exist in security table
    const Dcm_DspSecurityRowType *securityRow = &Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[0];
    while ((securityRow->DspSecurityLevel != requestedSecurityLevel) && (!securityRow->Arc_EOL)) {
        securityRow++;
        cntSecRow++; // Get the index of the security config
    }
    if (!securityRow->Arc_EOL) {

#if (DCM_SECURITY_EOL_INDEX != 0)

        //Check if a wait is required before accepting a request
        dspUdsSecurityAccesData.currSecLevIdx = cntSecRow;
        if (dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].startDelayTimer != DELAY_TIMER_DEACTIVE) {
            responseCode = DCM_E_REQUIREDTIMEDELAYNOTEXPIRED;
        }
        else
#endif
        {
            // Check length
            if (pduRxData->SduLength == (2 + securityRow->DspSecurityADRSize)) {    /** @req DCM321.RequestSeed */
                Dcm_SecLevelType activeSecLevel;
                Std_ReturnType result;
                result = Dcm_GetSecurityLevel(&activeSecLevel);
                if (result == E_OK) {
                    if (requestedSecurityLevel == activeSecLevel) {     /** @req DCM323 */
                        pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
                        // If same level set the seed to zeroes
                        memset(&pduTxData->SduDataPtr[2], 0, securityRow->DspSecuritySeedSize);
                        pduTxData->SduLength = 2 + securityRow->DspSecuritySeedSize;
                    } else {
                        // New security level ask for seed
                        if (securityRow->GetSeed.getSeedWithoutRecord != NULL) {
                            Std_ReturnType getSeedResult;
                            if(securityRow->DspSecurityADRSize > 0) {
                                getSeedResult = securityRow->GetSeed.getSeedWithRecord(&pduRxData->SduDataPtr[2], DCM_INITIAL, &pduTxData->SduDataPtr[2], &getSeedErrorCode); /** @req DCM324.RequestSeed */
                            } else {
                                getSeedResult = securityRow->GetSeed.getSeedWithoutRecord(DCM_INITIAL, &pduTxData->SduDataPtr[2], &getSeedErrorCode); /** @req DCM324.RequestSeed */
                            }
                            if ((getSeedResult == E_OK) && (getSeedErrorCode == E_OK)) {
                                // Everything ok add sub function to tx message and send it.
                                pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
                                pduTxData->SduLength = 2 + securityRow->DspSecuritySeedSize;

                                dspUdsSecurityAccesData.reqSecLevel = requestedSecurityLevel;
                                dspUdsSecurityAccesData.reqSecLevelRef = securityRow;
                                dspUdsSecurityAccesData.reqInProgress = TRUE;
                            }
                            else {
                                // GetSeed returned not ok
                                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                            }
                        } else {
                            responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                        }
                    }
                } else {
                    // NOTE: What to do?
                    responseCode = DCM_E_GENERALREJECT;
                }
            }
            else {
                // Length not ok
                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
            }
        }
    }
    else {
        // Requested security level not configured
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }

    return responseCode;
}

// sendKey message
Dcm_NegativeResponseCodeType DspUdsSecurityAccessSendKeySubFnc (const PduInfoType *pduRxData, PduInfoType *pduTxData, Dcm_SecLevelType requestedSecurityLevel) {

    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

    /* Check whether senkey message is sent according to a valid sequence */
    if (dspUdsSecurityAccesData.reqInProgress && (requestedSecurityLevel == dspUdsSecurityAccesData.reqSecLevel)) {

        /* Client should reiterate the process of getseed msg, if sendkey fails- ISO14229 */
        dspUdsSecurityAccesData.reqInProgress = FALSE;

#if (DCM_SECURITY_EOL_INDEX != 0)
        //Check if a wait is required before accepting a request
        if (dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].startDelayTimer != DELAY_TIMER_DEACTIVE) {
            responseCode = DCM_E_REQUIREDTIMEDELAYNOTEXPIRED;
        }
        else
#endif
        {
            if (pduRxData->SduLength == (2 + dspUdsSecurityAccesData.reqSecLevelRef->DspSecurityKeySize)) { /** @req DCM321.SendKey */
                if (dspUdsSecurityAccesData.reqSecLevelRef->CompareKey != NULL) {
                       Std_ReturnType compareKeyResult;
                       compareKeyResult = dspUdsSecurityAccesData.reqSecLevelRef->CompareKey(&pduRxData->SduDataPtr[2]); /** @req DCM324.SendKey */
                       if (compareKeyResult == E_OK) {
                           // Request accepted
                           // Kill timer
                           DslSetSecurityLevel(dspUdsSecurityAccesData.reqSecLevelRef->DspSecurityLevel); /** @req DCM325 */
                           pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
                           pduTxData->SduLength = 2;
                       }
                       else {
                           responseCode = DCM_E_INVALIDKEY; /** @req DCM660 */
                       }
                   } else {
                       responseCode = DCM_E_CONDITIONSNOTCORRECT;
                   }
            }
            else {
                // Length not ok
                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
            }

#if (DCM_SECURITY_EOL_INDEX != 0)
            //Count the false access attempts -> Only send invalid keys events according to ISO 14229
            if (responseCode == DCM_E_INVALIDKEY) {
                dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].secAcssAttempts += 1;

                if (dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].secAcssAttempts >= dspUdsSecurityAccesData.reqSecLevelRef->DspSecurityNumAttDelay) {
                    //Enable delay timer
                    if (Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[dspUdsSecurityAccesData.currSecLevIdx].DspSecurityDelayTime >= DCM_MAIN_FUNCTION_PERIOD_TIME_MS) {
                        dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].startDelayTimer = DELAY_TIMER_ON_EXCEEDING_LIMIT_ACTIVE;
                        dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].timerSecAcssAttempt = Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[dspUdsSecurityAccesData.currSecLevIdx].DspSecurityDelayTime;
                    }
                    dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].secAcssAttempts  = 0;
                    responseCode = DCM_E_EXCEEDNUMBEROFATTEMPTS;
                }
            }
#endif
        }
    }
    else {
        // sendKey request without a preceding requestSeed
        responseCode = DCM_E_REQUESTSEQUENCEERROR;
    }

    return responseCode;
}

void DspUdsSecurityAccess(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM252 */
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

	// Check sub function range (0x01 to 0x42)
	if ((pduRxData->SduDataPtr[1] >= 0x01) && (pduRxData->SduDataPtr[1] <= 0x42)) {
		boolean isRequestSeed = pduRxData->SduDataPtr[1] & 0x01u;
		Dcm_SecLevelType requestedSecurityLevel = (pduRxData->SduDataPtr[1]+1)/2;


		if (isRequestSeed) {
		    responseCode = DspUdsSecurityAccessGetSeedSubFnc(pduRxData, pduTxData, requestedSecurityLevel);
		}
		else {
		    responseCode = DspUdsSecurityAccessSendKeySubFnc(pduRxData, pduTxData, requestedSecurityLevel);
		}
	}
	else {
		responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
	}

	DsdDspProcessingDone(responseCode);
}


static boolean lookupRoutine(uint16 routineId, const Dcm_DspRoutineType **routinePtr)
{
	const Dcm_DspRoutineType *dspRoutine = Dcm_ConfigPtr->Dsp->DspRoutine;
	boolean routineFound = FALSE;

	while ((dspRoutine->DspRoutineIdentifier != routineId) &&  (!dspRoutine->Arc_EOL)) {
		dspRoutine++;
	}

	if (!dspRoutine->Arc_EOL) {
		routineFound = TRUE;
		*routinePtr = dspRoutine;
	}

	return routineFound;
}


static Dcm_NegativeResponseCodeType startRoutine(const Dcm_DspRoutineType *routinePtr, const PduInfoType *pduRxData, PduInfoType *pduTxData, boolean isCancel)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	Std_ReturnType routineResult;
	Dcm_OpStatusType opStatus = (Dcm_OpStatusType)DCM_INITIAL;
	if(dspUdsRoutineControlPending.state == DCM_ROUTINE_CONTROL_PENDING){
	    if( isCancel ) {
	        opStatus = (Dcm_OpStatusType)DCM_CANCEL;
	    } else {
	        opStatus = (Dcm_OpStatusType)DCM_PENDING;
	    }
	}

	// startRoutine
	if ((routinePtr->DspStartRoutineFnc != NULL) && (routinePtr->DspRoutineInfoRef->DspStartRoutine != NULL)) {
		if (((routinePtr->DspRoutineInfoRef->DspStartRoutine->DspStartRoutineCtrlOptRecSize + 4) <= pduRxData->SduLength)
			&& ((routinePtr->DspRoutineInfoRef->DspStartRoutine->DspStartRoutineStsOptRecSize + 4) <= pduTxData->SduLength)) {
			uint16 currentDataLength = pduRxData->SduLength - 4;
			routineResult = routinePtr->DspStartRoutineFnc(&pduRxData->SduDataPtr[4], opStatus, &pduTxData->SduDataPtr[4], &currentDataLength, &responseCode, FALSE);	/** @req DCM400 */ /** @req DCM401 */
			pduTxData->SduLength = currentDataLength + 4;
			if (routineResult == E_PENDING){
			    responseCode = DCM_E_RESPONSEPENDING;
			}
			else if (routineResult != E_OK && responseCode == DCM_E_POSITIVERESPONSE) {
				responseCode = DCM_E_CONDITIONSNOTCORRECT;
			}
			if(responseCode == DCM_E_POSITIVERESPONSE && pduTxData->SduLength > routinePtr->DspRoutineInfoRef->DspStartRoutine->DspStartRoutineStsOptRecSize + 4) {
			    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_PARAM);
			    pduTxData->SduLength = routinePtr->DspRoutineInfoRef->DspStartRoutine->DspStartRoutineStsOptRecSize + 4;
			}
		}
		else {
			responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
		}
	}
	else {
		responseCode = DCM_E_REQUESTOUTOFRANGE;
	}
	return responseCode;
}


static Dcm_NegativeResponseCodeType stopRoutine(const Dcm_DspRoutineType *routinePtr, const PduInfoType *pduRxData, PduInfoType *pduTxData, boolean isCancel)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    Std_ReturnType routineResult;
    Dcm_OpStatusType opStatus = (Dcm_OpStatusType)DCM_INITIAL;
    if(dspUdsRoutineControlPending.state == DCM_ROUTINE_CONTROL_PENDING){
        if( isCancel ) {
            opStatus = (Dcm_OpStatusType)DCM_CANCEL;
        } else {
            opStatus = (Dcm_OpStatusType)DCM_PENDING;
        }
    }

    // stopRoutine
    if ((routinePtr->DspStopRoutineFnc != NULL) && (routinePtr->DspRoutineInfoRef->DspRoutineStop != NULL)) {
        if (((routinePtr->DspRoutineInfoRef->DspRoutineStop->DspStopRoutineCtrlOptRecSize + 4) <= pduRxData->SduLength)
                && ((routinePtr->DspRoutineInfoRef->DspRoutineStop->DspStopRoutineStsOptRecSize + 4) <= pduTxData->SduLength)) {
            uint16 currentDataLength = pduRxData->SduLength - 4;
            routineResult = routinePtr->DspStopRoutineFnc(&pduRxData->SduDataPtr[4], opStatus, &pduTxData->SduDataPtr[4], &currentDataLength, &responseCode, FALSE);	/** @req DCM402 */ /** @req DCM403 */
            pduTxData->SduLength = currentDataLength + 4;
            if (routineResult == E_PENDING){
                responseCode = DCM_E_RESPONSEPENDING;
            }
            else if (routineResult != E_OK && responseCode == DCM_E_POSITIVERESPONSE) {
                responseCode = DCM_E_CONDITIONSNOTCORRECT;
            }
            if(responseCode == DCM_E_POSITIVERESPONSE && pduTxData->SduLength > routinePtr->DspRoutineInfoRef->DspRoutineStop->DspStopRoutineStsOptRecSize + 4) {
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_PARAM);
                pduTxData->SduLength = routinePtr->DspRoutineInfoRef->DspRoutineStop->DspStopRoutineStsOptRecSize + 4;
            }
        }
        else {
            responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
        }
    }
    else {
        responseCode = DCM_E_REQUESTOUTOFRANGE;
    }

    return responseCode;
}


static Dcm_NegativeResponseCodeType requestRoutineResults(const Dcm_DspRoutineType *routinePtr, PduInfoType *pduTxData, boolean isCancel)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    Std_ReturnType routineResult;
    Dcm_OpStatusType opStatus = (Dcm_OpStatusType)DCM_INITIAL;
    if(dspUdsRoutineControlPending.state == DCM_ROUTINE_CONTROL_PENDING){
        if( isCancel ) {
            opStatus = (Dcm_OpStatusType)DCM_CANCEL;
        } else {
            opStatus = (Dcm_OpStatusType)DCM_PENDING;
        }
    }

    // requestRoutineResults
    if ((routinePtr->DspRequestResultRoutineFnc != NULL) && (routinePtr->DspRoutineInfoRef->DspRoutineRequestRes != NULL)) {
        if ((routinePtr->DspRoutineInfoRef->DspRoutineRequestRes->DspReqResRtnCtrlOptRecSize + 4) <= pduTxData->SduLength) {
            uint16 currentDataLength = 0;
            routineResult = routinePtr->DspRequestResultRoutineFnc(opStatus, &pduTxData->SduDataPtr[4], &currentDataLength, &responseCode, FALSE);	/** @req DCM404 */ /** @req DCM405 */
            pduTxData->SduLength = currentDataLength + 4;
            if (routineResult == E_PENDING){
                responseCode = DCM_E_RESPONSEPENDING;
            }
            else if (routineResult != E_OK && responseCode == DCM_E_POSITIVERESPONSE) {
                responseCode = DCM_E_CONDITIONSNOTCORRECT;
            }
            if(responseCode == DCM_E_POSITIVERESPONSE && pduTxData->SduLength > routinePtr->DspRoutineInfoRef->DspRoutineRequestRes->DspReqResRtnCtrlOptRecSize + 4) {
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_PARAM);
                pduTxData->SduLength = routinePtr->DspRoutineInfoRef->DspRoutineRequestRes->DspReqResRtnCtrlOptRecSize + 4;
            }
        }
        else {
            responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
        }
    }
    else {
        responseCode = DCM_E_REQUESTOUTOFRANGE;
    }

    return responseCode;
}

/**
 * Cancels a pending routine request
 * @param pduRxData
 * @param pduTxData
 */
void DspCancelPendingRoutine(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
    uint8 subFunctionNumber = pduRxData->SduDataPtr[1];
    uint16 routineId = 0;
    if( (NULL != pduRxData) && (NULL != pduTxData)) {
        const Dcm_DspRoutineType *routinePtr = NULL;
        routineId = (uint16)((uint16)pduRxData->SduDataPtr[2] << 8) + pduRxData->SduDataPtr[3];
        if (lookupRoutine(routineId, &routinePtr)) {
            switch (subFunctionNumber) {
            case 0x01:  // startRoutine
                (void)startRoutine(routinePtr, pduRxData, pduTxData, TRUE);
                break;
            case 0x02:  // stopRoutine
                (void)stopRoutine(routinePtr, pduRxData, pduTxData, TRUE);
                break;
            case 0x03:  // requestRoutineResults
                (void)requestRoutineResults(routinePtr, pduTxData, TRUE);
                break;
            default:    // This shall never happen
                break;
            }
        }
    } else {
        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_PARAM);
    }
}
void DspUdsRoutineControl(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM257 */
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	uint8 subFunctionNumber = 0;
	uint16 routineId = 0;
	const Dcm_DspRoutineType *routinePtr = NULL;

	if (pduRxData->SduLength >= 4) {
		subFunctionNumber = pduRxData->SduDataPtr[1];
		if ((subFunctionNumber > 0) && (subFunctionNumber < 4)) {
			routineId = (uint16)((uint16)pduRxData->SduDataPtr[2] << 8) + pduRxData->SduDataPtr[3];
			if (lookupRoutine(routineId, &routinePtr)) {
				if (DspCheckSessionLevel(routinePtr->DspRoutineInfoRef->DspRoutineAuthorization.DspRoutineSessionRef)) {
					if (DspCheckSecurityLevel(routinePtr->DspRoutineInfoRef->DspRoutineAuthorization.DspRoutineSecurityLevelRef)) {
						switch (subFunctionNumber) {
						case 0x01:	// startRoutine
							responseCode = startRoutine(routinePtr, pduRxData, pduTxData, FALSE);
							break;

						case 0x02:	// stopRoutine
							responseCode = stopRoutine(routinePtr, pduRxData, pduTxData, FALSE);
							break;

						case 0x03:	// requestRoutineResults
							responseCode =  requestRoutineResults(routinePtr, pduTxData, FALSE);
							break;

						default:	// This shall never happen
							responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
							break;
						}
					}
					else {	// Not allowed in current security level
						responseCode = DCM_E_SECURITYACCESSDENIED;
					}
				}
				else {	// Not allowed in current session
					responseCode = DCM_E_SERVICENOTSUPPORTEDINACTIVESESSION;
				}
			}
			else {	// Unknown routine identifier
				responseCode = DCM_E_REQUESTOUTOFRANGE;
			}
		}
		else {	// Sub function not supported
			responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
		}
	}
	else {
		// Wrong length
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

    if( DCM_E_RESPONSEPENDING != responseCode ) {
        if (responseCode == DCM_E_POSITIVERESPONSE) {
            // Add header to the positive response message
            pduTxData->SduDataPtr[1] = subFunctionNumber;
            pduTxData->SduDataPtr[2] = (routineId >> 8) & 0xFFu;
            pduTxData->SduDataPtr[3] = routineId & 0xFFu;
        }

        dspUdsRoutineControlPending.state = DCM_ROUTINE_CONTROL_IDLE;
        DsdDspProcessingDone(responseCode);
    }
    else {
        dspUdsRoutineControlPending.state = DCM_ROUTINE_CONTROL_PENDING;
        dspUdsRoutineControlPending.pduRxData = pduRxData;
        dspUdsRoutineControlPending.pduTxData = pduTxData;
    }
}


void DspUdsTesterPresent(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM251 */
	if (pduRxData->SduLength == 2) {
		switch (pduRxData->SduDataPtr[1])
		{
		case ZERO_SUB_FUNCTION:
			DslResetSessionTimeoutTimer();
			// Create positive response
			pduTxData->SduDataPtr[1] = ZERO_SUB_FUNCTION;
			pduTxData->SduLength = 2;
			DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
			break;

		default:
			DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED);
			break;
		}
	}
	else {
		// Wrong length
		DsdDspProcessingDone(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
	}
}

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_CONTROLDTCSETTING)
void DspUdsControlDtcSetting(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	/** @req DCM249 */
	Dem_ReturnControlDTCStorageType resultCode;

	if (pduRxData->SduLength == 2) {
		switch (pduRxData->SduDataPtr[1])
		{
		case 0x01:	// ON
			resultCode = Dem_EnableDTCSetting(DEM_DTC_GROUP_ALL_DTCS, DEM_DTC_KIND_ALL_DTCS);		/** @req DCM304 */
			if (resultCode == DEM_CONTROL_DTC_STORAGE_OK) {
				pduTxData->SduDataPtr[1] = 0x01;
				pduTxData->SduLength = 2;
				DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
			}
			else {
				DsdDspProcessingDone(DCM_E_REQUESTOUTOFRANGE);
			}
			break;

		case 0x02:	// OFF
			resultCode = Dem_DisableDTCSetting(DEM_DTC_GROUP_ALL_DTCS, DEM_DTC_KIND_ALL_DTCS);		/** @req DCM406 */
			if (resultCode == DEM_CONTROL_DTC_STORAGE_OK) {
				pduTxData->SduDataPtr[1] = 0x02;
				pduTxData->SduLength = 2;
				DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
			}
			else {
				DsdDspProcessingDone(DCM_E_REQUESTOUTOFRANGE);
			}
			break;

		default:
			DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED);
			break;
		}
	}
	else {
		// Wrong length
		DsdDspProcessingDone(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
	}
}
#endif


#ifdef DCM_USE_SERVICE_RESPONSEONEVENT
void DspResponseOnEvent(const PduInfoType *pduRxData, PduIdType rxPduId, PduInfoType *pduTxData) {

    uint8 eventWindowTime = 0;
    uint16 eventTypeRecord = 0;
    uint8 serviceToRespondToLength = 0;
    uint8 storageState = 0;

    /* The minimum request includes Sid, Sub function, and event window time */
    if (pduRxData->SduLength >= 3) {

        storageState = pduRxData->SduDataPtr[1] & 0x40; /* Bit 6 */
        eventWindowTime = pduRxData->SduDataPtr[2];

        switch ( pduRxData->SduDataPtr[1] & 0x3F) {

        case 0x00:  /* Stop */
            DsdDspProcessingDone(DCM_ROE_Stop(storageState, eventWindowTime, pduTxData));
            break;

        case 0x01:  /* onDTCStatusChanged */
            DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED); //Not Implemented
            break;

        case 0x02:  /* onTimerInterrupt */
            DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED); //Not Implemented
            break;

        case 0x03:  /* OnChangeOfDataIdentifier */
            /* @req Dcm522 */
            eventTypeRecord = (pduRxData->SduDataPtr[3] << 8) +  pduRxData->SduDataPtr[4];
            serviceToRespondToLength = pduRxData->SduLength - 5;
            DsdDspProcessingDone(
                    DCM_ROE_AddDataIdentifierEvent(eventWindowTime,
                                                   storageState,
                                                   eventTypeRecord,
                                                   &pduRxData->SduDataPtr[5],
                                                   serviceToRespondToLength,
                                                   pduTxData));
            break;

        case 0x04:  /* reportActiveEvents */
            DsdDspProcessingDone(DCM_ROE_GetEventList(storageState, pduTxData));
            break;

        case 0x05:  /* startResponseOnEvent */
            DsdDspProcessingDone(DCM_ROE_Start(storageState, eventWindowTime, rxPduId, pduTxData));
            break;

        case 0x06:  /* clearResponseOnEvent */
            DsdDspProcessingDone(DCM_ROE_ClearEventList(storageState, eventWindowTime, pduTxData));
            break;

        case 0x07:  /* onComparisonOfValue */
            DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED); //Not Implemented
            break;

        default:
            DsdDspProcessingDone(DCM_E_SUBFUNCTIONNOTSUPPORTED);
            break;
        }
    }
    else {
        // Wrong length
        DsdDspProcessingDone(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
    }
}

#endif


void DspDcmConfirmation(PduIdType confirmPduId)
{
    DslResetSessionTimeoutTimer(); /** @req DCM141 */
    if ( DCM_DSP_RESET_WAIT_TX_CONF == dspUdsEcuResetData.resetPending ) {
        if (confirmPduId == dspUdsEcuResetData.resetPduId) {
            dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
            /* IMPROVEMENT: Should be a call to SchM */
            (void)Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_EXECUTE);
        }
    }

    if (dspUdsSessionControlData.sessionPending) {
        if (confirmPduId == dspUdsSessionControlData.sessionPduId) {
            dspUdsSessionControlData.sessionPending = FALSE;
            /* @req DCM311 */
            DslSetSesCtrlType(dspUdsSessionControlData.session);
            Dcm_DiagnosticSessionControl(dspUdsSessionControlData.session);
        }
    }
}
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
void DspResponsePendingConfirmed(PduIdType confirmPduId)
{
    if (dspUdsSessionControlData.sessionPending) {
        if (confirmPduId == dspUdsSessionControlData.sessionPduId) {
            if( DCM_JTB_WAIT_RESPONSE_PENDING_TX_CONFIRM == dspUdsSessionControlData.jumpToBootState ) {
                /* @req DCM654 */
                /* @req DCM535 */
                /* IMPROVEMENT: Add support for pending */
                Std_ReturnType status = E_NOT_OK;
                if(E_OK == Dcm_SetProgConditions(&GlobalProgConditions)) {
                    if(E_OK == Rte_Switch_Dcm_dcm_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_EXECUTE)) {
                        status = E_OK;
                    }
                }
                if( E_OK != status ) {
                    /* @req DCM715 */
                    DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
                }
                dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
            }
        }
    }
}
#endif

static Dcm_NegativeResponseCodeType readMemoryData( Dcm_OpStatusType *OpStatus,
													uint8 memoryIdentifier,
													uint32 MemoryAddress,
													uint32 MemorySize,
													PduInfoType *pduTxData)
{
	Dcm_ReturnReadMemoryType ReadRet;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	*OpStatus = DCM_INITIAL;
	ReadRet = Dcm_ReadMemory(*OpStatus,memoryIdentifier,
									MemoryAddress,
									MemorySize,
									&pduTxData->SduDataPtr[1]);
	if(DCM_READ_FAILED == ReadRet)
	{
		responseCode = DCM_E_GENERALPROGRAMMINGFAILURE;  /*@req Dcm644*/
	}
	if (DCM_READ_PENDING == ReadRet)
	{
		*OpStatus = DCM_READ_PENDING;
	}	
	return responseCode;
}

static Dcm_NegativeResponseCodeType checkAddressRange(DspMemoryServiceType serviceType, uint8 memoryIdentifier, uint32 memoryAddress, uint32 length) {
	const Dcm_DspMemoryIdInfo *dspMemoryInfo = Dcm_ConfigPtr->Dsp->DspMemory->DspMemoryIdInfo;
	const Dcm_DspMemoryRangeInfo *memoryRangeInfo = NULL;
	Dcm_NegativeResponseCodeType diagResponseCode = DCM_E_REQUESTOUTOFRANGE;

	for( ; (dspMemoryInfo->Arc_EOL == FALSE) && (memoryRangeInfo == NULL); dspMemoryInfo++ )
	{
		if( ((TRUE == Dcm_ConfigPtr->Dsp->DspMemory->DcmDspUseMemoryId) && (dspMemoryInfo->MemoryIdValue == memoryIdentifier))
			|| (FALSE == Dcm_ConfigPtr->Dsp->DspMemory->DcmDspUseMemoryId) )
		{
			if( DCM_READ_MEMORY == serviceType )
			{
				memoryRangeInfo = findRange( dspMemoryInfo->pReadMemoryInfo, memoryAddress, length );
			}
			else
			{
				memoryRangeInfo = findRange( dspMemoryInfo->pWriteMemoryInfo, memoryAddress, length );
			}

			if( NULL != memoryRangeInfo )
			{
			    if( DspCheckSecurityLevel(memoryRangeInfo->pSecurityLevel))
				{
					/* Range is ok */
					diagResponseCode = DCM_E_POSITIVERESPONSE;
				}
				else
				{
					diagResponseCode = DCM_E_SECURITYACCESSDENIED;
				}
			}
			else {
				/* Range was not configured for read/write */
				diagResponseCode = DCM_E_REQUESTOUTOFRANGE;
			}
		}
		else {
			/* No memory with this id found */
			diagResponseCode = DCM_E_REQUESTOUTOFRANGE;
		}
	}
	return diagResponseCode;
}

static const Dcm_DspMemoryRangeInfo* findRange(const Dcm_DspMemoryRangeInfo *memoryRangePtr, uint32 memoryAddress, uint32 length)
{
	const Dcm_DspMemoryRangeInfo *memoryRange = NULL;

	for( ; (memoryRangePtr->Arc_EOL == FALSE) && (memoryRange == NULL); memoryRangePtr++ )
	{
		/*@req DCM493*/
		if((memoryAddress >= memoryRangePtr->MemoryAddressLow)
			&& (memoryAddress <= memoryRangePtr->MemoryAddressHigh)
			&& (memoryAddress + length - 1 <= memoryRangePtr->MemoryAddressHigh))
		{
			memoryRange = memoryRangePtr;
		}
	}

	return memoryRange;
}

void DspUdsWriteMemoryByAddress(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType diagResponseCode;
	uint8 sizeFormat = 0;
	uint8 addressFormat = 0;
	uint32 memoryAddress = 0;
	uint32 length = 0;
	uint8 i = 0;
	uint8 memoryIdentifier = 0; /* Should be 0 if DcmDspUseMemoryId == FALSE */
	Dcm_OpStatusType OpStatus = DCM_INITIAL;
	uint8 addressOffset;

	if( pduRxData->SduLength > ALFID_INDEX )
	{
		sizeFormat = ((uint8)(pduRxData->SduDataPtr[ALFID_INDEX] & DCM_FORMAT_HIGH_MASK)) >> 4;	/*@req UDS_REQ_0x23_1 & UDS_REQ_0x23_5*/;
		addressFormat = ((uint8)(pduRxData->SduDataPtr[ALFID_INDEX])) & DCM_FORMAT_LOW_MASK;   /*@req UDS_REQ_0x23_1 & UDS_REQ_0x23_5*/;
		if((addressFormat != 0) && (sizeFormat != 0))
		{
			if(addressFormat + sizeFormat + SID_LEN + ALFID_LEN <= pduRxData->SduLength)
			{
				if( TRUE == Dcm_ConfigPtr->Dsp->DspMemory->DcmDspUseMemoryId ) {
					memoryIdentifier = pduRxData->SduDataPtr[ADDR_START_INDEX];
					addressOffset = 1;
				}
				else {
					addressOffset = 0;
				}

				/* Parse address */
				for(i = addressOffset; i < addressFormat; i++)
				{
					memoryAddress <<= 8;
					memoryAddress += (uint32)(pduRxData->SduDataPtr[ADDR_START_INDEX + i]);
				}

				/* Parse size */
				for(i = 0; i < sizeFormat; i++)
				{
					length <<= 8;
					length += (uint32)(pduRxData->SduDataPtr[ADDR_START_INDEX + addressFormat + i]);
				}

				if( addressFormat + sizeFormat + SID_LEN + ALFID_LEN + length == pduRxData->SduLength )
				{

					diagResponseCode = checkAddressRange(DCM_WRITE_MEMORY, memoryIdentifier, memoryAddress, length);
					if( DCM_E_POSITIVERESPONSE == diagResponseCode )
					{
						diagResponseCode = writeMemoryData(&OpStatus, memoryIdentifier, memoryAddress, length,
													&pduRxData->SduDataPtr[SID_LEN + ALFID_LEN + addressFormat + sizeFormat]);
					}

				}
				else
				{
					diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
				}
			}
			else
			{
				diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
			}
		}
		else
		{
			diagResponseCode = DCM_E_REQUESTOUTOFRANGE;  /*UDS_REQ_0x23_10*/
		}
	}
	else
	{
		diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

	if(DCM_E_POSITIVERESPONSE == diagResponseCode)
	{
		pduTxData->SduLength = SID_LEN + ALFID_LEN + addressFormat + sizeFormat;
		pduTxData->SduDataPtr[ALFID_INDEX] = pduRxData->SduDataPtr[ALFID_INDEX];
		for(i = 0; i < addressFormat + sizeFormat; i++)
		{
			pduTxData->SduDataPtr[ADDR_START_INDEX + i] = pduRxData->SduDataPtr[ADDR_START_INDEX + i];
			if(OpStatus != DCM_WRITE_PENDING)
			{
				DsdDspProcessingDone(diagResponseCode);
			}
			else
			{
        		dspMemoryState=DCM_MEMORY_WRITE;
			}
		}
	}
	else
	{
		DsdDspProcessingDone(diagResponseCode);
	}
}

/*@req Dcm442,DCM492*/
void DspUdsReadMemoryByAddress(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType diagResponseCode;
	uint8 sizeFormat;
	uint8 addressFormat;
	uint32 memoryAddress = 0;
	uint32 length = 0;
	uint8 i;
	uint8 memoryIdentifier = 0; /* Should be 0 if DcmDspUseMemoryId == FALSE */
	Dcm_OpStatusType OpStatus = DCM_INITIAL;
	uint8 addressOffset;

	if( pduRxData->SduLength > ALFID_INDEX )
	{
		sizeFormat = ((uint8)(pduRxData->SduDataPtr[ALFID_INDEX] & DCM_FORMAT_HIGH_MASK)) >> 4;	/*@req UDS_REQ_0x23_1 & UDS_REQ_0x23_5*/;
		addressFormat = ((uint8)(pduRxData->SduDataPtr[ALFID_INDEX])) & DCM_FORMAT_LOW_MASK;   /*@req UDS_REQ_0x23_1 & UDS_REQ_0x23_5*/;
		if((addressFormat != 0) && (sizeFormat != 0))
		{
			if(addressFormat + sizeFormat + SID_LEN + ALFID_LEN == pduRxData->SduLength)
			{
				if( TRUE == Dcm_ConfigPtr->Dsp->DspMemory->DcmDspUseMemoryId ) {
					memoryIdentifier = pduRxData->SduDataPtr[ADDR_START_INDEX];
					addressOffset = 1;
				}
				else {
					addressOffset = 0;
				}

				/* Parse address */
				for(i = addressOffset; i < addressFormat; i++)
				{
					memoryAddress <<= 8;
					memoryAddress += (uint32)(pduRxData->SduDataPtr[ADDR_START_INDEX + i]);
				}

				/* Parse size */
				for(i = 0; i < sizeFormat; i++)
				{
					length <<= 8;
					length += (uint32)(pduRxData->SduDataPtr[ADDR_START_INDEX + addressFormat + i]);
				}

				if(length <= (DCM_PROTOCAL_TP_MAX_LENGTH - SID_LEN) )
				{
					diagResponseCode = checkAddressRange(DCM_READ_MEMORY, memoryIdentifier, memoryAddress, length);
					if( DCM_E_POSITIVERESPONSE == diagResponseCode )
					{
						diagResponseCode = readMemoryData(&OpStatus, memoryIdentifier, memoryAddress, length, pduTxData);
					}
				}
				else {
					diagResponseCode = DCM_E_REQUESTOUTOFRANGE;
				}
			}
			else
			{
				diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
			}
		}
		else
		{
			diagResponseCode = DCM_E_REQUESTOUTOFRANGE;  /*UDS_REQ_0x23_10*/
		}
	}
	else
	{
		diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

	if(DCM_E_POSITIVERESPONSE == diagResponseCode)
	{
		pduTxData->SduLength = SID_LEN + length;
		if(OpStatus == DCM_READ_PENDING)
		{
			dspMemoryState = DCM_MEMORY_READ;
		}
		else
		{
			DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
		}
	}
	else
	{
		DsdDspProcessingDone(diagResponseCode);
	}
}

static Dcm_NegativeResponseCodeType writeMemoryData(Dcm_OpStatusType* OpStatus,
												uint8 memoryIdentifier,
												uint32 MemoryAddress,
												uint32 MemorySize,
												uint8 *SourceData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	Dcm_ReturnWriteMemoryType writeRet;
	*OpStatus = DCM_INITIAL;
	writeRet = Dcm_WriteMemory(*OpStatus,
								memoryIdentifier,
								MemoryAddress,
								MemorySize,
								SourceData);
	if(DCM_WRITE_FAILED == writeRet)
	{
		responseCode = DCM_E_GENERALPROGRAMMINGFAILURE;   /*@req UDS_REQ_0X3D_16,DCM643*/
	}
	else if(DCM_WRITE_PENDING == writeRet)
	{
		*OpStatus = DCM_PENDING;
	}
	else
	{
		responseCode = DCM_E_POSITIVERESPONSE;
	}
	
	return responseCode;
}

#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static boolean isInPDidBuffer(uint8 PeriodicDid)
{
    boolean ret = FALSE;
    for(uint8 i = 0; (i < dspPDidRef.PDidNofUsed) && (ret == FALSE); i++) {
        if(PeriodicDid == dspPDidRef.dspPDid[i].PeriodicDid) {
            ret = TRUE;
        }
    }
    return ret;
}
#endif
#ifdef DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER
static boolean checkPeriodicIdentifierBuffer(uint8 PeriodicDid, PduIdType rxPduId, uint8 *postion)
{
    boolean ret = FALSE;
    for(uint8 i = 0; (i < dspPDidRef.PDidNofUsed) && (ret == FALSE); i++)
    {
        if((PeriodicDid == dspPDidRef.dspPDid[i].PeriodicDid) && (rxPduId == dspPDidRef.dspPDid[i].PDidRxPduID)) {
            ret = TRUE;
            *postion = i;
        }
    }

    return ret;
}

static void DspPdidRemove(uint8 pDid, PduIdType rxPduId)
{
    uint8 pos = 0;
    if( checkPeriodicIdentifierBuffer(pDid, rxPduId, &pos) ) {
        dspPDidRef.PDidNofUsed--;
        dspPDidRef.dspPDid[pos].PeriodicDid = dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PeriodicDid;
        dspPDidRef.dspPDid[pos].PDidTxCounter = dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidTxCounter;
        dspPDidRef.dspPDid[pos].PDidTxPeriod = dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidTxPeriod;
        dspPDidRef.dspPDid[pos].PDidRxPduID = dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidRxPduID;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PeriodicDid = 0;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidTxCounter = 0;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidTxPeriod = 0;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidRxPduID = 0;
        if(dspPDidRef.nextStartIndex >= dspPDidRef.PDidNofUsed) {
            dspPDidRef.nextStartIndex = 0;
        }
    }
}
typedef enum {
    PDID_ADDED = 0,
    PDID_UPDATED,
    PDID_BUFFER_FULL
}PdidEntryStatusType;

static PdidEntryStatusType DspPdidAddOrUpdate(uint8 pDid, PduIdType rxPduId, uint32 periodicTransmitCounter)
{
    uint8 indx = 0;
    PdidEntryStatusType ret = PDID_BUFFER_FULL;
    if( checkPeriodicIdentifierBuffer(pDid, rxPduId, &indx) ) {
        if( 0 != periodicTransmitCounter ) {
            dspPDidRef.dspPDid[indx].PDidTxPeriod = periodicTransmitCounter;
            dspPDidRef.dspPDid[indx].PDidTxCounter = 0;
        }
        ret = PDID_UPDATED;
    } else if(dspPDidRef.PDidNofUsed < DCM_LIMITNUMBER_PERIODDATA) {
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PeriodicDid = pDid;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidTxCounter = 0;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidTxPeriod = periodicTransmitCounter;
        dspPDidRef.dspPDid[dspPDidRef.PDidNofUsed].PDidRxPduID = rxPduId;
        dspPDidRef.PDidNofUsed++;
        ret = PDID_ADDED;
    }
    return ret;
}

static Dcm_NegativeResponseCodeType readPeriodDidData(const Dcm_DspDidType *PDidPtr, uint8 *Data, uint16 *Length)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	const Dcm_DspSignalType *signalPtr;
	*Length = 0;
	if (PDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead != NULL ) {
        Std_ReturnType result = E_NOT_OK;
        Dcm_NegativeResponseCodeType errorCode = DCM_E_POSITIVERESPONSE;
        uint16 dataLen = 0;
        for( uint16 i = 0; (i < PDidPtr->DspNofSignals) && (DCM_E_POSITIVERESPONSE == responseCode); i++ ) {
            signalPtr = &PDidPtr->DspSignalRef[i];
            if( NULL != signalPtr->DspSignalDataRef->DspDataConditionCheckReadFnc ) {
                result = signalPtr->DspSignalDataRef->DspDataConditionCheckReadFnc(DCM_INITIAL, &errorCode);
                if( (E_PENDING == result) && (DATA_PORT_ASYNCH != signalPtr->DspSignalDataRef->DspDataUsePort)) {
                    /* Synch port cannot return E_PENDING */
                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                    result = E_NOT_OK;
                }
                if( (result == E_OK) && (errorCode == DCM_E_POSITIVERESPONSE) )  {
                    result = E_NOT_OK;
                    if ( signalPtr->DspSignalDataRef->DspDataInfoRef->DspDidFixedLength ) {
                        dataLen = signalPtr->DspSignalDataRef->DspDataSize;
                        result = E_OK;
                    } else {
                        if( NULL != signalPtr->DspSignalDataRef->DspDataReadDataLengthFnc ) {
                            result = signalPtr->DspSignalDataRef->DspDataReadDataLengthFnc(&dataLen);
                            if( E_OK != result ) {
                                /* Read function is only allowed to return E_OK */
                                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                result = E_NOT_OK;
                            }
                        } else {
                            responseCode = DCM_E_GENERALREJECT;
                        }
                    }
                    if( result == E_OK )  {
                        if( *Length < dataLen + signalPtr->DspSignalsPosition ) {
                            *Length = dataLen + signalPtr->DspSignalsPosition;
                        }
                        if( DATA_PORT_SYNCH == signalPtr->DspSignalDataRef->DspDataUsePort ) {
                            if( NULL !=  signalPtr->DspSignalDataRef->DspDataReadDataFnc.SynchDataReadFnc) {
                                result = signalPtr->DspSignalDataRef->DspDataReadDataFnc.SynchDataReadFnc(&Data[signalPtr->DspSignalsPosition]);
                            } else {
                                /* No read function */
                                result = E_NOT_OK;
                            }
                            if( E_OK != result ) {
                                /* Synch port cannot return E_PENDING */
                                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                result = E_NOT_OK;
                            }
                        } else if( DATA_PORT_ASYNCH == signalPtr->DspSignalDataRef->DspDataUsePort ) {
                            if( NULL != signalPtr->DspSignalDataRef->DspDataReadDataFnc.AsynchDataReadFnc ) {
                                result = signalPtr->DspSignalDataRef->DspDataReadDataFnc.AsynchDataReadFnc(DCM_INITIAL, &Data[signalPtr->DspSignalsPosition]);
                            } else {
                                /* No read function */
                                result = E_NOT_OK;
                            }
                        } else {
                            /* Port not supported */
                            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_CONFIG_INVALID);
                            result = E_NOT_OK;
                        }
                        if (result != E_OK) {
                            responseCode = DCM_E_REQUESTOUTOFRANGE;
                        }
                    } else {
                        responseCode = DCM_E_REQUESTOUTOFRANGE;
                    }
                } else {
                    responseCode = DCM_E_REQUESTOUTOFRANGE;
                }

            } else {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        }
	} else {
		responseCode = DCM_E_REQUESTOUTOFRANGE;
	}
	return responseCode;
}

static void ClearPeriodicIdentifier(const PduInfoType *pduRxData,PduInfoType *pduTxData, PduIdType rxPduId )
{
	uint16 PdidNumber;
	uint8 i;
	if( pduRxData->SduDataPtr[1] == DCM_PERIODICTRANSMIT_STOPSENDING_MODE )
	{
		PdidNumber = pduRxData->SduLength - 2;
		for(i = 0;i < PdidNumber;i++)
		{
			DspPdidRemove(pduRxData->SduDataPtr[2 + i], rxPduId);
				
		}
		pduTxData->SduLength = 1;
	}
}
/*
	DESCRIPTION:
		 UDS Service 0x2a - Read Data By Periodic Identifier
*/

#define DID_OK (uint8)(1<<0)
#define SECURITY_OK (uint8)(1<<1)
#define SESSION_OK (uint8)(1<<2)
#define REQUEST_OK (uint8)(1<<3)

static uint16 DspGetProtocolMaxPDidLength(Dcm_ProtocolTransTypeType txType)
{
    uint16 maxLength = 0;
    Dcm_ProtocolType activeProtocol;
    if(E_OK == DslGetActiveProtocol(&activeProtocol)) {
        switch(activeProtocol) {
            case DCM_OBD_ON_CAN:
            case DCM_UDS_ON_CAN:
            case DCM_ROE_ON_CAN:
            case DCM_PERIODICTRANS_ON_CAN:
                maxLength = (DCM_PROTOCOL_TRANS_TYPE_2 == txType) ? MAX_TYPE2_PERIODIC_DID_LEN_CAN : MAX_TYPE1_PERIODIC_DID_LEN_CAN;
                break;
            case DCM_OBD_ON_FLEXRAY:
            case DCM_UDS_ON_FLEXRAY:
            case DCM_ROE_ON_FLEXRAY:
            case DCM_PERIODICTRANS_ON_FLEXRAY:
                maxLength = (DCM_PROTOCOL_TRANS_TYPE_2 == txType) ? MAX_TYPE2_PERIODIC_DID_LEN_FLEXRAY : MAX_TYPE1_PERIODIC_DID_LEN_FLEXRAY;
                break;
            case DCM_OBD_ON_IP:
            case DCM_UDS_ON_IP:
            case DCM_ROE_ON_IP:
            case DCM_PERIODICTRANS_ON_IP:
                maxLength = (DCM_PROTOCOL_TRANS_TYPE_2 == txType) ? MAX_TYPE2_PERIODIC_DID_LEN_IP : MAX_TYPE1_PERIODIC_DID_LEN_IP;
                break;
            default:
                break;
        }
    }
    return maxLength;
}

static boolean checkPDidSupported(uint16 pDid, uint16 *didLength, Dcm_NegativeResponseCodeType *responseCode)
{
    const Dcm_DspDidType *PDidPtr = NULL;
    Dcm_DspDDDType *DDidPtr = NULL;
    boolean didSupported = FALSE;
    boolean isDynamicDid = FALSE;

    if( lookupNonDynamicDid(pDid, &PDidPtr) ) {
        didSupported = TRUE;
    } else if(lookupDynamicDid(pDid, &PDidPtr)) {
        didSupported = TRUE;
        isDynamicDid = TRUE;
    }

    *responseCode = DCM_E_REQUESTOUTOFRANGE;
    if(didSupported && (NULL != PDidPtr) && (NULL != PDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead)) {
        /* @req DCM721 */
        if(DspCheckSessionLevel(PDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSessionRef)) {
            /* @req DCM722 */
            *responseCode = DCM_E_SECURITYACCESSDENIED;
            if(DspCheckSecurityLevel(PDidPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSecurityLevelRef)) {
                *responseCode = DCM_E_POSITIVERESPONSE;
                if( isDynamicDid ) {
                    *responseCode = DCM_E_REQUESTOUTOFRANGE;
                    if( LookupDDD(pDid, (const Dcm_DspDDDType **)&DDidPtr) ) {
                        /* It is a dynamically defined did */
                        /* @req DCM721 *//* @req DCM722 */
                        *responseCode = checkDDDConditions(DDidPtr, didLength);
                    }
                }
            }
        }
    }
    return didSupported;
}
static Dcm_NegativeResponseCodeType getPDidData(uint16 did, uint8 *data, uint16 *dataLength)
{
    Dcm_NegativeResponseCodeType responseCode;
    const Dcm_DspDidType *PDidPtr = NULL;
    Dcm_DspDDDType *DDidPtr = NULL;
    if( lookupNonDynamicDid(did, &PDidPtr) ) {
        responseCode = readPeriodDidData(PDidPtr, data, dataLength);
    } else if( LookupDDD(did, (const Dcm_DspDDDType **)&DDidPtr) ) {
        responseCode = readDDDData(DDidPtr, data, dataLength);
    } else {
        responseCode = DCM_E_REQUESTOUTOFRANGE;
    }
    return responseCode;
}

void DspReadDataByPeriodicIdentifier(const PduInfoType *pduRxData, PduInfoType *pduTxData, PduIdType rxPduId, Dcm_ProtocolTransTypeType txType, boolean internalRequest)
{
    /** @req DCM254 */
    uint8 PDidLowByte;
    uint16 PdidNumber;
    uint32 periodicTransmitCounter = 0;
    uint16 DataLength;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    boolean secAccOK = FALSE;
    boolean requestOK = FALSE;
    boolean requestHasSupportedDid = FALSE;
    uint8 nofPdidsAdded = 0;
    uint8 pdidsAdded[DCM_LIMITNUMBER_PERIODDATA];
    uint16 maxDidLen = 0;
    boolean supressNRC = FALSE;
    memset(pdidsAdded, 0, DCM_LIMITNUMBER_PERIODDATA);
    if(pduRxData->SduLength > 2) {

        switch(pduRxData->SduDataPtr[1])
        {
            case DCM_PERIODICTRANSMIT_DEFAULT_MODE:
                periodicTransmitCounter = 0;
                responseCode = internalRequest ? responseCode:DCM_E_REQUESTOUTOFRANGE;
                break;
            case DCM_PERIODICTRANSMIT_SLOWRATE_MODE:
                periodicTransmitCounter = DCM_PERIODICTRANSMIT_SLOW;
                break;
                case DCM_PERIODICTRANSMIT_MEDIUM_MODE:
                    periodicTransmitCounter = DCM_PERIODICTRANSMIT_MEDIUM;
                break;
            case DCM_PERIODICTRANSMIT_FAST_MODE:
                periodicTransmitCounter = DCM_PERIODICTRANSMIT_FAST;
                break;
            case DCM_PERIODICTRANSMIT_STOPSENDING_MODE:
                ClearPeriodicIdentifier(pduRxData,pduTxData, rxPduId);
                break;
            default:
                responseCode = DCM_E_REQUESTOUTOFRANGE;
                break;
        }
        if((pduRxData->SduDataPtr[1] != DCM_PERIODICTRANSMIT_STOPSENDING_MODE) && (DCM_E_POSITIVERESPONSE == responseCode)) {
            maxDidLen = DspGetProtocolMaxPDidLength(txType);
            PdidNumber = pduRxData->SduLength - 2;
            /* Check the dids in the request. Must be "small" enough to fit in the response frame.
             * If there are more dids in the request than we can handle, we only give negative response code
             * if the number of supported dids in the request are greater than the number of entries left
             * in our buffer. */
            for( uint8 indx = 0; (indx < PdidNumber) && (DCM_E_POSITIVERESPONSE == responseCode); indx++ ) {
                PDidLowByte = pduRxData->SduDataPtr[2 + indx];
                uint16 didLength = 0;
                Dcm_NegativeResponseCodeType resp = DCM_E_POSITIVERESPONSE;
                if(checkPDidSupported(TO_PERIODIC_DID(PDidLowByte), &didLength, &resp)) {
                    requestHasSupportedDid = TRUE;
                    secAccOK = secAccOK || (DCM_E_SECURITYACCESSDENIED != resp);
                    if((DCM_E_POSITIVERESPONSE == resp) && (didLength <= maxDidLen)) {
                        requestOK = TRUE;
                        PdidEntryStatusType pdidStatus = DspPdidAddOrUpdate(PDidLowByte, rxPduId, periodicTransmitCounter);
                        if( PDID_ADDED == pdidStatus) {
                            pdidsAdded[nofPdidsAdded++] = PDidLowByte;
                        } else if( PDID_BUFFER_FULL == pdidStatus ){
                            /* Would exceed the maximum number of periodic dids.
                             * Clear the ones added now. */
                            for( uint8 idx = 0; idx < nofPdidsAdded; idx++ ) {
                                DspPdidRemove(pdidsAdded[idx], rxPduId);
                            }
                            responseCode = DCM_E_REQUESTOUTOFRANGE;
                            requestOK = FALSE;
                        }
                    }
                }
            }

            if( requestOK ) {
                /* Request contained at least one supported DID
                 * accessible in the current session and security level */
                uint8 dataStartIndex = 1; /* Type 1*/

                if (DCM_PROTOCOL_TRANS_TYPE_2 == txType) {
                    dataStartIndex = 0;
                    memset(pduTxData->SduDataPtr, 0, 8); /* The buffer is always 8 bytes */
                }

                if( (1 == PdidNumber) && (0 == periodicTransmitCounter)) {
                    supressNRC = TRUE;
                    responseCode = getPDidData(TO_PERIODIC_DID(PDidLowByte), &pduTxData->SduDataPtr[dataStartIndex + 1], &DataLength);
                    if(responseCode != DCM_E_POSITIVERESPONSE) {
                        /* NOTE: If a read returns error, should we really remove the did? */
                        DspPdidRemove(PDidLowByte, rxPduId);
                    } else {
                        pduTxData->SduDataPtr[dataStartIndex] = PDidLowByte;
                        pduTxData->SduLength = DataLength + dataStartIndex + 1;
                    }
                } else {
                    pduTxData->SduLength = 1;
                }
            } else {
                if(requestHasSupportedDid && !secAccOK) {
                    /* Request contained supported did(s) but none had access in the current security level */
                    /* @req DCM721 */
                    responseCode = DCM_E_SECURITYACCESSDENIED;
                } else {
                    /* No did available in current session, buffer overflow or no did in request will fit in single frame */
                    /* @req DCM722 */
                    responseCode = DCM_E_REQUESTOUTOFRANGE;
                }
            }
        }
    } else if( (pduRxData->SduLength == 2) && (pduRxData->SduDataPtr[1] == DCM_PERIODICTRANSMIT_STOPSENDING_MODE) ) {
        memset(&dspPDidRef,0,sizeof(dspPDidRef));
        pduTxData->SduLength = 1;
    } else  {
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }
    DsdDspProcessingDone_ReadDataByPeriodicIdentifier(responseCode, supressNRC);
}
#endif

#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static Dcm_NegativeResponseCodeType dynamicallyDefineDataIdentifierbyDid(uint16 DDIdentifier,const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
    uint8 i;
    uint16 SourceDidNr;
    const Dcm_DspDidType *SourceDid = NULL;
    Dcm_DspDDDType *DDid = NULL;
    uint16 SourceLength = 0;
    uint16 DidLength = 0;
    uint16 Length = 0;
    uint8 Num = 0;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    const Dcm_DspSignalType *signalPtr;

    if(FALSE == LookupDDD(DDIdentifier, (const Dcm_DspDDDType **)&DDid)) {
        while((Num < DCM_MAX_DDD_NUMBER) && (dspDDD[Num].DynamicallyDid != 0 )) {
            Num++;
        }
        if(Num >= DCM_MAX_DDD_NUMBER) {
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        } else {
            DDid = &dspDDD[Num];
        }
    } else {
        while((SourceLength < DCM_MAX_DDDSOURCE_NUMBER) && (DDid->DDDSource[SourceLength].formatOrPosition != 0 )) {
            SourceLength++;
        }
    }
    if(responseCode == DCM_E_POSITIVERESPONSE) {
        Length = (pduRxData->SduLength - SID_AND_DDDID_LEN) /SDI_AND_MS_LEN;
        if(((Length*SDI_AND_MS_LEN) == (pduRxData->SduLength - SID_AND_DDDID_LEN)) && (Length != 0)) {
            if((Length + SourceLength) <= DCM_MAX_DDDSOURCE_NUMBER) {
                for(i = 0;(i < Length) && (responseCode == DCM_E_POSITIVERESPONSE);i++) {
                    SourceDidNr = (((uint16)pduRxData->SduDataPtr[SID_AND_DDDID_LEN + i*SDI_AND_MS_LEN] << 8) & DCM_DID_HIGH_MASK) + (((uint16)pduRxData->SduDataPtr[(5 + i*SDI_AND_MS_LEN)]) & DCM_DID_LOW_MASK);
                    if( lookupNonDynamicDid(SourceDidNr, &SourceDid) && (NULL != SourceDid->DspDidInfoRef->DspDidAccess.DspDidRead) ) {/*UDS_REQ_0x2C_4*/
                        /* @req DCM725 */
                        if(DspCheckSessionLevel(SourceDid->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSessionRef)) {
                            /* @treq DCM726 */
                            if(DspCheckSecurityLevel(SourceDid->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSecurityLevelRef)) {
                                DidLength = 0;
                                uint16 tempSize = 0;
                                for( uint16 sigIndex = 0; (sigIndex < SourceDid->DspNofSignals) && (responseCode == DCM_E_POSITIVERESPONSE); sigIndex++ ) {
                                    tempSize = 0;
                                    signalPtr = &SourceDid->DspSignalRef[sigIndex];
                                    if( signalPtr->DspSignalDataRef->DspDataInfoRef->DspDidFixedLength ) {
                                        tempSize = signalPtr->DspSignalsPosition + signalPtr->DspSignalDataRef->DspDataSize;
                                    } else if( NULL != signalPtr->DspSignalDataRef->DspDataReadDataLengthFnc ) {
                                        (void)signalPtr->DspSignalDataRef->DspDataReadDataLengthFnc(&tempSize);
                                        tempSize += signalPtr->DspSignalsPosition;
                                    }
                                    if( tempSize > DidLength ) {
                                        DidLength = tempSize;
                                    }
                                }
                                if(DidLength != 0) {
                                    if((pduRxData->SduDataPtr[SID_AND_SDI_LEN + i*SDI_AND_MS_LEN] != 0) &&
                                        (pduRxData->SduDataPtr[SID_AND_PISDR_LEN + i*SDI_AND_MS_LEN] != 0) &&
                                        (((uint16)pduRxData->SduDataPtr[SID_AND_SDI_LEN + i*SDI_AND_MS_LEN] + (uint16)pduRxData->SduDataPtr[SID_AND_PISDR_LEN + i*SID_AND_DDDID_LEN] - 1) <= DidLength))
                                    {
                                        DDid->DDDSource[i + SourceLength].formatOrPosition = pduRxData->SduDataPtr[SID_AND_SDI_LEN + i*SDI_AND_MS_LEN];
                                        DDid->DDDSource[i + SourceLength].Size = pduRxData->SduDataPtr[SID_AND_PISDR_LEN + i*SDI_AND_MS_LEN];
                                        DDid->DDDSource[i + SourceLength].SourceAddressOrDid = SourceDid->DspDidIdentifier;
                                        DDid->DDDSource[i + SourceLength].DDDTpyeID = DCM_DDD_SOURCE_DID;
                                    } else {
                                        /*UDS_REQ_0x2C_6*/
                                        responseCode = DCM_E_REQUESTOUTOFRANGE;
                                    }
                                } else {
                                    /*UDS_REQ_0x2C_14*/
                                    responseCode = DCM_E_REQUESTOUTOFRANGE;
                                }
                            } else {
                                responseCode = DCM_E_SECURITYACCESSDENIED;
                            }
                        } else {
                            /*UDS_REQ_0x2C_19,DCM726*/
                            responseCode = DCM_E_REQUESTOUTOFRANGE;
                        }
                    } else {
                        /*DCM725*/
                        responseCode = DCM_E_REQUESTOUTOFRANGE;
                    }
                }
            } else {
                /*UDS_REQ_0x2C_13*/
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        } else {
            /*UDS_REQ_0x2C_11*/
            responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
        }
        if(responseCode == DCM_E_POSITIVERESPONSE) {
            DDid->DynamicallyDid = DDIdentifier;
            pduTxData->SduDataPtr[1] = DCM_DDD_SUBFUNCTION_DEFINEBYDID;
        }
    }
    if(responseCode == DCM_E_POSITIVERESPONSE) {
        pduTxData->SduDataPtr[1] = DCM_DDD_SUBFUNCTION_DEFINEBYDID;
    }
    
    return responseCode;
}

static Dcm_NegativeResponseCodeType dynamicallyDefineDataIdentifierbyAddress(uint16 DDIdentifier,const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
	uint16 numNewDefinitions;
	uint16 numEarlierDefinitions = 0;
	Dcm_DspDDDType *DDid = NULL;
	uint8 Num = 0;
	uint8 definitionIndex;
	Dcm_NegativeResponseCodeType diagResponseCode = DCM_E_POSITIVERESPONSE;
	uint8 sizeFormat;
	uint8 addressFormat;
	uint32 memoryAddress = 0;
	uint32 length = 0;
	uint8 i;
	uint8 memoryIdentifier = 0; /* Should be 0 if DcmDspUseMemoryId == FALSE */
	uint8 addressOffset;
	
	if(FALSE == LookupDDD(DDIdentifier, (const Dcm_DspDDDType **)&DDid))
	{
		while((Num < DCM_MAX_DDD_NUMBER) && (dspDDD[Num].DynamicallyDid != 0 ))
		{
			Num++;
		}
		if(Num >= DCM_MAX_DDD_NUMBER)
		{
			diagResponseCode = DCM_E_REQUESTOUTOFRANGE;
		}
		else
		{
			DDid = &dspDDD[Num];
		}
	}
	else
	{
		while((numEarlierDefinitions < DCM_MAX_DDDSOURCE_NUMBER) && (DDid->DDDSource[numEarlierDefinitions].formatOrPosition != 0 ))
		{
			numEarlierDefinitions++;
		}
		if(numEarlierDefinitions >= DCM_MAX_DDDSOURCE_NUMBER)
		{
			diagResponseCode = DCM_E_REQUESTOUTOFRANGE;
		}
	}

	if( diagResponseCode == DCM_E_POSITIVERESPONSE )
	{
		if( pduRxData->SduLength > DYNDEF_ALFID_INDEX )
		{
			sizeFormat = ((uint8)(pduRxData->SduDataPtr[DYNDEF_ALFID_INDEX] & DCM_FORMAT_HIGH_MASK)) >> 4;	/*@req UDS_REQ_0x23_1 & UDS_REQ_0x23_5*/;
			addressFormat = ((uint8)(pduRxData->SduDataPtr[DYNDEF_ALFID_INDEX])) & DCM_FORMAT_LOW_MASK;   /*@req UDS_REQ_0x23_1 & UDS_REQ_0x23_5*/;
			if((addressFormat != 0) && (sizeFormat != 0))
			{
				numNewDefinitions = (pduRxData->SduLength - (SID_LEN + SF_LEN + DDDDI_LEN + ALFID_LEN) ) / (sizeFormat + addressFormat);
				if( (numNewDefinitions != 0) &&
					((SID_LEN + SF_LEN + DDDDI_LEN + ALFID_LEN + numNewDefinitions * (sizeFormat + addressFormat)) == pduRxData->SduLength) )
				{
					if( (numEarlierDefinitions+numNewDefinitions) <= DCM_MAX_DDDSOURCE_NUMBER )
					{
						for( definitionIndex = 0; (definitionIndex < numNewDefinitions) && (diagResponseCode == DCM_E_POSITIVERESPONSE); definitionIndex++ )
						{

							if( TRUE == Dcm_ConfigPtr->Dsp->DspMemory->DcmDspUseMemoryId ) {
								memoryIdentifier = pduRxData->SduDataPtr[DYNDEF_ADDRESS_START_INDEX + definitionIndex * (sizeFormat + addressFormat)];
								addressOffset = 1;
							}
							else {
								addressOffset = 0;
							}

							/* Parse address */
							memoryAddress = 0;
							for(i = addressOffset; i < addressFormat; i++)
							{
								memoryAddress <<= 8;
								memoryAddress += (uint32)(pduRxData->SduDataPtr[DYNDEF_ADDRESS_START_INDEX + definitionIndex * (sizeFormat + addressFormat) + i]);
							}

							/* Parse size */
							length = 0;
							for(i = 0; i < sizeFormat; i++)
							{
								length <<= 8;
								length += (uint32)(pduRxData->SduDataPtr[DYNDEF_ADDRESS_START_INDEX + definitionIndex * (sizeFormat + addressFormat) + addressFormat + i]);
							}

							diagResponseCode = checkAddressRange(DCM_READ_MEMORY, memoryIdentifier, memoryAddress, length);
							if( DCM_E_POSITIVERESPONSE == diagResponseCode )
							{
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].formatOrPosition = pduRxData->SduDataPtr[DYNDEF_ALFID_INDEX];
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].memoryIdentifier = memoryIdentifier;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].SourceAddressOrDid = memoryAddress;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].Size = length;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].DDDTpyeID = DCM_DDD_SOURCE_ADDRESS;
							}
						}
						if(diagResponseCode == DCM_E_POSITIVERESPONSE)
						{
							DDid->DynamicallyDid = DDIdentifier;
						}
						else
						{
							for( definitionIndex = 0; (definitionIndex < numNewDefinitions); definitionIndex++ )
							{
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].formatOrPosition = 0x00;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].memoryIdentifier = 0x00;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].SourceAddressOrDid = 0x00000000;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].Size = 0x0000;
								DDid->DDDSource[definitionIndex + numEarlierDefinitions].DDDTpyeID = DCM_DDD_SOURCE_DEFAULT;
							}
						}
					}
					else
					{
						diagResponseCode = DCM_E_REQUESTOUTOFRANGE;
					}
				}
				else
				{
					diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
				}
			}
			else
			{
				diagResponseCode = DCM_E_REQUESTOUTOFRANGE;  /*UDS_REQ_0x23_10*/
			}
		}
		else
		{
			diagResponseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
		}
	}


	if(diagResponseCode == DCM_E_POSITIVERESPONSE)
	{
		pduTxData->SduDataPtr[SF_INDEX] = DCM_DDD_SUBFUNCTION_DEFINEBYADDRESS;
	}
	
	return diagResponseCode;
}


/*
	DESCRIPTION:
		 UDS Service 0x2c - Clear dynamically Did
*/
static Dcm_NegativeResponseCodeType ClearDynamicallyDefinedDid(uint16 DDIdentifier,const PduInfoType *pduRxData, PduInfoType * pduTxData)
{
	/*UDS_REQ_0x2C_5*/
	sint8 i, j;
	Dcm_DspDDDType *DDid = NULL;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	
	if(pduRxData->SduLength == 4)
	{
		if(TRUE == LookupDDD(DDIdentifier, (const Dcm_DspDDDType **)&DDid))
		{
			if((isInPDidBuffer(pduRxData->SduDataPtr[3]) == TRUE) && (pduRxData->SduDataPtr[2] == 0xF2))
			{
				/*UDS_REQ_0x2C_9*/
				responseCode = DCM_E_REQUESTOUTOFRANGE;
			}
			else
			{
				memset(DDid, 0, sizeof(Dcm_DspDDDType));
				for(i = DCM_MAX_DDD_NUMBER - 1;i >= 0 ;i--) {	/* find the first DDDid from bottom */
					if (0 != dspDDD[i].DynamicallyDid) {
						for (j = 0; j <DCM_MAX_DDD_NUMBER; j++) { /* find the first empty slot from top */
							if (j >= i) {
								/* Rearrange finished */
								pduTxData->SduDataPtr[1] = DCM_DDD_SUBFUNCTION_CLEAR;
								pduTxData->SduLength = 2;
								return responseCode;
							}
							else if (0 == dspDDD[j].DynamicallyDid) {	/* find, exchange */
								memcpy(&dspDDD[j], (Dcm_DspDDDType*)&dspDDD[i], sizeof(Dcm_DspDDDType));
								memset((Dcm_DspDDDType*)&dspDDD[i], 0, sizeof(Dcm_DspDDDType));
							}
						}
					}
				}
			}
		}
		else{
			responseCode = DCM_E_REQUESTOUTOFRANGE;	/* DDDid not found */
		}
	}

	else
	{
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}
	if(responseCode == DCM_E_POSITIVERESPONSE)
	{
		pduTxData->SduDataPtr[1] = DCM_DDD_SUBFUNCTION_CLEAR;
		pduTxData->SduLength = 2;
	}
	
	return responseCode;
}

void DspDynamicallyDefineDataIdentifier(const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
    /*UDS_REQ_0x2C_1,DCM 259*/
    uint16 i;
    uint16 DDIdentifier;
    boolean PeriodicUse = FALSE;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    const Dcm_DspDidType *dDidPtr = NULL;

    if(pduRxData->SduLength > 2) {
        /* Check if DDID is in the range 0xF200-0xF3FF */
        DDIdentifier = ((((uint16)pduRxData->SduDataPtr[2]) << 8) & DCM_DID_HIGH_MASK) + (pduRxData->SduDataPtr[3] & DCM_DID_LOW_MASK);
        /* @req DCM723 */
        if( lookupDynamicDid(DDIdentifier, &dDidPtr) && (NULL != dDidPtr) && (NULL != dDidPtr->DspDidInfoRef->DspDidAccess.DspDidWrite) && DspCheckSessionLevel(dDidPtr->DspDidInfoRef->DspDidAccess.DspDidWrite->DspDidWriteSessionRef)) {
            /* @req DCM724 */
            if( DspCheckSecurityLevel(dDidPtr->DspDidInfoRef->DspDidAccess.DspDidWrite->DspDidWriteSecurityLevelRef) ) {
                switch(pduRxData->SduDataPtr[1])	/*UDS_REQ_0x2C_2,DCM 646*/
                {
                    case DCM_DDD_SUBFUNCTION_DEFINEBYDID:
                        responseCode  = dynamicallyDefineDataIdentifierbyDid(DDIdentifier,pduRxData,pduTxData);
                        break;
                    case DCM_DDD_SUBFUNCTION_DEFINEBYADDRESS:
                        responseCode = dynamicallyDefineDataIdentifierbyAddress(DDIdentifier,pduRxData,pduTxData);
                        break;
                    case DCM_DDD_SUBFUNCTION_CLEAR:
                        responseCode = ClearDynamicallyDefinedDid(DDIdentifier, pduRxData,pduTxData);
                        break;
                    default:
                        responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
                        /*UDS_REQ_0x2C_10*/
                        break;
                }
            } else {
                responseCode = DCM_E_SECURITYACCESSDENIED;
            }
        } else {
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        }
        if(responseCode == DCM_E_POSITIVERESPONSE) {
            pduTxData->SduDataPtr[2] = pduRxData->SduDataPtr[2];
            pduTxData->SduDataPtr[3] = pduRxData->SduDataPtr[3];
            pduTxData->SduLength = 4;
        }
    } else if((pduRxData->SduLength == 2)&&(pduRxData->SduDataPtr[1] == DCM_DDD_SUBFUNCTION_CLEAR)) {
        /*UDS_REQ_0x2C_7*/
        for(i = 0;i < DCM_MAX_DDD_NUMBER;i++) {
            if(isInPDidBuffer((uint8)(dspDDD[i].DynamicallyDid & DCM_DID_LOW_MASK)) == TRUE) {
                PeriodicUse = TRUE;
            }
        }
        if(PeriodicUse == FALSE) {
            memset(dspDDD,0,sizeof(dspDDD));
            pduTxData->SduDataPtr[1] = DCM_DDD_SUBFUNCTION_CLEAR;
            pduTxData->SduLength = 2;
        } else {
            responseCode = DCM_E_CONDITIONSNOTCORRECT;
        }
    } else {
        /*UDS_REQ_0x2C_11*/
        responseCode =  DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }
    DsdDspProcessingDone(responseCode);
}
#endif

#if defined(DCM_USE_SERVICE_INPUTOUTPUTCONTROLBYIDENTIFIER) && defined(DCM_USE_CONTROL_DIDS)
static void DspStopInputOutputControl(boolean checkSessionAndSecLevel)
{
    const Dcm_DspDidControlType *DidControl = NULL;
    const Dcm_DspDidType *DidPtr = NULL;
    const Dcm_DspSignalType *signalPtr;
    Dcm_NegativeResponseCodeType responseCode;
    /* @req DCM628 */
    boolean serviceSupported = DsdDspCheckServiceSupportedInActiveSessionAndSecurity(SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER);
    for(uint16 i = 0; i < DCM_NOF_IOCONTROL_DIDS; i++) {
        if( IOControlStateList[i].controlActive ) {
            /* Control not in the hands of the ECU.
             * Return it. */
            if(lookupNonDynamicDid(IOControlStateList[i].did, &DidPtr)) {
                DidControl = DidPtr->DspDidInfoRef->DspDidAccess.DspDidControl;
                if(NULL != DidControl) {
                    boolean returnToECU = TRUE;
                    if(serviceSupported && checkSessionAndSecLevel) {
                        /* Should check if supported in session and security level */
                        if( DspCheckSessionLevel(DidControl->DspDidControlSessionRef) && DspCheckSecurityLevel(DidControl->DspDidControlSecurityLevelRef) ) {
                            /* Control is supported in current session and security level.
                             * Do not return control to ECU. */
                            returnToECU = FALSE;
                        }
                    }
                    if( returnToECU ) {
                        /* Return control to the ECU */
                        for( uint16 sigIndex = 0; sigIndex < DidPtr->DspNofSignals; sigIndex++ ) {
                            if( IOControlStateList[i].activeSignalBitfield[sigIndex/8] & TO_SIGNAL_BIT(sigIndex) ) {
                                signalPtr = &DidPtr->DspSignalRef[sigIndex];
                                if ((signalPtr->DspSignalDataRef->DspDataUsePort == DATA_PORT_ECU_SIGNAL) && (signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.EcuSignalReturnControlToECUFnc != NULL)){
                                    (void)signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.EcuSignalReturnControlToECUFnc(DCM_RETURN_CONTROL_TO_ECU, NULL);
                                }
                                else if(signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.FuncReturnControlToECUFnc != NULL) {
                                    (void)signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.FuncReturnControlToECUFnc(DCM_INITIAL, &responseCode);
                                }
                                IOControlStateList[i].activeSignalBitfield[sigIndex/8] &= ~(TO_SIGNAL_BIT(sigIndex));
                            }
                        }
                        IOControlStateList[i].controlActive = FALSE;
                    } else {
                        /* Control is supported in the current session and security level */
                    }
                } else {
                    /* No control access. */
                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
                    IOControlStateList[i].controlActive = FALSE;
                }
            } else {
                /* Did not found in config. Strange.. */
                IOControlStateList[i].controlActive = FALSE;
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
            }
        }
    }
}

static void DspIOControlSetActive(uint16 didNr, Dcm_DspIOControlVector signalAffected, boolean active)
{
    uint16 unusedIndex = DCM_NOF_IOCONTROL_DIDS;
    uint16 didIndex = DCM_NOF_IOCONTROL_DIDS;
    boolean indexOK = TRUE;
    uint16 indx = 0;

    for(uint16 i = 0; (i < DCM_NOF_IOCONTROL_DIDS) && (DCM_NOF_IOCONTROL_DIDS == didIndex); i++) {
        if(didNr == IOControlStateList[i].did) {
            didIndex = i;
        } else if( (DCM_NOF_IOCONTROL_DIDS == unusedIndex) && !IOControlStateList[i].controlActive ) {
            unusedIndex = i;
        }
    }

    if( DCM_NOF_IOCONTROL_DIDS > didIndex ) {
        indx = didIndex;
    } else if( active && (DCM_NOF_IOCONTROL_DIDS > unusedIndex) ) {
        indx = unusedIndex;
    } else {
        indexOK = FALSE;
    }

    if( indexOK ) {
        /* Did was in list or found and unused slot and should activate */

        IOControlStateList[indx].controlActive = FALSE;
        IOControlStateList[indx].did = didNr;
        for( uint8 byteIndex = 0; byteIndex < sizeof(Dcm_DspIOControlVector); byteIndex++ ) {
            if( active ) {
                IOControlStateList[indx].activeSignalBitfield[byteIndex] |= signalAffected[byteIndex];
            } else {
                IOControlStateList[indx].activeSignalBitfield[byteIndex] &= ~(signalAffected[byteIndex]);
            }
            if(IOControlStateList[indx].activeSignalBitfield[byteIndex]) {
                IOControlStateList[indx].controlActive = TRUE;
            }
        }
    } else if( active ) {
        /* Should set control active but could not find an entry
         * to use. */
        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
    }
}

static void DspIOControlStopNonActive(uint16 didNr, Dcm_DspIOControlVector signalActivated)
{
    const Dcm_DspDidType *DidPtr = NULL;
    const Dcm_DspSignalType *signalPtr;
    Dcm_NegativeResponseCodeType responseCode;
    boolean done = FALSE;
    Dcm_DspIOControlVector currentlyActive;
    memset(currentlyActive, 0, sizeof(Dcm_DspIOControlVector));
    if(lookupNonDynamicDid(didNr, &DidPtr)) {
        for(uint16 i = 0; (i < DCM_NOF_IOCONTROL_DIDS) && !done; i++) {
            if(didNr == IOControlStateList[i].did) {
                memcpy(currentlyActive, IOControlStateList[i].activeSignalBitfield, sizeof(Dcm_DspIOControlVector));
                done = TRUE;
            }
        }
    }

    if(NULL != DidPtr) {
        for( uint16 sigIndex = 0; sigIndex < DidPtr->DspNofSignals; sigIndex++ ) {
            if( (0 == (currentlyActive[sigIndex/8] & TO_SIGNAL_BIT(sigIndex))) &&
                (0 != (signalActivated[sigIndex/8] & TO_SIGNAL_BIT(sigIndex))) ) {
                signalPtr = &DidPtr->DspSignalRef[sigIndex];
                if ((signalPtr->DspSignalDataRef->DspDataUsePort == DATA_PORT_ECU_SIGNAL) && (signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.EcuSignalReturnControlToECUFnc != NULL)){
                    (void)signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.EcuSignalReturnControlToECUFnc(DCM_RETURN_CONTROL_TO_ECU, NULL);
                }
                else if(signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.FuncReturnControlToECUFnc != NULL) {
                    (void)signalPtr->DspSignalDataRef->DspDataReturnControlToEcuFnc.FuncReturnControlToECUFnc(DCM_INITIAL, &responseCode);
                }
            }
        }
    }
}
#endif

#ifdef DCM_USE_SERVICE_INPUTOUTPUTCONTROLBYIDENTIFIER

/* This is used when the port is USE_ECU_SIGNAL i.e. calling IOHWAB */
static Std_ReturnType EcuSignalInputOutputControl(const Dcm_DspDataType *DataPtr, Dcm_IOControlParameterType action, uint8* controlOptionRecord, Dcm_NegativeResponseCodeType* responseCode) {
    /* @req DCM580 */
    *responseCode = DCM_E_REQUESTOUTOFRANGE; // Value to use if no callback found
    Std_ReturnType retVal = E_NOT_OK;

    switch(action)
    {
    case DCM_RETURN_CONTROL_TO_ECU:
        if (DataPtr->DspDataReturnControlToEcuFnc.EcuSignalReturnControlToECUFnc != NULL) {
            *responseCode = DCM_E_POSITIVERESPONSE;
            retVal = DataPtr->DspDataReturnControlToEcuFnc.EcuSignalReturnControlToECUFnc(DCM_RETURN_CONTROL_TO_ECU, NULL);
        }
        break;
    case DCM_RESET_TO_DEFAULT:
        if (DataPtr->DspDataResetToDefaultFnc.EcuSignalResetToDefaultFnc != NULL){
            *responseCode = DCM_E_POSITIVERESPONSE;
            retVal = DataPtr->DspDataResetToDefaultFnc.EcuSignalResetToDefaultFnc(DCM_RESET_TO_DEFAULT, NULL);
        }
        break;
    case DCM_FREEZE_CURRENT_STATE:
        if (DataPtr->DspDataFreezeCurrentStateFnc.EcuSignalFreezeCurrentStateFnc != NULL){
            *responseCode = DCM_E_POSITIVERESPONSE;
            retVal = DataPtr->DspDataFreezeCurrentStateFnc.EcuSignalFreezeCurrentStateFnc(DCM_FREEZE_CURRENT_STATE, NULL);
        }
        break;
    case DCM_SHORT_TERM_ADJUSTMENT:
        if (DataPtr->DspDataShortTermAdjustmentFnc.EcuSignalShortTermAdjustmentFnc != NULL){
            *responseCode = DCM_E_POSITIVERESPONSE;
            retVal = DataPtr->DspDataShortTermAdjustmentFnc.EcuSignalShortTermAdjustmentFnc(DCM_SHORT_TERM_ADJUSTMENT, controlOptionRecord);
        }
        break;
    default:
        break;
    }

    return retVal;
}

/* This is used when the port is not USE_ECU_SIGNAL */
static Std_ReturnType FunctionInputOutputControl(const Dcm_DspDataType *DataPtr, Dcm_IOControlParameterType action, uint8* controlOptionRecord, Dcm_NegativeResponseCodeType* responseCode) {

    *responseCode = DCM_E_REQUESTOUTOFRANGE; // Value to use if no callback found
    Std_ReturnType retVal = E_NOT_OK;

    switch(action)
    {
    case DCM_RETURN_CONTROL_TO_ECU:
        if(DataPtr->DspDataReturnControlToEcuFnc.FuncReturnControlToECUFnc != NULL)
        {
            retVal = DataPtr->DspDataReturnControlToEcuFnc.FuncReturnControlToECUFnc(DCM_INITIAL ,responseCode);
        }
        break;
    case DCM_RESET_TO_DEFAULT:
        if(DataPtr->DspDataResetToDefaultFnc.FuncResetToDefaultFnc != NULL)
        {
            retVal = DataPtr->DspDataResetToDefaultFnc.FuncResetToDefaultFnc(DCM_INITIAL ,responseCode);
        }
        break;
    case DCM_FREEZE_CURRENT_STATE:
        if(DataPtr->DspDataFreezeCurrentStateFnc.FuncFreezeCurrentStateFnc != NULL)
        {
            retVal = DataPtr->DspDataFreezeCurrentStateFnc.FuncFreezeCurrentStateFnc(DCM_INITIAL ,responseCode);
        }
        break;
    case DCM_SHORT_TERM_ADJUSTMENT:
        if(DataPtr->DspDataShortTermAdjustmentFnc.FuncShortTermAdjustmentFnc != NULL)
        {
            retVal = DataPtr->DspDataShortTermAdjustmentFnc.FuncShortTermAdjustmentFnc(controlOptionRecord, DCM_INITIAL, responseCode);
        }
        break;
    default:
        break;
    }

    return retVal;
}


void DspIOControlByDataIdentifier(const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
    Std_ReturnType retVal = E_OK;
    uint16 didSize = 0;
    uint16 didNr = 0;
    const Dcm_DspDidType *DidPtr = NULL;
    const Dcm_DspDidControlType *DidControl = NULL;
    const Dcm_DspSignalType *signalPtr;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    boolean controlActivated = FALSE;
    Dcm_DspIOControlVector signalAffected;
    memset(signalAffected, 0 , sizeof(signalAffected));
    if(pduRxData->SduLength >= SID_LEN + IOI_LEN + IOCP_LEN) {
        didNr = (pduRxData->SduDataPtr[IOI_INDEX] << 8 & DCM_DID_HIGH_MASK) + (pduRxData->SduDataPtr[IOI_INDEX+1] & DCM_DID_LOW_MASK);
        /* @req DCM563 */
        if(TRUE == lookupNonDynamicDid(didNr, &DidPtr)) {
            DidControl = DidPtr->DspDidInfoRef->DspDidAccess.DspDidControl;
            /* @req DCM565 */
            if((NULL != DidControl) && (DidPtr->DspNofSignals != 0) && IS_VALID_IOCTRL_PARAM(pduRxData->SduDataPtr[IOCP_INDEX]))  {
                /* @req DCM566 */
                if(TRUE == DspCheckSessionLevel(DidControl->DspDidControlSessionRef)) {
                    /* @req DCM567 */
                    if(TRUE == DspCheckSecurityLevel(DidControl->DspDidControlSecurityLevelRef))  {
                        uint8 *maskPtr = NULL;
                        boolean requestOk = FALSE;
                        didSize = DidPtr->DspDidDataSize;
                        uint16 controlOptionSize = (DCM_SHORT_TERM_ADJUSTMENT == pduRxData->SduDataPtr[IOCP_INDEX]) ? DidPtr->DspDidDataSize : 0;
                        uint16 controlEnableMaskSize = (DidPtr->DspNofSignals > 1) ? ((DidPtr->DspNofSignals + 7) / 8) : 0;
                        requestOk = (pduRxData->SduLength == (SID_LEN + IOI_LEN + IOCP_LEN + controlOptionSize + controlEnableMaskSize));
                        maskPtr = (DidPtr->DspNofSignals > 1) ? (&pduRxData->SduDataPtr[COR_INDEX + controlOptionSize]) : NULL;

                        if( requestOk) {
                            /* @req DCM680 */
                            for(uint16 sigIndex = 0; (sigIndex < DidPtr->DspNofSignals) && (E_OK == retVal) && (DCM_E_POSITIVERESPONSE == responseCode); sigIndex++) {
                                signalPtr = &DidPtr->DspSignalRef[sigIndex];
                                /* @req DCM581 */
                                if( (NULL == maskPtr) || (maskPtr[sigIndex/8] & TO_SIGNAL_BIT(sigIndex)) ) {
                                    if ( signalPtr->DspSignalDataRef->DspDataUsePort == DATA_PORT_ECU_SIGNAL) {
                                        retVal = EcuSignalInputOutputControl(signalPtr->DspSignalDataRef, pduRxData->SduDataPtr[IOCP_INDEX], &pduRxData->SduDataPtr[COR_INDEX + signalPtr->DspSignalsPosition], &responseCode);
                                    }
                                    else {
                                        retVal = FunctionInputOutputControl(signalPtr->DspSignalDataRef, pduRxData->SduDataPtr[IOCP_INDEX], &pduRxData->SduDataPtr[COR_INDEX + signalPtr->DspSignalsPosition], &responseCode);
                                    }
                                    if((E_OK == retVal) && (DCM_E_POSITIVERESPONSE == responseCode)) {
                                        signalAffected[sigIndex/8] |= TO_SIGNAL_BIT(sigIndex);
                                        controlActivated = TRUE;
                                    }
                                }
                            }
                        } else {
                            responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                        }
                        if(responseCode == DCM_E_POSITIVERESPONSE && retVal != E_OK) {
                            responseCode = DCM_E_CONDITIONSNOTCORRECT;
                        }

                    } else {
                        responseCode = DCM_E_SECURITYACCESSDENIED;
                    }
                } else {
                    responseCode = DCM_E_REQUESTOUTOFRANGE;
                }
            } else {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
        } else {
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        }
    } else {
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }
    if(responseCode == DCM_E_POSITIVERESPONSE) {
        if( pduTxData->SduLength >= SID_LEN + IOI_LEN + IOCP_LEN + didSize ) {
            pduTxData->SduLength = SID_LEN + IOI_LEN + IOCP_LEN + didSize;
            // did
            pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
            pduTxData->SduDataPtr[2] = pduRxData->SduDataPtr[2];
            pduTxData->SduDataPtr[3] = pduRxData->SduDataPtr[IOCP_INDEX];
            // IMPROVEMENT: rework this totally: use the read did implementation

            if( NULL != DidPtr ) {
                /* @req DCM682 */
                for( uint16 sigIndex = 0; (sigIndex < DidPtr->DspNofSignals) && (E_OK == retVal); sigIndex++ ) {
                    signalPtr = &DidPtr->DspSignalRef[sigIndex];
                    if( signalPtr->DspSignalDataRef->DspDataReadDataFnc.SynchDataReadFnc != NULL ) {
                        if( (DATA_PORT_SYNCH == signalPtr->DspSignalDataRef->DspDataUsePort) || (DATA_PORT_ECU_SIGNAL == signalPtr->DspSignalDataRef->DspDataUsePort) ) {
                            if( E_OK != signalPtr->DspSignalDataRef->DspDataReadDataFnc.SynchDataReadFnc(&pduTxData->SduDataPtr[4 + signalPtr->DspSignalsPosition])) {
                                /* Synch port cannot return E_PENDING */
                                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                retVal = E_NOT_OK;
                            }
                        } else if(DATA_PORT_ASYNCH == signalPtr->DspSignalDataRef->DspDataUsePort) {
                            retVal = signalPtr->DspSignalDataRef->DspDataReadDataFnc.AsynchDataReadFnc(DCM_INITIAL, &pduTxData->SduDataPtr[4 + signalPtr->DspSignalsPosition]);
                        } else {
                            /* Port not supported */
                            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_CONFIG_INVALID);
                            retVal = E_NOT_OK;
                        }
                        if(E_OK != retVal) {
                            responseCode = DCM_E_CONDITIONSNOTCORRECT;
                        }
                    } else {
                        /* No read function */
                        responseCode = DCM_E_REQUESTOUTOFRANGE;
                    }
                }
            }
        } else {
            /* Tx buffer not big enough */
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        }
    }
#ifdef DCM_USE_CONTROL_DIDS
    if( DCM_E_POSITIVERESPONSE == responseCode ) {
        switch(pduRxData->SduDataPtr[IOCP_INDEX])
        {
        case DCM_RETURN_CONTROL_TO_ECU:
            DspIOControlSetActive(didNr, signalAffected, FALSE);
            break;
        case DCM_RESET_TO_DEFAULT:
        case DCM_FREEZE_CURRENT_STATE:
        case DCM_SHORT_TERM_ADJUSTMENT:
            DspIOControlSetActive(didNr, signalAffected, TRUE);
            break;
        default:
            break;
        }
    } else if( controlActivated && (DCM_RETURN_CONTROL_TO_ECU != pduRxData->SduDataPtr[IOCP_INDEX])) {
        /* Will give negative response. Disable any control which was activated by this request */
        DspIOControlStopNonActive(didNr, signalAffected);
    }
#endif
    DsdDspProcessingDone(responseCode);
}
#endif

#ifdef DCM_USE_SERVICE_COMMUNICATIONCONTROL
void DspCommunicationControl(const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_REQUESTOUTOFRANGE;
    uint8 subFunction = pduRxData->SduDataPtr[SF_INDEX];
    if(pduRxData->SduLength == 3) {
        if( !IS_ISO_RESERVED(subFunction) ) {
            Dcm_Arc_CommunicationControl(subFunction, pduRxData->SduDataPtr[CC_CTP_INDEX], &responseCode);
            /* Check the response code to make sure that the callout did not set it
             * to something invalid.
             * Valid response codes positiveResponse, conditionsNotCorrect
             * subFunctionNotSupported and requestOutOfRange */
            if( !((DCM_E_POSITIVERESPONSE == responseCode) ||
                    (DCM_E_REQUESTOUTOFRANGE == responseCode) ||
                    (DCM_E_CONDITIONSNOTCORRECT == responseCode) ||
                    (DCM_E_SUBFUNCTIONNOTSUPPORTED == responseCode)) ) {
                /* Response invalid. Override it and report to Det. */
                DCM_DET_REPORTERROR(DCM_UDS_COMMUNICATION_CONTROL_ID, DCM_E_UNEXPECTED_RESPONSE);
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            } else {
                /* Valid response. */
            }
        } else {
            /* ISO reserved for future definition */
            responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
        }
    } else {
        /* Length not correct */
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }
    if(responseCode == DCM_E_POSITIVERESPONSE)
    {
        pduTxData->SduLength = SID_LEN + SF_LEN;
        pduTxData->SduDataPtr[SF_INDEX] = pduRxData->SduDataPtr[SF_INDEX];
    }
    DsdDspProcessingDone(responseCode);
}
#endif

#if defined(DCM_USE_SERVICE_REQUESTCURRENTPOWERTRAINDIAGNOSTICDATA) || defined(DCM_USE_SERVICE_REQUESTPOWERTRAINFREEZEFRAMEDATA)
static boolean lookupPid(uint8 pidId, Dcm_PidServiceType service, const Dcm_DspPidType **PidPtr)
{
    boolean pidFound = FALSE;
    const Dcm_DspPidType *dspPid = Dcm_ConfigPtr->Dsp->DspPid;

    if(dspPid != NULL)
    {
        while ((dspPid->DspPidIdentifier != pidId) && (!dspPid->Arc_EOL))
        {
            dspPid++;
        }
        if ((!dspPid->Arc_EOL) && ((DCM_SERVICE_01_02 == dspPid->DspPidService) || (service == dspPid->DspPidService) ))
        {
            pidFound = TRUE;
            *PidPtr = dspPid;
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
        /*do nothing*/
    }

    return pidFound;
}
#endif

#ifdef DCM_USE_SERVICE_REQUESTCURRENTPOWERTRAINDIAGNOSTICDATA
static boolean setAvailabilityPidValue(uint8 Pid, Dcm_PidServiceType service, uint32 *Data)
{
    uint8 shift;
    uint32 pidData = 0;
    uint32 temp;
    boolean setOk = TRUE;
    const Dcm_DspPidType *dspPid = Dcm_ConfigPtr->Dsp->DspPid;

    if(dspPid != NULL) {
        while (0 == dspPid->Arc_EOL) {
            if( (DCM_SERVICE_01_02 == dspPid->DspPidService) || (service == dspPid->DspPidService) ) {
                if((dspPid->DspPidIdentifier >= (Pid + AVAIL_TO_SUPPORTED_PID_OFFSET_MIN)) && (dspPid->DspPidIdentifier <= (Pid + AVAIL_TO_SUPPORTED_PID_OFFSET_MAX))) {
                    shift = dspPid->DspPidIdentifier - Pid;
                    temp = (uint32)1 << (AVAIL_TO_SUPPORTED_PID_OFFSET_MAX - shift);
                    pidData |= temp;
                } else if(dspPid->DspPidIdentifier > (Pid + AVAIL_TO_SUPPORTED_PID_OFFSET_MAX)) {
                    pidData |= (uint32)1;
                } else {
                    /*do nothing*/
                }
            }
            dspPid++;
        }
    } else {
        setOk = FALSE;
    }

    if(0 == pidData) {
        setOk = FALSE;
    } else {
        /*do nothing*/
    }
    (*Data) = pidData;
    
    return setOk;
}

/*@req OBD_DCM_REQ_2*//* @req OBD_REQ_1 */
void DspObdRequestCurrentPowertrainDiagnosticData(const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
	uint16 i ;
	uint16 nofAvailabilityPids = 0;
	uint16 findPid = 0;
	uint16 txPos = SID_LEN;
	uint32 DATA = 0;
	uint8 txBuffer[255] = {0};
	uint16 txLength = SID_LEN;
	uint8 requestPid[MAX_REQUEST_PID_NUM];
	uint16 pidNum = pduRxData->SduLength - SID_LEN;
	const Dcm_DspPidType *sourcePidPtr = NULL;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	/* @req OBD_REQ_3 */
	if((pduRxData->SduLength >= OBD_REQ_MESSAGE_LEN_ONE_MIN) && (pduRxData->SduLength <= OBD_REQ_MESSAGE_LEN_MAX))
	{
		i = pidNum;
		while(i > 0)
		{
			requestPid[i - 1] = pduRxData->SduDataPtr[i];
			if( IS_AVAILABILITY_PID(requestPid[i - 1]) )
			{
			    nofAvailabilityPids++; /*used to judge if the message is valid, if nofAvailabilityPids != 0 or Pidnum, invalid*/
			}
			else
			{
				/*do nothing*/
			}
			i--;
		}
		for(i = 0;i < pidNum;i++)                                        /*figure out the txLength to be sent*/
		{
			/*situation of supported Pids*/
			if(TRUE == lookupPid(requestPid[i], DCM_SERVICE_01, &sourcePidPtr))
			{
				txLength += PID_LEN + sourcePidPtr->DspPidSize;
			}
			/*situation of availability Pids*/
			else if( IS_AVAILABILITY_PID(requestPid[i]) )
			{
				txLength += PID_LEN + SUPPRTED_PIDS_DATA_LEN;
			}
			else 
			{
				/*do nothing*/
			}
		}
		/*@req OBD_DCM_REQ_7*/
		if(txLength <= pduTxData->SduLength)			 	/*if txLength is smaller than the configured length*/
		{
			if(pidNum == nofAvailabilityPids) /*check if all the request PIDs are the 0x00...0xE0 format*/
			{
				for(i = 0;i < pidNum;i++)		/*Check the PID configuration,find which PIDs were configured for 0x00,0x20,0x40 respectively,and fill in the pduTxBuffer,and count the txLength*/
				{
					/*@OBD_DCM_REQ_3,@OBD_DCM_REQ_6*/
					if(TRUE == setAvailabilityPidValue(requestPid[i], DCM_SERVICE_01, &DATA))
					{						
						pduTxData->SduDataPtr[txPos++] = requestPid[i];
						/*take every byte of uint32 DATA,and fill in txbuffer*/
						pduTxData->SduDataPtr[txPos++] = (uint8)(((DATA) & (OBD_DATA_LSB_MASK << OFFSET_THREE_BYTES)) >> OFFSET_THREE_BYTES);
						pduTxData->SduDataPtr[txPos++] = (uint8)(((DATA) & (OBD_DATA_LSB_MASK << OFFSET_TWO_BYTES)) >> OFFSET_TWO_BYTES);
						pduTxData->SduDataPtr[txPos++] = (uint8)(((DATA) & (OBD_DATA_LSB_MASK << OFFSET_ONE_BYTE)) >> OFFSET_ONE_BYTE);
						pduTxData->SduDataPtr[txPos++] = (uint8)((DATA) & OBD_DATA_LSB_MASK);
					}
					else if(PIDZERO == requestPid[i])
					{
						pduTxData->SduDataPtr[txPos++] = requestPid[i];
						pduTxData->SduDataPtr[txPos++] = DATAZERO;
						pduTxData->SduDataPtr[txPos++] = DATAZERO;
						pduTxData->SduDataPtr[txPos++] = DATAZERO;
						pduTxData->SduDataPtr[txPos++] = DATAZERO;
					}
					else
					{
						findPid++;
					}
				}
			}
			else if(0 == nofAvailabilityPids) /*check if all the request PIDs are the supported PIDs,like 0x01,0x02...*/
			{
				for(i = 0;i < pidNum;i++)
				{
					if(TRUE == lookupPid(requestPid[i], DCM_SERVICE_01, &sourcePidPtr))
					{
						/*@req OBD_DCM_REQ_3,OBD_DCM_REQ_5,OBD_DCM_REQ_8*//* @req OBD_REQ_2 */
						if(E_OK == sourcePidPtr->DspGetPidValFnc(txBuffer))
						{
							pduTxData->SduDataPtr[txPos] = requestPid[i];
							txPos++;
							memcpy(&(pduTxData->SduDataPtr[txPos]),txBuffer,sourcePidPtr->DspPidSize);
							txPos += sourcePidPtr->DspPidSize;
						}
						else
						{
							responseCode = DCM_E_CONDITIONSNOTCORRECT;
							break;
						}

					}
					else
					{
						findPid++;
					}
				}
			}
			else
			{
				responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
			}
			if(pidNum == findPid)
			{
				responseCode = DCM_E_REQUESTOUTOFRANGE;
			}
			else
			{
				/*do nothing*/
			}
		}
		else
		{
			responseCode = DCM_E_REQUESTOUTOFRANGE;
		}
	}
	else
	{
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}
	if(DCM_E_POSITIVERESPONSE == responseCode)
	{
		pduTxData->SduLength = txPos;
	}
	else
	{
		/*do nothing*/
	}
	DsdDspProcessingDone(responseCode);

	return;
}
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_REQUESTPOWERTRAINFREEZEFRAMEDATA)

/*@req OBD_DCM_REQ_9*/
void DspObdRequestPowertrainFreezeFrameData(const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
    uint16 i ;
    uint16 j = 0;
    uint16 nofAvailabilityPids = 0;
    uint16 findPid = 0;
    uint32 dtc = 0;
    uint32 supportBitfield = 0;
    uint16 txPos = SID_LEN;
    uint16 txLength = SID_LEN;
    uint8 requestPid[DCM_MAX_PID_NUM_IN_FF];
    uint8 requestFFNum[DCM_MAX_PID_NUM_IN_FF];
    uint16 messageLen = pduRxData->SduLength;
    const Dcm_DspPidType *sourcePidPtr = NULL;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

    /* @req OBD_REQ_6 */
    if((messageLen >= OBD_REQ_MESSAGE_LEN_TWO_MIN ) && (messageLen <= OBD_REQ_MESSAGE_LEN_MAX ) && (((messageLen - 1) % 2) == 0))
    {
        uint16 pidNum = ((messageLen - 1) >> 1);

        /*find out PID and FFnum*/
        for(i = 0;i < pidNum;i++)
        {
            requestPid[i] = pduRxData->SduDataPtr[j + 1];
            if( IS_AVAILABILITY_PID(requestPid[i]) )
            {
                nofAvailabilityPids++; /*used to judge if the message is valid, if nofAvailabilityPids != 0 or Pidnum, invalid*/
            }
            requestFFNum[i] = pduRxData->SduDataPtr[j + 2];
            j += 2;
        }
        /*count txLength*/
        for(i = 0;i < pidNum;i++)
        {
            if(requestPid[i] == OBD_SERVICE_TWO)
            {
                txLength += PID_LEN + FF_NUM_LEN + OBD_DTC_LEN;
            }
            else if( IS_AVAILABILITY_PID(requestPid[i]) )
            {
                txLength += PID_LEN + SUPPRTED_PIDS_DATA_LEN;
            }
            else if(TRUE == lookupPid(requestPid[i], DCM_SERVICE_02, &sourcePidPtr))
            {
                txLength += PID_LEN + FF_NUM_LEN + sourcePidPtr->DspPidSize;
            }
            else
            {
                /*do nothing*/
            }
        }
        /*@req OBD_DCM_REQ_7*/
        if(txLength <= (pduTxData->SduLength))
        {
            if(pidNum == nofAvailabilityPids) /*check if all the request PIDs are the 0x00...0xE0 format*/
            {
                for(i = 0;i < pidNum;i++)		/*Check the PID configuration,find which PIDs were configured for 0x00,0x20,0x40 respectively,and fill in the pduTxBuffer,and count the txLength*/
                {
                    if(requestFFNum[i] == RECORD_NUM)
                    {
                        if(TRUE == setAvailabilityPidValue(requestPid[i], DCM_SERVICE_02, &supportBitfield)) {
                            pduTxData->SduDataPtr[txPos++] = requestPid[i];
                            pduTxData->SduDataPtr[txPos++] = requestFFNum[i];
                            /*take every byte of uint32 DATA,and fill in txbuffer*/
                            pduTxData->SduDataPtr[txPos++] = (uint8)(((supportBitfield) & (OBD_DATA_LSB_MASK << OFFSET_THREE_BYTES)) >> OFFSET_THREE_BYTES);
                            pduTxData->SduDataPtr[txPos++] = (uint8)(((supportBitfield) & (OBD_DATA_LSB_MASK << OFFSET_TWO_BYTES)) >> OFFSET_TWO_BYTES);
                            pduTxData->SduDataPtr[txPos++] = (uint8)(((supportBitfield) & (OBD_DATA_LSB_MASK << OFFSET_ONE_BYTE)) >> OFFSET_ONE_BYTE);
                            pduTxData->SduDataPtr[txPos++] = (uint8)((supportBitfield) & OBD_DATA_LSB_MASK);
                        } else if(PIDZERO == requestPid[i]) {
                            pduTxData->SduDataPtr[txPos++] = requestPid[i];
                            pduTxData->SduDataPtr[txPos++] = requestFFNum[i];
                            pduTxData->SduDataPtr[txPos++] = DATAZERO;
                            pduTxData->SduDataPtr[txPos++] = DATAZERO;
                            pduTxData->SduDataPtr[txPos++] = DATAZERO;
                            pduTxData->SduDataPtr[txPos++] = DATAZERO;
                        } else {
                            findPid++;
                        }
                    }
                    else
                    {
                        /*@req OBD_DCM_REQ_11*/
                        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                        break;
                    }

                }
            }
            else if(0 == nofAvailabilityPids) /*check if all the request PIDs are the supported PIDs,like 0x01,0x02...*/
            {
                for(i = 0;i < pidNum;i++)
                {
                    /*@req OBD_DCM_REQ_10*/
                    if(requestFFNum[i] == RECORD_NUM)
                    {
                        uint8 bufSize = 0;
                        if(requestPid[i] == OBD_SERVICE_TWO)
                        {
                            /*@req OBD_DCM_REQ_12,@OBD_DCM_REQ_13,@OBD_DCM_REQ_14*/
                            if(E_OK == Dem_GetDTCOfOBDFreezeFrame(requestFFNum[i],&dtc))
                            {
                                pduTxData->SduDataPtr[txPos++] = requestPid[i];
                                pduTxData->SduDataPtr[txPos++] = requestFFNum[i];
                                pduTxData->SduDataPtr[txPos++] = (uint8)(((dtc) & (OBD_DATA_LSB_MASK << OFFSET_ONE_BYTE)) >> OFFSET_ONE_BYTE);
                                pduTxData->SduDataPtr[txPos++] = (uint8)((dtc) & OBD_DATA_LSB_MASK);
                            }
                            /*if the DTC did not cause the stored FF,DTC of 0x0000 should be returned*/
                            /* @req OBD_REQ_5 */
                            else
                            {
                                pduTxData->SduDataPtr[txPos++] = requestPid[i];
                                pduTxData->SduDataPtr[txPos++] = requestFFNum[i];
                                pduTxData->SduDataPtr[txPos++] = 0x00;
                                pduTxData->SduDataPtr[txPos++] = 0x00;
                            }
                        }
                        /*req OBD_DCM_REQ_17*/
                        else
                        {
                            /*@req OBD_DCM_REQ_28*/
                            pduTxData->SduDataPtr[txPos++] = requestPid[i];
                            pduTxData->SduDataPtr[txPos++] = requestFFNum[i];
                            bufSize = (uint8)(pduTxData->SduLength - txPos);
                            /*@req OBD_DCM_REQ_15,OBD_DCM_REQ_16*//* @req OBD_REQ_4 */
                            /* IMPROVEMENT: Dem_GetOBDFreezeFrameData should be called for each data element in the
                             * Pid. Parameter DataElementIndexOfPid should be the index of the data element
                             * within the Pid. But currently only one data element per Pid is supported. */
                            if(E_OK == Dem_ReadDataOfOBDFreezeFrame(requestPid[i], DATA_ELEMENT_INDEX_OF_PID_NOT_SUPPORTED, &(pduTxData->SduDataPtr[txPos]), &bufSize))
                            {
                                txPos += bufSize;
                            }
                            else
                            {
                                txPos -= 2;
                                findPid++;
                            }
                        }
                    }
                    else
                    {
                        /*@req OBD_DCM_REQ_11*/
                        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                        break;
                    }
                }

            }
            else
            {
                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
            }
            if(pidNum == findPid)
            {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
            }
            else
            {
                /*do nothing*/
            }
        }
        else
        {
            responseCode = DCM_E_REQUESTOUTOFRANGE;
        }
    }
    else
    {
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }
    if(DCM_E_POSITIVERESPONSE == responseCode)
    {
        pduTxData->SduLength = txPos;
    }
    else
    {
        /*do nothing*/
    }
    DsdDspProcessingDone(responseCode);

    return;
}
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_CLEAREMISSIONRELATEDDIAGNOSTICDATA)
static boolean lookupService(uint8 serviceId,const Dcm_DsdServiceType **dsdService)
{
	boolean serviceFind = FALSE;
	const Dcm_DsdServiceTableType *ServiceTable = Dcm_ConfigPtr->Dsd->DsdServiceTable;
	const Dcm_DsdServiceType *dsdServicePtr = NULL;

	if(ServiceTable != NULL)
	{
		while(0 == ServiceTable->Arc_EOL)
		{
			if(ServiceTable->DsdService != NULL)
			{
				dsdServicePtr = ServiceTable->DsdService;
				while((serviceId != dsdServicePtr->DsdSidTabServiceId) && (0 == dsdServicePtr->Arc_EOL))
				{
					dsdServicePtr++;
				}
				if((serviceId == dsdServicePtr->DsdSidTabServiceId) && (0 == dsdServicePtr->Arc_EOL))
				{
					*dsdService = dsdServicePtr;
					serviceFind = TRUE;
				}
			}

			ServiceTable++;
		}
	}

	return serviceFind;
}

/*@req OBD_DCM_REQ_19*//* @req OBD_REQ_10 */
void DspObdClearEmissionRelatedDiagnosticData(const PduInfoType *pduRxData,PduInfoType *pduTxData)
{
	uint16 messageLen = pduRxData->SduLength;
	const Dcm_DsdServiceType *dsdService = NULL;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

	if(messageLen == SID_LEN )
	{
		if(TRUE == lookupService(OBD_SERVICE_FOUR, &dsdService))
		{
			if(dsdService->ArcDcmDsdSidConditionCheckFnc != NULL)
			{
				/* @req OBD_REQ_11 */
				if(E_OK == dsdService->ArcDcmDsdSidConditionCheckFnc())
				{
					/*@req OBD_DCM_REQ_1,OBD_DCM_REQ_20,OBD_DCM_REQ_21*/
					if(DEM_CLEAR_OK == Dem_ClearDTC(DEM_DTC_GROUP_ALL_DTCS, DEM_DTC_KIND_EMISSION_REL_DTCS, DEM_DTC_ORIGIN_PRIMARY_MEMORY))
					{
						/*do nothing*/
					}
					else
					{
						responseCode = DCM_E_CONDITIONSNOTCORRECT;
					}
				}
				else
				{
					responseCode = DCM_E_CONDITIONSNOTCORRECT;
				}

			}
			if(dsdService->resetPids != NULL)
			{
				if(E_OK != dsdService->resetPids())
				{
					responseCode = DCM_E_CONDITIONSNOTCORRECT;
				}
			}
		}
	}
	else
	{
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

	/*@req OBD_DCM_REQ_22*/
	if(DCM_E_POSITIVERESPONSE == responseCode)
	{
		pduTxData->SduLength = SID_LEN;
	}
	else
	{
		/*do nothing*/
	}
	DsdDspProcessingDone(responseCode);

	return;
}
#endif

#if defined(USE_DEM)
#if defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDIAGNOSTICTROUBLECODES) || defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDTCSDETECTEDDURINGCURRENTORLASTCOMPLETEDDRIVINGCYCLE)
static Dcm_NegativeResponseCodeType OBD_Sevice_03_07(PduInfoType *pduTxData, Dem_ReturnSetFilterType setDtcFilterResult)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

    if (setDtcFilterResult == DEM_FILTER_ACCEPTED)
    {
        uint32 dtc;
        Dem_EventStatusExtendedType dtcStatus;
        uint8 nrOfDtcs = 0;
        uint8 indx = 2;

        while ((Dem_GetNextFilteredDTC(&dtc, &dtcStatus)) == DEM_FILTERED_OK)
        {

            if((indx + LENGTH_OF_DTC) >= (pduTxData->SduLength))
            {
                responseCode = DCM_E_REQUESTOUTOFRANGE;
                break;
            }
            /* @req OBD_REQ_9 */
            pduTxData->SduDataPtr[indx] = (uint8)EMISSION_DTCS_HIGH_BYTE(dtc);
            pduTxData->SduDataPtr[1+indx] = (uint8)EMISSION_DTCS_LOW_BYTE(dtc);
            indx += LENGTH_OF_DTC;
            nrOfDtcs++;

        }
        /* @req OBD_REQ_8 */
        if(responseCode == DCM_E_POSITIVERESPONSE)
        {
            pduTxData->SduLength = (PduLengthType)(indx);
            pduTxData->SduDataPtr[1] = nrOfDtcs;
        }

    }
    else
    {
        responseCode = DCM_E_CONDITIONSNOTCORRECT;
    }

    return responseCode;

}
#endif
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDIAGNOSTICTROUBLECODES)
/*@req OBD_DCM_REQ_23*//* @req OBD_REQ_7 */
void  DspObdRequestEmissionRelatedDiagnosticTroubleCodes(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	uint16 messageLen = pduRxData->SduLength;
	Dem_ReturnSetFilterType setDtcFilterResult;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

	if(messageLen == SID_LEN )
	{
		/*"confirmed" diagnostic trouble codes*/
		/*@req OBD_DCM_REQ_1*/	/*@req OBD_DCM_REQ_24*/
		setDtcFilterResult = Dem_SetDTCFilter(DEM_CONFIRMED_DTC, DEM_DTC_KIND_EMISSION_REL_DTCS, DEM_DTC_FORMAT_OBD,
											DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO,
											VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);

		responseCode = OBD_Sevice_03_07(pduTxData,setDtcFilterResult);

	}
	else
	{
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

	DsdDspProcessingDone(responseCode);

}
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDTCSDETECTEDDURINGCURRENTORLASTCOMPLETEDDRIVINGCYCLE)
/*@req OBD_DCM_REQ_25*//* @req OBD_REQ_12 */
void  DspObdRequestEmissionRelatedDiagnosticTroubleCodesService07(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	uint16 messageLen = pduRxData->SduLength;
	Dem_ReturnSetFilterType setDtcFilterResult;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

	if(messageLen == SID_LEN )
	{
		/*"pending" diagnostic trouble codes*/
		/*@req OBD_DCM_REQ_1*/	/*@req OBD_DCM_REQ_26*/
		setDtcFilterResult = Dem_SetDTCFilter(DEM_PENDING_DTC, DEM_DTC_KIND_EMISSION_REL_DTCS, DEM_DTC_FORMAT_OBD,
											DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO,
											VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);

		responseCode = OBD_Sevice_03_07(pduTxData,setDtcFilterResult);

	}
	else
	{
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

  DsdDspProcessingDone(responseCode);
  
  return;

}
#endif

#ifdef DCM_USE_SERVICE_REQUESTVEHICLEINFORMATION
static boolean lookupInfoType(uint8 InfoType, const Dcm_DspVehInfoType **InfoTypePtr)
{
	const Dcm_DspVehInfoType *dspVehInfo = Dcm_ConfigPtr->Dsp->DspVehInfo;
	boolean InfoTypeFound = FALSE;

	while ((dspVehInfo->DspVehInfoType != InfoType) && ((dspVehInfo->Arc_EOL) == FALSE))
	{
		dspVehInfo++;
	}
	if ((dspVehInfo->Arc_EOL) == FALSE)
	{
		InfoTypeFound = TRUE;
		*InfoTypePtr = dspVehInfo;
	}

	return InfoTypeFound;
}

static boolean setAvailabilityInfoTypeValue(uint8 InfoType, uint32 *DATABUF)
{
	uint8 shift;
	uint32 databuf = 0;
	uint32 temp;
	boolean setInfoTypeOk = TRUE;
	const Dcm_DspVehInfoType *dspVehInfo = Dcm_ConfigPtr->Dsp->DspVehInfo;

    if(dspVehInfo != NULL)
    {
        while ((dspVehInfo->DspVehInfoType != FALSE) &&  ((dspVehInfo->Arc_EOL) == FALSE))
        {
            if((dspVehInfo->DspVehInfoType >= (InfoType + AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MIN)) && (dspVehInfo->DspVehInfoType <= (InfoType + AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MAX)))
            {
                shift = dspVehInfo->DspVehInfoType - InfoType;
                temp = (uint32)1 << (AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MAX - shift);
                databuf |= temp;
            }
            else if( dspVehInfo->DspVehInfoType > (InfoType + AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MAX))
            {
                databuf |= (uint32)0x01;
            }
            else
            {
                /*do nothing*/
            }
            dspVehInfo++;
        }

        if(databuf == 0)
        {
            setInfoTypeOk = FALSE;
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
        setInfoTypeOk = FALSE;
    }

    (*DATABUF) = databuf;

    return setInfoTypeOk;

}

/*@req OBD_DCM_REQ_27*//*@req OBD_REQ_13*/
void DspObdRequestVehicleInformation(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	uint8 i ;
	uint8 nofAvailabilityInfoTypes = 0;
	uint8 requestInfoType[MAX_REQUEST_VEHINFO_NUM];
	uint16 txPos = SID_LEN;
	uint32 DATABUF;
	uint8 txBuffer[PID_BUFFER_SIZE];
	uint8 findNum = 0;
	uint16 InfoTypeNum = pduRxData->SduLength - 1;
	const Dcm_DspVehInfoType *sourceVehInfoPtr = NULL;
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

	/*@req OBD_REQ_14*/
	if((pduRxData->SduLength >= OBD_REQ_MESSAGE_LEN_ONE_MIN) && (pduRxData->SduLength <= OBD_REQ_MESSAGE_LEN_MAX ))
	{
		i = (uint8)InfoTypeNum;
		while(i > 0)
		{
			requestInfoType[i - 1] = pduRxData->SduDataPtr[i];
			if( IS_AVAILABILITY_INFO_TYPE(requestInfoType[i - 1]) )
			{
			    nofAvailabilityInfoTypes++;
			}
			else
			{
			}
			i--;
		 }

		/*@req OBD_DCM_REQ_29*/
		if(InfoTypeNum == nofAvailabilityInfoTypes)	/*check if all the request PIDs are the 0x00...0xE0 format*/
		{
			for(i = 0;i < InfoTypeNum;i++)		/*Check the PID configuration,find which PIDs were configured for 0x00,0x20,0x40 respectively,and fill in the pduTxBuffer,and count the txLength*/
			{
				if(TRUE == setAvailabilityInfoTypeValue(requestInfoType[i], &DATABUF))
				{
					pduTxData->SduDataPtr[txPos++] = requestInfoType[i];
					/*take every byte of uint32 DTC,and fill in txbuffer*/
					pduTxData->SduDataPtr[txPos++] = (uint8)((DATABUF & (OBD_DATA_LSB_MASK << OFFSET_THREE_BYTES)) >> OFFSET_THREE_BYTES);
					pduTxData->SduDataPtr[txPos++] = (uint8)((DATABUF & (OBD_DATA_LSB_MASK << OFFSET_TWO_BYTES)) >> OFFSET_TWO_BYTES);
					pduTxData->SduDataPtr[txPos++] = (uint8)((DATABUF & (OBD_DATA_LSB_MASK << OFFSET_ONE_BYTE)) >> OFFSET_ONE_BYTE);
					pduTxData->SduDataPtr[txPos++] = (uint8)(DATABUF & OBD_DATA_LSB_MASK);
				}
				else if(INFOTYPE_ZERO == requestInfoType[i])
				{
					pduTxData->SduDataPtr[txPos++] = requestInfoType[i];
					pduTxData->SduDataPtr[txPos++] = DATAZERO;
					pduTxData->SduDataPtr[txPos++] = DATAZERO;
					pduTxData->SduDataPtr[txPos++] = DATAZERO;
					pduTxData->SduDataPtr[txPos++] = DATAZERO;
				}
				else
				{
					findNum++;
				}
			}
		}
		/*@req OBD_DCM_REQ_28*/
		else if(nofAvailabilityInfoTypes == 0) /*check if all the request PIDs are the supported VINs,like 0x01,0x02...*/
		{
			/*@req OBD_REQ_15*/
			if(pduRxData->SduLength == OBD_REQ_MESSAGE_LEN_ONE_MIN)
			{
				if(TRUE == lookupInfoType(requestInfoType[i], &sourceVehInfoPtr ))
				{

					if (sourceVehInfoPtr->DspGetVehInfoTypeFnc(txBuffer) != E_OK) 
					{
						if( requestInfoType[i] == 0x02 )	/* Special for read VIN fail,  customer's requirement*/
						{
							for(uint8 j = 0; j < (OBD_VIN_LENGTH*sourceVehInfoPtr->DspVehInfoNumberOfDataItems);j++)
							{
								txBuffer[j] = 0xff;
							}
						}
						else
						{
							responseCode = DCM_E_CONDITIONSNOTCORRECT;
						}
					}

					pduTxData->SduDataPtr[txPos++] = requestInfoType[i];

					/*@req OBD_DCM_REQ_30*/
					pduTxData->SduDataPtr[txPos++] = sourceVehInfoPtr->DspVehInfoNumberOfDataItems;
					memcpy(&(pduTxData->SduDataPtr[txPos]), txBuffer, (sourceVehInfoPtr->DspVehInfoSize * (sourceVehInfoPtr->DspVehInfoNumberOfDataItems)));

					txPos += (sourceVehInfoPtr->DspVehInfoSize * (sourceVehInfoPtr->DspVehInfoNumberOfDataItems)) ;
					if(txPos >= ((pduTxData->SduLength)))
					{
						responseCode = DCM_E_REQUESTOUTOFRANGE;
					}
				}
				else
				{
					findNum++;
				}
			}
			/*@req OBD_REQ_16*/
			else
			{
				responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
			}
	 	}
		else
		{
			responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
		}

		if(findNum == InfoTypeNum)
		{
			responseCode = DCM_E_REQUESTOUTOFRANGE;
		}
		else
		{
			/* do nothing */
		}
	}
	else
	{
		responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
	}

	if(DCM_E_POSITIVERESPONSE == responseCode)
	{
		pduTxData->SduLength = txPos;
	}
	else
	{
		/* do nothing */
	}

	DsdDspProcessingDone(responseCode);

}
#endif

uint32 DspRoutineInfoReadUnsigned(uint8 *data, uint16 bitOffset, uint8 size, boolean changeEndian) {
	uint32 retVal = 0;
	const uint16 little_endian = 0x1;
    if(size > 32) {
        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_CONFIG_INVALID);
        return 0;
    }
    if((uint8)changeEndian ^ *((uint8*)&little_endian)) {
        // read little endian
        for(int i = 0; i < size / 8; i++) {
            retVal = (retVal << 8) | (data+bitOffset/8 + size/8 - 1)[-i];
        }
    } else {
        // read big endian
        for(int i = 0; i < size / 8; i++) {
            retVal = (retVal << 8) | (data+bitOffset/8)[i];
        }
    }
	return retVal;
}
sint32 DspRoutineInfoRead(uint8 *data, uint16 bitOffset, uint8 size, boolean changeEndian) {
    uint32 retVal = DspRoutineInfoReadUnsigned(data, bitOffset, size, changeEndian);
    uint32 mask = 0xFFFFFFFF << (size - 1);
    if(retVal & mask) {
        // result is negative
        retVal &= mask;
    }
    return (sint32)retVal;
}
void DspRoutineInfoWrite(uint32 val, uint8 *data, uint16 bitOffset, uint8 size, boolean changeEndian) {
    const uint16 little_endian = 0x1;
    if((uint8)changeEndian ^ *((uint8*)&little_endian)) {
        // write little endian
        for(int i = 0; i < size / 8; i++) {
            (data+bitOffset/8)[i] = 0xFF & val;
            val = val >> 8;
        }
    } else {
        for(int i = 0; i < size / 8; i++) {
            (data+(bitOffset + size)/8 - 1)[-i] = 0xFF & val;
            val = val >> 8;
        }
    }
}

#define UDS_0x34_MAX_NUM_BLOCK_LEN   (1440 + 2)
#define UDS_0x35_MAX_NUM_BLOCK_LEN    (128 + 2)

void DspUdsRequestDownload(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	//lint +estring(920,pointer)  /* cast to void */
	// Prepare positive response message
    pduTxData->SduLength = 4;   //SID + LengthFormatIdentifier + maxNumberOfBlockLength
    pduTxData->SduDataPtr[1] = 0x20;    //currently we set maxNumberOfBlock as 4090(0x0FFA), so it takes 2 bytes
    pduTxData->SduDataPtr[2] = (uint8)(UDS_0x34_MAX_NUM_BLOCK_LEN >> 8);
    pduTxData->SduDataPtr[3] = (uint8)(UDS_0x34_MAX_NUM_BLOCK_LEN & 0xFF);

    DsdDspProcessingDone(responseCode);
}

void DspUdsRequestUpload(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
	//lint -estring(920,pointer)  /* cast to void */
	(void)pduRxData;
	//lint +estring(920,pointer)  /* cast to void */
	// Prepare positive response message
    pduTxData->SduLength = 3;   //SID + LengthFormatIdentifier + maxNumberOfBlockLength
    pduTxData->SduDataPtr[1] = 0x10;    //currently we set maxNumberOfBlock as 130(0x82), so it takes 1 bytes
    pduTxData->SduDataPtr[2] = (uint8)UDS_0x35_MAX_NUM_BLOCK_LEN;

    DsdDspProcessingDone(responseCode);
}

void DspUdsTransferData(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    //lint -estring(920,pointer)  /* cast to void */
    (void)pduRxData;
    //lint +estring(920,pointer)  /* cast to void */
	pduTxData->SduLength = (uint8)UDS_0x35_MAX_NUM_BLOCK_LEN;
    pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];

    DsdDspProcessingDone(responseCode);
}

void DspUdsRequestTransferExit(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
	Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    //lint -estring(920,pointer)  /* cast to void */
    (void)pduRxData;
    //lint +estring(920,pointer)  /* cast to void */
    // Prepare positive response
    pduTxData->SduLength = 2;
    pduTxData->SduDataPtr[1] = 0x00;    //transferResponseParameterRecore, user-defined

    DsdDspProcessingDone(responseCode);
}
