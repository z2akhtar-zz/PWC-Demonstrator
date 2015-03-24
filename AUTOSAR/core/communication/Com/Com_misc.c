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
#include <assert.h>

#include "Com_Arc_Types.h"
#include "Com.h"
#include "Com_Internal.h"
#include "Com_misc.h"
#include "debug.h"
#include "Cpu.h"

/* Declared in Com_Cfg.c */
extern const ComNotificationCalloutType ComNotificationCallouts[];

void Com_CopySignalGroupDataFromShadowBufferToPdu(const Com_SignalIdType signalGroupId, boolean deferredBufferDestination, boolean *dataChanged) {

	// Get PDU
	const ComSignal_type * Signal = GET_Signal(signalGroupId);
	const ComIPdu_type *IPdu = GET_IPdu(Signal->ComIPduHandleId);
	Com_Arc_Signal_type * Arc_Signal = GET_ArcSignal(Signal->ComHandleId);

	uint8 *pduDataPtr = 0;
	if (deferredBufferDestination) {
		pduDataPtr = GET_ArcIPdu(Signal->ComIPduHandleId)->ComIPduDeferredDataPtr;
	} else {
		pduDataPtr = GET_ArcIPdu(Signal->ComIPduHandleId)->ComIPduDataPtr;
	}

	// Aligned opaque data -> straight copy with signalgroup mask
    uint8 *buf = (uint8 *)Arc_Signal->Com_Arc_ShadowBuffer;
    uint8 data = 0;
    *dataChanged = FALSE;
    for(int i= 0; i < IPdu->ComIPduSize; i++){
        data = (~Signal->Com_Arc_ShadowBuffer_Mask[i] & *pduDataPtr) |
               (Signal->Com_Arc_ShadowBuffer_Mask[i] & *buf);
        if(*pduDataPtr != data) {
            *dataChanged = TRUE;
        }
        *pduDataPtr = data;
        buf++;
        pduDataPtr++;
    }

}


void Com_CopySignalGroupDataFromPduToShadowBuffer(const Com_SignalIdType signalGroupId) {

	// Get PDU
	const ComSignal_type * Signal = GET_Signal(signalGroupId);
	const ComIPdu_type *IPdu = GET_IPdu(Signal->ComIPduHandleId);

	const uint8 *pduDataPtr = 0;
	if (IPdu->ComIPduSignalProcessing == DEFERRED && IPdu->ComIPduDirection == RECEIVE) {
		pduDataPtr = GET_ArcIPdu(Signal->ComIPduHandleId)->ComIPduDeferredDataPtr;
	} else {
		pduDataPtr = GET_ArcIPdu(Signal->ComIPduHandleId)->ComIPduDataPtr;
	}

	// Aligned opaque data -> straight copy with signalgroup mask
	uint8 *buf = (uint8 *)GET_ArcSignal(Signal->ComHandleId)->Com_Arc_ShadowBuffer;
	for(int i= 0; i < IPdu->ComIPduSize; i++){
		*buf++ = Signal->Com_Arc_ShadowBuffer_Mask[i] & *pduDataPtr++;
	}
}

void Com_Internal_ReadSignalDataFromPdu (
	    const uint8 *comIPduDataPtr,
		Com_BitPositionType bitPosition,
		uint16 bitSize,
		ComSignalEndianess_type endian,
		Com_SignalType signalType,
		uint8 *SignalData) {
	imask_t state;
	Irq_Save(state);

	if (endian == COM_OPAQUE || signalType == UINT8_N) {
		// Aligned opaque data -> straight copy
		/* @req COM472 */
		memcpy(SignalData, comIPduDataPtr + bitPosition/8, bitSize / 8);
		Irq_Restore(state);
	} else {
		// Unaligned data and/or endianness conversion
		uint32 pduData;
		if(endian == COM_BIG_ENDIAN) {
			uint32 lsbIndex = ((bitPosition ^ 0x7) + bitSize - 1) ^ 7; // calculate lsb bit index. This could be moved to generator
			const uint8 *pduDataPtr = comIPduDataPtr + lsbIndex / 8 - 3; // calculate big endian ptr to data
			uint8 bitShift = lsbIndex % 8;
			pduData = 0;
			for(uint32 i = 0; i < 4; i++) {
				pduData = (pduData << 8) | pduDataPtr[i];
			}
			pduData >>= bitShift;
			if(32 - bitShift < bitSize) {
				pduData |= pduDataPtr[-1] << (32 - bitShift);
			}
		} else if (endian == COM_LITTLE_ENDIAN) {
			uint32 lsbIndex = bitPosition;
			const uint8 *pduDataPtr = comIPduDataPtr + (bitPosition/8);
			uint8 bitShift = lsbIndex % 8;
			pduData = 0;
			for(sint32 i = 3; i >= 0; i--) {
				pduData = (pduData << 8) | pduDataPtr[i];
			}
			pduData >>= bitShift;
			if(32 - bitShift < bitSize) {
				pduData |= pduDataPtr[4] << (32 - bitShift);
			}
		} else {
			assert(0);
		}
		Irq_Restore(state);
		uint32 mask = 0xFFFFFFFFu >> (32 - bitSize); // calculate mask for SigVal
		pduData &= mask; // clear bit out of range
		uint32 signmask = ~(mask >> 1);
		switch(signalType) {
		case SINT8:
			if(pduData & signmask) {
				pduData |= signmask; // add sign bits
			}
			// no break, sign extended data can be written as uint
		case BOOLEAN:
		case UINT8:
			*(uint8*)SignalData = pduData;
			break;
		case SINT16:
			if(pduData & signmask) {
				pduData |= signmask; // add sign bits
			}
			// no break, sign extended data can be written as uint
		case UINT16:
			*(uint16*)SignalData = pduData;
			break;
		case SINT32:
			if(pduData & signmask) {
				pduData |= signmask; // add sign bits
			}
			// no break, sign extended data can be written as uint
		case UINT32:
			*(uint32*)SignalData = pduData;
			break;
		case UINT8_N:
		case UINT8_DYN:
			assert(0);
		}
	}
}

