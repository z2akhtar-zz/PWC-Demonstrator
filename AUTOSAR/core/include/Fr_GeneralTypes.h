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

/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.0.3 */
/** @tagSettings DEFAULT_ARCHITECTURE=RH850F1H */

/* @req FR455 */
/* @req FR499 */
#ifndef FR_GENERAL_TYPES
#define FR_GENERAL_TYPES

#if defined(USE_DEM) || defined(CFG_FR_DEM_TEST)
#include "Dem.h"
#endif

/* @req FR102 */

/* @req FR117 */
/* @req FR110 */
/* @req FR077 */

/* @req FR505 */
typedef enum {
    FR_POCSTATE_CONFIG = 0,
    FR_POCSTATE_DEFAULT_CONFIG,
    FR_POCSTATE_HALT,
    FR_POCSTATE_NORMAL_ACTIVE,
    FR_POCSTATE_NORMAL_PASSIVE,
    FR_POCSTATE_READY,
    FR_POCSTATE_STARTUP,
    FR_POCSTATE_WAKEUP
}Fr_POCStateType;

/* @req FR506 */
typedef enum {
    FR_SLOTMODE_KEYSLOT,
    FR_SLOTMODE_ALL_PENDING,
    FR_SLOTMODE_ALL
}Fr_SlotModeType;

/* @req FR599 */
#define FR_SLOTMODE_SINGLE()  FR_SLOTMODE_KEYSLOT

/* @req FR507 */
typedef enum {
    FR_ERRORMODE_ACTIVE = 0,
    FR_ERRORMODE_PASSIVE,
    FR_ERRORMODE_COMM_HALT
}Fr_ErrorModeType;

/* @req FR508 */
typedef enum {
    FR_WAKEUP_UNDEFINED = 0,
    FR_WAKEUP_RECEIVED_HEADER,
    FR_WAKEUP_RECEIVED_WUP,
    FR_WAKEUP_COLLISION_HEADER,
    FR_WAKEUP_COLLISION_WUP,
    FR_WAKEUP_COLLISION_UNKNOWN,
    FR_WAKEUP_TRANSMITTED
}Fr_WakeupStatusType;

/* @req FR509 */
typedef enum {
    FR_STARTUP_UNDEFINED = 0,
    FR_STARTUP_COLDSTART_LISTEN,
    FR_STARTUP_INTEGRATION_COLDSTART_CHECK,
    FR_STARTUP_COLDSTART_JOIN,
    FR_STARTUP_COLDSTART_COLLISION_RESOLUTION,
    FR_STARTUP_COLDSTART_CONSISTENCY_CHECK,
    FR_STARTUP_INTEGRATION_LISTEN,
    FR_STARTUP_INITIALIZE_SCHEDULE,
    FR_STARTUP_INTEGRATION_CONSISTENCY_CHECK,
    FR_STARTUP_COLDSTART_GAP,
    FR_STARTUP_EXTERNAL_STARTUP
}Fr_StartupStateType;

/* !req FR510 */
typedef struct {
    boolean CHIHaltRequest;
    boolean ColdstartNoise;
    Fr_ErrorModeType ErrorMode;
    boolean Freeze;
    Fr_SlotModeType SlotMode;
    Fr_StartupStateType StartupState;
    Fr_POCStateType State;
    Fr_WakeupStatusType WakeupStatus;
    /*boolean CHIReadyRequest;  NOT SUPPORTED */
}Fr_POCStatusType;

/* @req FR511 */
typedef enum {
    FR_TRANSMITTED = 0,
    FR_NOT_TRANSMITTED
}Fr_TxLPduStatusType;

/* @req FR512 */
typedef enum {
    FR_RECEIVED = 0,
    FR_NOT_RECEIVED,
    FR_RECEIVED_MORE_DATA_AVAILABLE
}Fr_RxLPduStatusType;

/* @req FR468 */
/* @req FR514 */
typedef enum {
    FR_CHANNEL_A = 0,
    FR_CHANNEL_B,
    FR_CHANNEL_AB
}Fr_ChannelType;

/* @req FrTrcv434 */
typedef enum {
    FRTRCV_TRCVMODE_NORMAL,
    FRTRCV_TRCVMODE_STANDBY,
    FRTRCV_TRCVMODE_SLEEP, // sleep is optional.
    FRTRCV_TRCVMODE_RECEIVEONLY // receive only is optional.
}FrTrcv_TrcvModeType;

/* @req FrTrcv435 */
/* @req FrTrcv074 */
typedef enum {
    FRTRCV_WU_NOT_SUPPORTED,
    FRTRCV_WU_BY_BUS,
    FRTRCV_WU_BY_PIN,
    FRTRCV_WU_INTERNALLY,
    FRTRCV_WU_RESET,
    FRTRCV_WU_POWER_ON
}FrTrcv_TrcvWUReasonType;

