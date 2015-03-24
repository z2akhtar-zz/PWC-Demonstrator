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


#ifndef COM_MISC_H_
#define COM_MISC_H_

// Copy Signal group data from PDU to shadowbuffer
void Com_CopySignalGroupDataFromPduToShadowBuffer(
		const Com_SignalIdType signalGroupId);

// Copy Signal group data from shadowbuffer to Pdu
void Com_CopySignalGroupDataFromShadowBufferToPdu(
		const Com_SignalIdType signalGroupId,
		boolean deferredBufferDestination,
		boolean *dataChanged);

/**
 * Reads signal data from Pdu
 * @param comIPduDataPtr
 * @param bitPosition
 * @param bitSize
 * @param endian
 * @param signalType
 * @param SignalData
 */
void Com_Internal_ReadSignalDataFromPdu (
	    const uint8 *comIPduDataPtr,
		Com_BitPositionType bitPosition,
		uint16 bitSize,
		ComSignalEndianess_type endian,
		Com_SignalType signalType,
		uint8 *SignalData);

/**
 * Write signal data to PDU
 * @param SignalDataPtr
 * @param signalType
 * @param comIPduDataPtr
 * @param bitPosition
 * @param bitSize
 * @param endian
 * @param dataChanged
 */
void Com_WriteSignalDataToPdu(
		const uint8 *SignalDataPtr,
		Com_SignalType signalType,
	    uint8 *comIPduDataPtr,
		Com_BitPositionType bitPosition,
		uint16 bitSize,
		ComSignalEndianess_type endian,
		boolean *dataChanged);

void Com_Internal_UpdateShadowSignal(Com_SignalIdType SignalId, const void *SignalDataPtr);

/*
 * This function copies numBits bits of data from Source to Destination with the possibility to offset
 * both the source and destination.
 *
 * Return value: the last bit it copies (sign bit).
 */
//uint8 Com_CopyData(void *Destination, const void *Source, uint8 numBits, uint8 destOffset, uint8 sourceOffset);

//void Com_CopyData2(char *dest, const char *source, uint8 destByteLength, uint8 segmentStartBitOffset, uint8 segmentBitLength);

void Com_RxProcessSignals(const ComIPdu_type *IPdu,Com_Arc_IPdu_type *Arc_IPdu);
static inline PduIdType getPduId(const ComIPdu_type* IPdu) {
	extern const Com_ConfigType * ComConfig;
	return (PduIdType)(IPdu - (ComConfig->ComIPdu));
}

void UnlockTpBuffer(PduIdType PduId);
#define isPduBufferLocked(id) Com_BufferPduState[id].locked


/* Helpers for getting and setting that a PDU confirmation status */
void SetTxConfirmationStatus(const ComIPdu_type *IPdu, boolean value);
boolean GetTxConfirmationStatus(const ComIPdu_type *IPdu);

#endif /* COM_MISC_H_ */