void Com_WriteSignalDataToPdu(
		const uint8 *SignalDataPtr,
		Com_SignalType signalType,
	    uint8 *comIPduDataPtr,
		Com_BitPositionType bitPosition,
		uint16 bitSize,
		ComSignalEndianess_type endian,
		boolean *dataChanged) {
    imask_t irq_state;
	if (endian == COM_OPAQUE || signalType == UINT8_N) {
		/* @req COM472 */
		uint8 *pduBufferBytes = (uint8 *)comIPduDataPtr + bitPosition / 8;
		uint16 signalLength = bitSize / 8;
		Irq_Save(irq_state);
		*dataChanged = ( 0 != memcmp(pduBufferBytes, SignalDataPtr, signalLength) );
		memcpy(pduBufferBytes, SignalDataPtr, signalLength);
	} else {
		uint32 sigVal = 0;
		switch(signalType) {
		case BOOLEAN:
		case UINT8:
		case SINT8:
			sigVal = *((uint8*)SignalDataPtr);
			break;
		case UINT16:
		case SINT16:
			sigVal = *((uint16*)SignalDataPtr);
			break;
		case UINT32:
		case SINT32:
			sigVal = *((uint32*)SignalDataPtr);
			break;
		case UINT8_N:
		case UINT8_DYN:
			assert(0);
		}
		uint32 mask = 0xFFFFFFFFu >> (32 - bitSize); // calculate mask for SigVal
		sigVal &= mask; // mask sigVal;
		Irq_Save(irq_state);
		if(endian == COM_BIG_ENDIAN) {
			uint32 lsbIndex = ((bitPosition ^ 0x7) + bitSize - 1) ^ 7; // calculate lsb bit index. This could be moved to generator
			uint8 *pduDataPtr = (comIPduDataPtr + lsbIndex / 8 - 3); // calculate big endian ptr to data
			uint32 pduData = 0;
			for(uint32 i = 0; i < 4; i++) {
				pduData = (pduData << 8) | pduDataPtr[i];
			}
			uint8 bitShift = lsbIndex % 8;
			uint32 sigLo = sigVal << bitShift;
			uint32 maskLo = ~(mask  << bitShift);
			uint32 newPduData = (pduData & maskLo) | sigLo;
			*dataChanged = (newPduData != pduData);
			for(int i = 3; i >= 0; i--) {
				pduDataPtr[i] = (uint8)newPduData;
				newPduData >>= 8;
			}
			sint8 maxBitsWritten = 32 - bitShift;
			if(maxBitsWritten < bitSize) {
				pduDataPtr--;
				pduData = *pduDataPtr;
				uint32 maskHi = ~(mask  >> maxBitsWritten);
				uint32 sigHi = sigVal >> maxBitsWritten;
				newPduData = (pduData & maskHi) | sigHi;
				*dataChanged |= (newPduData != pduData) ? TRUE : *dataChanged;
				*pduDataPtr = newPduData;
			}
		} else if (endian == COM_LITTLE_ENDIAN) {
			uint32 lsbIndex = bitPosition; // calculate lsb bit index.
			uint8 *pduDataPtr = (comIPduDataPtr + lsbIndex / 8); // calculate big endian ptr to data
			uint32 pduData = 0;
			for(sint32 i = 3; i >= 0; i--) {
				pduData = (pduData << 8) | pduDataPtr[i];
			}
			uint8 bitShift = lsbIndex % 8;
			uint32 sigLo = sigVal << bitShift;
			uint32 maskLo = ~(mask  << bitShift);
			uint32 newPduData = (pduData & maskLo) | sigLo;
			*dataChanged = (newPduData != pduData);
			for(uint32 i = 0; i < 4; i++) {
				pduDataPtr[i] = (uint8)newPduData;
				newPduData >>= 8;
			}
			sint8 maxBitsWritten = 32 - bitShift;
			if(maxBitsWritten < bitSize) {
				pduDataPtr += 4;
				pduData = *pduDataPtr;
				uint32 maskHi = ~(mask >> maxBitsWritten);
				uint32 sigHi = sigVal >> maxBitsWritten;
				newPduData = (pduData & maskHi) | sigHi;
				*dataChanged = (newPduData != pduData) ? TRUE : *dataChanged;
				*pduDataPtr = newPduData;
			}
		} else {
			assert(0);
		}
	}
    Irq_Restore(irq_state);
}