#if 0
typedef struct {
    //Implementation specific

    //This type contains the implementation-specific post build time configuration structure.
    //Only pointers of this type are allowed.
}FrIf_ConfigType;
#endif

typedef enum {
    FRIF_STATE_OFFLINE,
    FRIF_STATE_ONLINE
}FrIf_StateType;

typedef enum {
    FRIF_GOTO_OFFLINE,
    FRIF_GOTO_ONLINE
}FrIf_StateTransitionType;

typedef enum {
    FR_N1SAMPLES = 0,
    FR_N2SAMPLES,
    FR_N4SAMPLES
}Fr_SamplePerMicroTickType;

typedef enum {
    FR_T12_5NS = 0,
    FR_T25NS,
    FR_T50NS,
    FR_T100NS,
    FR_T200NS,
    FR_T400NS
}Fr_MicroTickType;

/* !req FR657 - Fr_ReadCCConfig not implemented*/
#if 0
/* Macro name,                                      Value,  Mapps to configuration parameter */
#define FR_CIDX_GDCYCLE()                           /* 0, FrIfGdCycle */
#define FR_CIDX_PMICROPERCYCLE()                    /* 1, FrPMicroPerCycle */
#define FR_CIDX_PDLISTENTIMEOUT()                   /* 2, FrPdListenTimeout */
#define FR_CIDX_GMACROPERCYCLE()                    /* 3, FrIfGMacroPerCycle */
#define FR_CIDX_GDMACROTICK()                       /* 4, FrIfGdMacrotick */
#define FR_CIDX_GNUMBEROFMINISLOTS()                /* 5, FrIfGNumberOfMinislots */
#define FR_CIDX_GNUMBEROFSTATICSLOTS()              /* 6, FrIfGNumberOfStaticSlots */
#define FR_CIDX_GDNIT()                             /* 7, FrIfGdNit */
#define FR_CIDX_GDSTATICSLOT()                      /* 8, FrIfGdStaticSlot */
#define FR_CIDX_GDWAKEUPRXWINDOW()                  /* 9, FrIfGdWakeupRxWindow */
#define FR_CIDX_PKEYSLOTID()                        /* 10, FrPKeySlotId */
#define FR_CIDX_PLATESTTX()                         /* 11, FrPLatestTx */
#define FR_CIDX_POFFSETCORRECTIONOUT()              /* 12, FrPOffsetCorrectionOut */
#define FR_CIDX_POFFSETCORRECTIONSTART()            /* 13, FrPOffsetCorrectionStart */
#define FR_CIDX_PRATECORRECTIONOUT()                /* 14, FrPRateCorrectionOut */
#define FR_CIDX_PSECONDKEYSLOTID()                  /* 15, FrPSecondKeySlotId */
#define FR_CIDX_PDACCEPTEDSTARTUPRANGE()            /* 16, FrPdAcceptedStartupRange */
#define FR_CIDX_GCOLDSTARTATTEMPTS()                /* 17, FrIfGColdStartAttempts */
#define FR_CIDX_GCYCLECOUNTMAX()                    /* 18, FrIfGCycleCountMax */
#define FR_CIDX_GLISTENNOISE()                      /* 19, FrIfGListenNoise */
#define FR_CIDX_GMAXWITHOUTCLOCKCORRECTFATAL()      /* 20, FrIfGMaxWithoutClockCorrectFatal */
#define FR_CIDX_GMAXWITHOUTCLOCKCORRECTPASSIVE()    /* 21, FrIfGMaxWithoutClockCorrectPassive */
#define FR_CIDX_GNETWORKMANAGEMENTVECTORLENGTH()    /* 22, FrIfGNetworkManagementVectorLength */
#define FR_CIDX_GPAYLOADLENGTHSTATIC()              /* 23, FrIfGPayloadLengthStatic */
#define FR_CIDX_GSYNCFRAMEIDCOUNTMAX()              /* 24, FrIfGSyncFrameIDCountMax */
#define FR_CIDX_GDACTIONPOINTOFFSET()               /* 25, FrIfGdActionPointOffset */
#define FR_CIDX_GDBIT()                             /* 26, FrIfGdBit */
#define FR_CIDX_GDCASRXLOWMAX()                     /* 27, FrIfGdCasRxLowMax */
#define FR_CIDX_GDDYNAMICSLOTIDLEPHASE()            /* 28, FrIfGdDynamicSlotIdlePhase */
#define FR_CIDX_GDMINISLOTACTIONPOINTOFFSET()       /* 29, FrIfGdMiniSlotActionPointOffset */
#define FR_CIDX_GDMINISLOT()                        /* 30, FrIfGdMinislot */
#define FR_CIDX_GDSAMPLECLOCKPERIOD()               /* 31, FrIfGdSampleClockPeriod */
#define FR_CIDX_GDSYMBOLWINDOW()                    /* 32, FrIfGdSymbolWindow */
#define FR_CIDX_GDSYMBOLWINDOWACTIONPOINTOFFSET()   /* 33, FrIfGdSymbolWindowActionPointOffset */
#define FR_CIDX_GDTSSTRANSMITTER()                  /* 34, FrIfGdTssTransmitter */
#define FR_CIDX_GDWAKEUPRXIDLE()                    /* 35, FrIfGdWakeupRxIdle */
#define FR_CIDX_GDWAKEUPRXLOW()                     /* 36, FrIfGdWakeupRxLow */
#define FR_CIDX_GDWAKEUPTXACTIVE()                  /* 37, FrIfGdWakeupTxActive */
#define FR_CIDX_GDWAKEUPTXIDLE()                    /* 38, FrIfGdWakeupTxIdle */
#define FR_CIDX_PALLOWPASSIVETOACTIVE()             /* 39, FrPAllowPassiveToActive */
#define FR_CIDX_PCHANNELS()                         /* 40, FrPChannels */
#define FR_CIDX_PCLUSTERDRIFTDAMPING()              /* 41, FrPClusterDriftDamping */
#define FR_CIDX_PDECODINGCORRECTION()               /* 42, FrPDecodingCorrection */
#define FR_CIDX_PDELAYCOMPENSATIONA()               /* 43, FrPDelayCompensationA */
#define FR_CIDX_PDELAYCOMPENSATIONB()               /* 44, FrPDelayCompensationB */
#define FR_CIDX_PMACROINITIALOFFSETA()              /* 45, FrPMacroInitialOffsetA */
#define FR_CIDX_PMACROINITIALOFFSETB()              /* 46, FrPMacroInitialOffsetB */
#define FR_CIDX_PMICROINITIALOFFSETA()              /* 47, FrPMicroInitialOffsetA */
#define FR_CIDX_PMICROINITIALOFFSETB()              /* 48, FrPMicroInitialOffsetB */
#define FR_CIDX_PPAYLOADLENGTHDYNMAX()              /* 49, FrPPayloadLengthDynMax */
#define FR_CIDX_PSAMPLESPERMICROTICK()              /* 50, FrPSamplesPerMicrotick */
#define FR_CIDX_PWAKEUPCHANNEL()                    /* 51, FrPWakeupChannel */
#define FR_CIDX_PWAKEUPPATTERN()                    /* 52, FrPWakeupPattern */
#define FR_CIDX_PDMICROTICK()                       /* 53, FrPdMicrotick */
#define FR_CIDX_GDIGNOREAFTERTX()                   /* 54, FrIfGdIgnoreAfterTx */
#define FR_CIDX_PALLOWHALTDUETOCLOCK()              /* 55, FrPAllowHaltDueToClock */
#define FR_CIDX_PEXTERNALSYNC()                     /* 56, FrPExternalSync */
#define FR_CIDX_PFALLBACKINTERNAL()                 /* 57, FrPFallBackInternal */
#define FR_CIDX_PKEYSLOTONLYENABLED()               /* 58, FrPKeySlotOnlyEnabled */
#define FR_CIDX_PKEYSLOTUSEDFORSTARTUP()            /* 59, FrPKeySlotUsedForStartup */
#define FR_CIDX_PKEYSLOTUSEDFORSYNC()               /* 60, FrPKeySlotUsedForSync */
#define FR_CIDX_PNMVECTOREARLYUPDATE()              /* 61, FrPNmVectorEarlyUpdate */
#define FR_CIDX_PTWOKEYSLOTMODE()                   /* 62, FrPTwoKeySlotMode */
#endif

