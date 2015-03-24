/*
 * Generator version: 1.2.0
 * AUTOSAR version:   4.0.3
 */

#include "PduR.h"
#include "MemMap.h"

#if PDUR_CANIF_SUPPORT == STD_ON
#include "CanIf.h"
#include "CanIf_PBCfg.h"
#endif
#if PDUR_CANTP_SUPPORT == STD_ON
#include "CanTp.h"
#include "CanTp_PBCfg.h"
#endif
#if PDUR_LINIF_SUPPORT == STD_ON
#include "LinIf.h"
#endif
#if PDUR_COM_SUPPORT == STD_ON
#include "Com.h"
#include "Com_PbCfg.h"
#endif
#if PDUR_DCM_SUPPORT == STD_ON
#include "Dcm.h"
#endif
#if PDUR_SOAD_SUPPORT == STD_ON
#include "SoAd.h"
#endif
#if PDUR_J1939TP_SUPPORT == STD_ON
#include "J1939Tp.h"
#endif
#if PDUR_IPDUM_SUPPORT == STD_ON
#include "IpduM.h"
#endif


// Destinations


// Routing paths


SECTION_POSTBUILD_DATA const PduRRoutingPath_type * const PduRRoutingPaths[] = { 
	NULL
};

// RoutingPathGroups
// NOTE: RoutingPathGroup API not implemented


// Default values


// Tx buffer default value pointer list (sorted in the same order as Tx buffer IDs)
SECTION_POSTBUILD_DATA const uint8 * const PduRTxBufferDefaultValues[] =
{
	NULL
};

// Tx buffer default value length list (sorted in the same order as Tx buffer IDs)
SECTION_POSTBUILD_DATA const uint32 * const PduRTxBufferDefaultValueLengths[] =
{
	NULL
};


// Exported config
SECTION_POSTBUILD_DATA const PduR_PBConfigType PduR_Config = {
	.PduRConfigurationId = 0,
	.RoutingPaths = PduRRoutingPaths,
	.NRoutingPaths = 0,
	.DefaultValues = PduRTxBufferDefaultValues,
	.DefaultValueLengths = PduRTxBufferDefaultValueLengths,
};