void Com_RxProcessSignals(const ComIPdu_type *IPdu,Com_Arc_IPdu_type *Arc_IPdu) {
	/* !req COM053 */
	/* @req COM055 */
	/* !req COM396 */ /* Neither invalidation nor filtering supported */
	/* !req COM352 */
	const ComSignal_type *comSignal;
	for (uint8 i = 0; IPdu->ComIPduSignalRef[i] != NULL; i++) {
		comSignal = IPdu->ComIPduSignalRef[i];
		Com_Arc_Signal_type * Arc_Signal = GET_ArcSignal(comSignal->ComHandleId);

		// If this signal uses an update bit, then it is only considered if this bit is set.
		/* @req COM324 */
		/* @req COM067 */
		if ( (!comSignal->ComSignalArcUseUpdateBit) ||
			( (comSignal->ComSignalArcUseUpdateBit) && (TESTBIT(Arc_IPdu->ComIPduDataPtr, comSignal->ComUpdateBitPosition)) ) ) {

			if (comSignal->ComTimeoutFactor > 0) { // If reception deadline monitoring is used.
				// Reset the deadline monitoring timer.
				/* @req COM715 */
				Arc_Signal->Com_Arc_DeadlineCounter = comSignal->ComTimeoutFactor;
			}

			// Check the signal processing mode.
			if (IPdu->ComIPduSignalProcessing == IMMEDIATE) {
				// If signal processing mode is IMMEDIATE, notify the signal callback.
				/* @req COM300 */
				/* @req COM301 */
				if ((IPdu->ComIPduSignalRef[i]->ComNotification != COM_NO_FUNCTION_CALLOUT) &&
					(ComNotificationCallouts[IPdu->ComIPduSignalRef[i]->ComNotification] != NULL) ) {
					ComNotificationCallouts[IPdu->ComIPduSignalRef[i]->ComNotification]();
				}
			} else {
				// Signal processing mode is DEFERRED, mark the signal as updated.
				Arc_Signal->ComSignalUpdated = TRUE;
			}

		} else {
			DEBUG(DEBUG_LOW, "Com_RxIndication: Ignored signal %d of I-PD %d since its update bit was not set\n", comSignal->ComHandleId, ComRxPduId);
		}
	}
}

void UnlockTpBuffer(PduIdType PduId) {
	Com_BufferPduState[PduId].locked = false;
	Com_BufferPduState[PduId].currentPosition = 0;
}

void Com_Internal_UpdateShadowSignal(Com_SignalIdType SignalId, const void *SignalDataPtr) {
	Com_Arc_GroupSignal_type *Arc_GroupSignal = GET_ArcGroupSignal(SignalId);
	const ComGroupSignal_type *GroupSignal = GET_GroupSignal(SignalId);
	/* @req COM632 */
	/* @req COM633 */ /* Sign extension? */
	boolean dataChanged = FALSE;
	Com_WriteSignalDataToPdu(
			SignalDataPtr,
			GroupSignal->ComSignalType,
			Arc_GroupSignal->Com_Arc_ShadowBuffer,
			GroupSignal->ComBitPosition,
			GroupSignal->ComBitSize,
			GroupSignal->ComSignalEndianess,
			&dataChanged);
}

/* Helpers for getting and setting that a TX PDU confirmation status
 * These function uses the ComSignalUpdated for the first signal within the Pdu. The
 * ComSignalUpdated isn't used for anything else in TxSignals and it is mainly used
 * in Rx signals.
 * The reason is to save RAM.
 */

void SetTxConfirmationStatus(const ComIPdu_type *IPdu, boolean value) {

    const ComSignal_type *signal = IPdu->ComIPduSignalRef[0];

    if (signal != NULL) {
        Com_Arc_Signal_type * Arc_Signal = GET_ArcSignal(signal->ComHandleId);
        Arc_Signal->ComSignalUpdated = value;
    }
}

boolean GetTxConfirmationStatus(const ComIPdu_type *IPdu) {

    if (IPdu == NULL) {
        return FALSE;
    }

    const ComSignal_type *signal = IPdu->ComIPduSignalRef[0];

    if (signal == NULL) {
        return FALSE;
    }

    Com_Arc_Signal_type * Arc_Signal = GET_ArcSignal(signal->ComHandleId);
    return Arc_Signal->ComSignalUpdated;
}