// Controller configuration
typedef struct {
        uint32  FrCtrlIdx;
        boolean FrPAllowHaltDueToClock;
        uint32  FrPAllowPassiveToActive;
        Fr_ChannelType FrPChannels;
        uint32  FrPClusterDriftDamping;
        uint32  FrPDecodingCorrection;
        uint32  FrPDelayCompensationA;
        uint32  FrPDelayCompensationB;
        boolean FrPExternalSync;
        boolean FrPFallBackInternal;
        uint32  FrPKeySlotId;
        boolean FrPKeySlotOnlyEnabled;
        boolean FrPKeySlotUsedForStartup;
        boolean FrPKeySlotUsedForSync;
        uint32  FrPLatestTx;
        uint32  FrPMacroInitialOffsetA;
        uint32  FrPMacroInitialOffsetB;
        uint32  FrPMicroInitialOffsetA;
        uint32  FrPMicroInitialOffsetB;
        uint32  FrPMicroPerCycle;
        uint32  FrPNmVectorEarlyUpdate;
        uint32  FrPOffsetCorrectionOut;
        uint32  FrPOffsetCorrectionStart;
        uint32  FrPPayloadLengthDynMax;
        uint32  FrPRateCorrectionOut;
        Fr_SamplePerMicroTickType FrPSamplesPerMicrotick;
        uint32  FrPSecondKeySlotId;
        boolean FrPTwoKeySlotMode;
        Fr_ChannelType FrPWakeupChannel;
        uint32  FrPWakeupPattern;
        uint32  FrPdAcceptedStartupRange;
        uint32  FrPdListenTimeout;
        Fr_MicroTickType FrPdMicrotick;
        uint8  FrArcAbsTimerMaxIdx;
#if defined(USE_DEM) || defined(CFG_FR_DEM_TEST)
        Dem_EventIdType FrDemEventParamRef;
#endif
        //FrFifo -- Not supported yet
}Fr_CtrlConfigParametersType;

