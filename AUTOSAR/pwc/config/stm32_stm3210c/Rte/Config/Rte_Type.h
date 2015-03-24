/**
 * RTE Types Header File
 *
 * @req SWS_Rte_01161
 * @req SWS_Rte_01160
 */

#ifndef RTE_TYPE_H_
#define RTE_TYPE_H_

/** --- Includes ----------------------------------------------------------------------------------
 * @req SWS_Rte_01163
 */
#include <Rte.h>
#include <Rte_Type_Workarounds.h>

// No support for unknown types (for commandType) yet

// No support for unknown types (for myBoolean) yet

// No support for unknown types (for requestType) yet            

/* Redefinition type AnalogValue */
typedef sint32 AnalogValue;

/* Redefinition type ComM_InhibitionStatusType */
typedef uint8 ComM_InhibitionStatusType;

/* Redefinition type ComM_ModeType */
typedef uint8 ComM_ModeType;

/* Redefinition type ComM_UserHandleType */
typedef uint8 ComM_UserHandleType;

/* Pointer type ConstVoidPtr */
typedef const void * ConstVoidPtr;

/* Redefinition type Dcm_ConfirmationStatusType */
typedef uint8 Dcm_ConfirmationStatusType;

/* Redefinition type Dcm_NegativeResponseCodeType */
typedef uint8 Dcm_NegativeResponseCodeType;

/* Redefinition type Dcm_OpStatusType */
typedef uint8 Dcm_OpStatusType;

/* Redefinition type Dcm_ProtocolType */
typedef uint8 Dcm_ProtocolType;

/* Redefinition type Dcm_RoeStateType */
typedef uint8 Dcm_RoeStateType;

/* Redefinition type Dcm_SecLevelType */
typedef uint8 Dcm_SecLevelType;

/* Redefinition type Dcm_SesCtrlType */
typedef uint8 Dcm_SesCtrlType;

/* Redefinition type Dem_DTCFormatType */
typedef uint8 Dem_DTCFormatType;

/* Redefinition type Dem_DTCOriginType */
typedef uint8 Dem_DTCOriginType;

/* Redefinition type Dem_DTCStatusMaskType */
typedef uint8 Dem_DTCStatusMaskType;

/* Redefinition type Dem_EventIdType */
typedef uint16 Dem_EventIdType;

/* Redefinition type Dem_EventStatusExtendedType */
typedef uint8 Dem_EventStatusExtendedType;

/* Redefinition type Dem_EventStatusType */
typedef uint8 Dem_EventStatusType;

/* Redefinition type Dem_IndicatorStatusType */
typedef uint8 Dem_IndicatorStatusType;

/* Redefinition type Dem_InitMonitorReasonType */
typedef uint8 Dem_InitMonitorReasonType;

/* Array type Dem_MaxDataValueType */
typedef uint8 Dem_MaxDataValueType[1];

/* Redefinition type Dem_OperationCycleIdType */
typedef uint8 Dem_OperationCycleIdType;

/* Redefinition type Dem_OperationCycleStateType */
typedef uint8 Dem_OperationCycleStateType;

/* Redefinition type Dem_ReturnClearDTCType */
typedef uint8 Dem_ReturnClearDTCType;

/* Redefinition type DigitalLevel */
typedef uint8 DigitalLevel;

/* Redefinition type DutyCycle */
typedef uint32 DutyCycle;

/* Redefinition type EcuM_BootTargetType */
typedef uint8 EcuM_BootTargetType;

/* Redefinition type EcuM_StateType */
typedef uint8 EcuM_StateType;

/* Redefinition type EcuM_UserType */
typedef uint8 EcuM_UserType;

/* Redefinition type Frequency */
typedef uint32 Frequency;

/* Redefinition type IoHwAb_SignalType_ */
typedef uint32 IoHwAb_SignalType_;

/* Redefinition type NvM_BlockIdType */
typedef uint16 NvM_BlockIdType;

/* Redefinition type NvM_RequestResultType */
typedef uint8 NvM_RequestResultType;

/* Redefinition type SignalQuality */
typedef uint8 SignalQuality;

/* Pointer type VoidPtr */
typedef void * VoidPtr;

#endif /* RTE_TYPE_H_ */
