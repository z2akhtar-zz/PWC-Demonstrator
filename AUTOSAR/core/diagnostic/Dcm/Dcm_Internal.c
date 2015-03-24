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

#include <string.h>
#include "Dcm.h"
#include "Dcm_Internal.h"
#include "MemMap.h"
#ifndef DCM_NOT_SERVICE_COMPONENT
#include "Rte_Dcm.h"
#endif
#if defined(USE_NVM)
#include "NvM.h"
#endif

/* Disable MISRA 2004 rule 16.2, MISRA 2012 rule 17.2.
 * This because of recursive calls to readDidData.
 *  */
//lint -estring(974,*recursive*)


boolean lookupNonDynamicDid(uint16 didNr, const Dcm_DspDidType **didPtr)
{
    const Dcm_DspDidType *dspDid = Dcm_ConfigPtr->Dsp->DspDid;
    boolean didFound = FALSE;

    while ((dspDid->DspDidIdentifier != didNr) &&  (!dspDid->Arc_EOL)) {
        dspDid++;
    }

    /* @req Dcm651 */
    if (!dspDid->Arc_EOL && (!(dspDid->DspDidInfoRef->DspDidDynamicllyDefined && DID_IS_IN_DYNAMIC_RANGE(didNr)) ) ) {
        didFound = TRUE;
        *didPtr = dspDid;
    }

    return didFound;
}

boolean lookupDynamicDid(uint16 didNr, const Dcm_DspDidType **didPtr)
{
    const Dcm_DspDidType *dspDid = Dcm_ConfigPtr->Dsp->DspDid;
    boolean didFound = FALSE;

    if( DID_IS_IN_DYNAMIC_RANGE(didNr) ) {
        while ((dspDid->DspDidIdentifier != didNr) &&  (!dspDid->Arc_EOL)) {
            dspDid++;
        }

        /* @req Dcm651 */
        if (!dspDid->Arc_EOL && dspDid->DspDidInfoRef->DspDidDynamicllyDefined) {
            didFound = TRUE;
            *didPtr = dspDid;
        }
    }

    return didFound;
}

Dcm_NegativeResponseCodeType readDidData(const Dcm_DspDidType *didPtr, PduInfoType *pduTxData, uint16 *txPos,
        ReadDidPendingStateType *pendingState, uint16 *pendingDid, uint16 *pendingSignalIndex, uint16 *pendingDataLen, uint16 *didIndex, uint16 didStartIndex, uint16 *didDataStartPos)
{
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

    boolean didOnlyRefsDids = ((NULL == didPtr->DspSignalRef));
    if (didPtr->DspDidInfoRef->DspDidAccess.DspDidRead != NULL) {  /** @req DCM433 */
        if (DspCheckSessionLevel(didPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSessionRef)) { /** @req DCM434 */
            if (DspCheckSecurityLevel(didPtr->DspDidInfoRef->DspDidAccess.DspDidRead->DspDidReadSecurityLevelRef)) { /** @req DCM435 */
                if( !didOnlyRefsDids && (*didIndex >= didStartIndex)) {
                    /* Get the data if available */
                    responseCode = getDidData(didPtr, pduTxData, txPos, pendingState, pendingDataLen, pendingSignalIndex, didDataStartPos);
                    if(DCM_E_RESPONSEPENDING == responseCode) {
                        *pendingDid = didPtr->DspDidIdentifier;
                    } else if( DCM_E_POSITIVERESPONSE == responseCode ) {
                        /* Data successfully read */
                        (*didIndex)++;
                        *pendingSignalIndex = 0;
                    }
                } else {
                    /* This did only references other dids or did already read. */
                    if( *didIndex >= didStartIndex ) {
                        /* Not already read. */
                        if ((*txPos + 2) <= pduTxData->SduLength) {
                            pduTxData->SduDataPtr[(*txPos)++] = (didPtr->DspDidIdentifier >> 8) & 0xFFu;
                            pduTxData->SduDataPtr[(*txPos)++] = didPtr->DspDidIdentifier & 0xFFu;
                            responseCode = DCM_E_POSITIVERESPONSE;
                        } else {
                            /* Tx buffer full. */
                            responseCode = DCM_E_REQUESTOUTOFRANGE;
                        }
                    }
                    (*didIndex)++;
                }
            }
            else {  // Not allowed in current security level
                responseCode = DCM_E_SECURITYACCESSDENIED;
            }
        }
        else {  // Not allowed in current session
            responseCode = DCM_E_SERVICENOTSUPPORTEDINACTIVESESSION;
        }
    }
    else {  // Read access not configured
        responseCode = DCM_E_REQUESTOUTOFRANGE;
    }

    for (uint16 i = 0; (!didPtr->DspDidRef[i]->Arc_EOL) && (DCM_E_POSITIVERESPONSE == responseCode); i++) {
        /* Recurse trough the rest of the dids. *//** @req DCM440 */
        responseCode = readDidData(didPtr->DspDidRef[i], pduTxData, txPos, pendingState, pendingDid, pendingSignalIndex, pendingDataLen, didIndex, didStartIndex, didDataStartPos);
    }

    return responseCode;
}