typedef struct {
    uint32 FrTrigObjectId;
    boolean FrTrigAllowDynamicLen;
    boolean FrTrigAlwaysTransmit;
    uint32  FrTrigBaseCycle;
    Fr_ChannelType  FrTrigChannel;
    uint32  FrTrigCycleRepetition;
    boolean FrTrigPayloadPreamble;
    uint32  FrTrigSlotId;
    uint32  FrTrigLSduLength;
    uint32 FrTrigIsTx;
#if defined(USE_DEM) || defined(CFG_FR_DEM_TEST)
    Dem_EventIdType FrTrigDemFTSlotStatusRef;
#endif
}Fr_FrIfTriggeringConfType;

typedef struct {
    uint16 FrMsgBufferIdx;
    uint32 FrDataPartitionAddr;
    uint32 FrCurrentLengthSetup;
}Fr_MessageBufferConfigType;

typedef struct {
    uint32 FrNbrTrigConfiged;
    const Fr_FrIfTriggeringConfType *FrTrigConfPtr;
    Fr_MessageBufferConfigType *FrMsgBufferCfg;
}Fr_FrIfCCTriggeringType;


typedef struct {
    boolean FrLpdIsReconf;
    uint32 FrLpdTriggIdx;
}Fr_FrIfLPduType;


typedef struct {
    const uint16 FrNbrLPdusConfigured;
    const Fr_FrIfLPduType *FrLpdu;
}Fr_FrIfLPduContainerType;

typedef struct {
    uint32 FrClusterGColdStartAttempts;
    uint32 FrClusterGListenNoise;
    uint32 FrClusterGMaxWithoutClockCorrectPassive;
    uint32 FrClusterGMaxWithoutClockCorrectFatal;
    uint32 FrClusterGNetworkManagementVectorLength;
    uint32 FrClusterGdTSSTransmitter;
    uint32 FrClusterGdCasRxLowMax;
    Fr_MicroTickType FrClusterGdSampleClockPeriod;
    uint32 FrClusterGdSymbolWindow;
    uint32 FrClusterGdWakeupRxIdle;
    uint32 FrClusterGdWakeupRxLow;
    uint32 FrClusterGdWakeupTxIdle;
    uint32 FrClusterGdWakeupTxActive;
    uint32 FrClusterGPayloadLengthStatic;
    uint32 FrClusterGMacroPerCycle;
    uint32 FrClusterGSyncFrameIDCountMax;
    uint32 FrClusterGdNit;
    uint32 FrClusterGdStaticSlot;
    uint32 FrClusterGNumberOfStaticSlots;
    uint32 FrClusterGdMiniSlotActionPointOffset;
    uint32 FrClusterGNumberOfMinislots;
    uint32 FrClusterGdActionPointOffset;
    uint32 FrClusterGdDynamicSlotIdlePhase;
    uint32 FrClusterGdMinislot;
    uint8 FrClusterGCycleCountMax;
    Fr_MicroTickType FrClusterGdBit;
    float32 FrClusterGdCycle;
    uint32 FrClusterGdIgnoreAfterTx;
    float32 FrClusterGdMacrotick;
    uint32 FrClusterGdSymbolWindowActionPointOffset;
    uint32 FrClusterGdWakeupRxWindow;
}Fr_FrIfClusterConfigType;


#endif /*FR_GENERAL_TYPES*/
