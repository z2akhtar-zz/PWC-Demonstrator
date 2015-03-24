/*
 * Generator version: 1.2.0
 * AUTOSAR version:   4.0.3
 */

#include "PduR.h"




PduRTpBufferInfo_type PduRTpBuffers[] = {

	{
		.pduInfoPtr = NULL,
		.status = PDUR_BUFFER_NOT_ALLOCATED_FROM_UP_MODULE,
		.bufferSize = 0
	}
};


PduRTpBufferInfo_type *PduRTpRouteBufferPtrs[] = {
	NULL
};




// Tx buffer pointer list (sorted in the same order as Tx buffer IDs)
PduRTxBuffer_type PduRTxBuffers[] =
{
	NULL
};


const PduR_RamBufCfgType PduR_RamBufCfg = {
	.TpBuffers = PduRTpBuffers,
	.TpRouteBuffers = PduRTpRouteBufferPtrs,
	.TxBuffers = PduRTxBuffers,
	.NTpBuffers = 1, // Including 1 non-allocated buffer place holder for Upper module allocated buffer
	.NTpRouteBuffers = 0, 
	.NTxBuffers = 0
};