Dcm_NegativeResponseCodeType getDidData(const Dcm_DspDidType *didPtr, PduInfoType *pduTxData, uint16 *txPos, ReadDidPendingStateType *pendingState, uint16 *pendingDataLen, uint16 *pendingSignalIndex, uint16 *didDataStartPos)
{
    Dcm_NegativeResponseCodeType errorCode = DCM_E_POSITIVERESPONSE;
    Dcm_OpStatusType opStatus = DCM_INITIAL;
    Std_ReturnType result = E_OK;
    const Dcm_DspSignalType *signalPtr;
    const Dcm_DspDataType *dataPtr;
    if( (DCM_READ_DID_PENDING_COND_CHECK == *pendingState) || (DCM_READ_DID_PENDING_READ_DATA == *pendingState) ){
        opStatus = DCM_PENDING;
    } else {
        pduTxData->SduDataPtr[(*txPos)++] = (didPtr->DspDidIdentifier >> 8) & 0xFFu;
        pduTxData->SduDataPtr[(*txPos)++] = didPtr->DspDidIdentifier & 0xFFu;
        *didDataStartPos = *txPos;
    }
    /* @req Dcm578 Skipping condition check for ECU_SIGNALs */

    for(uint16 signalIndex = *pendingSignalIndex; (signalIndex < didPtr->DspNofSignals) && (DCM_E_POSITIVERESPONSE == errorCode); signalIndex++) {
        signalPtr = &didPtr->DspSignalRef[signalIndex];
        dataPtr = signalPtr->DspSignalDataRef;

        if( ((DCM_READ_DID_PENDING_COND_CHECK == *pendingState) || (DCM_READ_DID_IDLE == *pendingState)) && (DATA_PORT_ECU_SIGNAL != dataPtr->DspDataUsePort)) {
            /* @req Dcm439 */
            if((DATA_PORT_ASYNCH == dataPtr->DspDataUsePort) || (DATA_PORT_SYNCH == dataPtr->DspDataUsePort)) {
                if( NULL != dataPtr->DspDataConditionCheckReadFnc ) {
                    result = dataPtr->DspDataConditionCheckReadFnc(opStatus, &errorCode);
                } else {
                    result = E_NOT_OK;
                }
                if(DATA_PORT_ASYNCH == dataPtr->DspDataUsePort) {
                    if( E_PENDING == result ) {
                        *pendingState = DCM_READ_DID_PENDING_COND_CHECK;
                    } else {
                        *pendingState = DCM_READ_DID_IDLE;
                        opStatus = DCM_INITIAL;
                    }
                } else {
                    if( E_PENDING == result ) {
                        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                        result = E_NOT_OK;
                    }
                }
            }
#if defined(USE_NVM)
            else if(DATA_PORT_BLOCK_ID == dataPtr->DspDataUsePort){
                /* @req DCM560 */
                if (E_OK == NvM_ReadBlock(dataPtr->DspNvmUseBlockID, &pduTxData->SduDataPtr[(*txPos)])) {
                    *pendingState = DCM_READ_DID_PENDING_READ_DATA;
                }else{
                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                    result = E_NOT_OK;
                }
            }
#endif
            else {
                /* Port not supported */
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_CONFIG_INVALID);
                result = E_NOT_OK;
            }
        } else {
            /* Condition check function already called with positive return */
            result = E_OK;
            errorCode = DCM_E_POSITIVERESPONSE;
        }

        if ((result == E_OK) && (errorCode == DCM_E_POSITIVERESPONSE)) {    /** @req DCM439 */
            uint16 dataLen = 0;
            if ( dataPtr->DspDataInfoRef->DspDidFixedLength) {  /** @req DCM436 */
                dataLen = dataPtr->DspDataSize;
            } else {
                if (dataPtr->DspDataReadDataLengthFnc != NULL) {
                    if( DCM_READ_DID_IDLE == *pendingState ) {
                        /* ReadDataLengthFunction is only allowed to return E_OK  */
                        if(E_OK != dataPtr->DspDataReadDataLengthFnc(&dataLen)) {
                            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                            result = E_NOT_OK;
                        }
                    } else {
                        /* Read length function has already been called */
                        dataLen = *pendingDataLen;
                    }
                }
            }
            if( E_OK == result ) {
                // Now ready for reading the data!
                if ((*didDataStartPos + signalPtr->DspSignalsPosition + dataLen) <= pduTxData->SduLength) {
                    /** @req DCM437 */
                    if((DATA_PORT_SYNCH == dataPtr->DspDataUsePort ) || (DATA_PORT_ECU_SIGNAL == dataPtr->DspDataUsePort)) {
                        if( NULL != dataPtr->DspDataReadDataFnc.SynchDataReadFnc ) {
                            /* Synch read function is only allowed to return E_OK */
                            if(E_OK != dataPtr->DspDataReadDataFnc.SynchDataReadFnc(&pduTxData->SduDataPtr[(*didDataStartPos) + signalPtr->DspSignalsPosition]) ) {
                                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                                result = E_NOT_OK;
                            }
                        } else {
                            result = E_NOT_OK;
                        }
                    } else if( DATA_PORT_ASYNCH == dataPtr->DspDataUsePort ) {
                        if( NULL != dataPtr->DspDataReadDataFnc.AsynchDataReadFnc ) {
                            result = dataPtr->DspDataReadDataFnc.AsynchDataReadFnc(opStatus, &pduTxData->SduDataPtr[(*didDataStartPos) + signalPtr->DspSignalsPosition]);
                        } else {
                            result = E_NOT_OK;
                        }
#if defined(USE_NVM)
                    } else if ( DATA_PORT_BLOCK_ID == dataPtr->DspDataUsePort ){
                        NvM_RequestResultType errorStatus;
                        if (E_OK != NvM_GetErrorStatus(dataPtr->DspNvmUseBlockID, &errorStatus)){
                            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                            result = E_NOT_OK;
                        }
                        if (errorStatus == NVM_REQ_PENDING){
                            result = E_PENDING;
                        }else if (errorStatus == NVM_REQ_OK){
                            *pendingState = DCM_READ_DID_IDLE;
                        }else{
                            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
                            result = E_NOT_OK;
                        }
#endif
                    }else {
                        /* Port not supported */
                        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_CONFIG_INVALID);
                        result = E_NOT_OK;
                    }
                    if( *txPos < ((*didDataStartPos) + signalPtr->DspSignalsPosition + dataLen) ) {
                        *txPos = ((*didDataStartPos) + signalPtr->DspSignalsPosition + dataLen);
                    }

                    if( E_PENDING == result ) {
                        *pendingState = DCM_READ_DID_PENDING_READ_DATA;
                        *pendingDataLen = dataLen;
                        *pendingSignalIndex = signalIndex;
                        errorCode = DCM_E_RESPONSEPENDING;
                    }
                    else if (result != E_OK) {
                        errorCode = DCM_E_CONDITIONSNOTCORRECT;
                    } else {
                        /* Did successfully read */
                        *pendingState = DCM_READ_DID_IDLE;
                    }
                } else { // tx buffer full
                    errorCode = DCM_E_REQUESTOUTOFRANGE;
                }
            } else {// Invalid return from readLenFunction
                errorCode = DCM_E_CONDITIONSNOTCORRECT;
            }
        } else if( E_PENDING == result ) {
            /* Pending condition check */
            *pendingSignalIndex = signalIndex;
            errorCode = DCM_E_RESPONSEPENDING;
        } else {    // CheckRead failed
            errorCode = DCM_E_CONDITIONSNOTCORRECT;
        }
    }

    return errorCode;
}


void getDidLength(const Dcm_DspDidType *didPtr, uint16 *length, uint16* nofDatas)
{

    boolean didOnlyRefsDids = ((NULL == didPtr->DspSignalRef));

    if( !didOnlyRefsDids) {
        /* Get the data if available */
        (*length) += didPtr->DspDidDataSize;
        (*nofDatas)++;
    }

    for (uint16 i = 0; !didPtr->DspDidRef[i]->Arc_EOL; i++) {
        /* Recurse trough the rest of the dids. */
        getDidLength(didPtr->DspDidRef[i], length, nofDatas);
    }

}

