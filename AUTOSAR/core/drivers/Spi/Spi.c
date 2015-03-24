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

/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */
/** @tagSettings DEFAULT_ARCHITECTURE=PPC */

/* ----------------------------[information]----------------------------------*/
/*
 * Author: mahi
 *
 * Description:
 *   Implements the SPI driver module
 */

//*PC-Lint Exception */
/*lint -save -e613 -e794 Our asserts check for null pointers.*/
/* ----------------------------[includes]------------------------------------*/

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "Spi_Internal.h"
#include "Mcu.h"
#include "math.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
#include "isr.h"
#endif

#if (SPI_MAX_JOB > 32)
#error Spi: The maximum number of 32 jobs is supported
#endif
/* ----------------------------[private define]------------------------------*/
#define USE_DIO_CS          STD_OFF  /* Not supported yet. */

#define MODULE_NAME 	"/driver/Spi"

#include "Ramlog.h"
//#define USE_LOCAL_RAMLOG
#if defined(USE_LOCAL_RAMLOG)
#define RAMLOG_STR(_x) ramlog_str(_x)
#define RAMLOG_DEC(_x) ramlog_dec(_x)
#define RAMLOG_HEX(_x) ramlog_hex(_x)
#else
#define RAMLOG_STR(_x)
#define RAMLOG_DEC(_x)
#define RAMLOG_HEX(_x)
#endif

/* ----------------------------[private macro]-------------------------------*/

//#define USE_LDEBUG_PRINTF
#include "debug.h"

/* Development error macros. */
#if ( SPI_DEV_ERROR_DETECT == STD_ON )
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(SPI_MODULE_ID,0,_api,_err); \
          return; \
        }

#define VALIDATE_W_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(SPI_MODULE_ID,0,_api,_err); \
          return (_rv); \
        }
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_W_RV(_exp,_api,_err,_rv )
#endif

/* ----------------------------[private typedef]-----------------------------*/

typedef struct {
    Spi_SeqResultType seqResult;
} Spi_SeqUnitType;


/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/

Spi_GlobalType  Spi_Global = { 0 };
Spi_UnitType    Spi_Unit[SPI_CONTROLLER_CNT];
Spi_SeqUnitType Spi_SeqUnit[SPI_MAX_SEQUENCE];
Spi_JobUnitType Spi_JobUnit[SPI_MAX_JOB];
uint8           Spi_CtrlToUnit[6]; /*lint !e9003 It can't be defined at block scope */

/* ----------------------------[private functions]---------------------------*/

static Std_ReturnType Spi_SeqWrite(Spi_SequenceType seqIndex, Spi_CallTypeType callType);
static void Spi_FailAll( Spi_UnitType *uPtr );


//-------------------------------------------------------------------

/**
 * Sets a result for a sequence
 *
 * @param Sequence The sequence to set the result for
 * @param result The result to set.
 */
static void Spi_SetSequenceResult(Spi_SequenceType Sequence, Spi_SeqResultType result) {
    Spi_SeqUnit[Sequence].seqResult = result;
}

/**
 * Gets the Rx buffer for a channel.
 *
 * @param ch
 * @param length
 * @return pointer to the buffer
 */
Spi_DataBufferType *Spi_Internal_GetRxChBuf(Spi_ChannelType ch, Spi_NumberOfDataType *length) {
    Spi_DataBufferType *buf;
    if ((Spi_Global.configPtr->SpiChannelConfig[ch].SpiChannelType == SPI_EB)
            && (Spi_Global.extBufPtr[ch].active == 1))
    {
        *length = Spi_Global.extBufPtr[ch].length;
        buf = Spi_Global.extBufPtr[ch].dest;

    }
    else {
        /* No support */
        SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
        *length = 0;
        buf = NULL;
    }
    return buf;
}

/**
 * Gets the Tx buffer for a channel.
 *
 * @param ch
 * @param length
 * @return pointer to the buffer
 */
const Spi_DataBufferType *Spi_Internal_GetTxChBuf(Spi_ChannelType ch, Spi_NumberOfDataType *length) {
    const Spi_DataBufferType *buf;
    if ((Spi_Global.configPtr->SpiChannelConfig[ch].SpiChannelType == SPI_EB)
            && (Spi_Global.extBufPtr[ch].active == 1))
    {
        *length = Spi_Global.extBufPtr[ch].length;
        buf = Spi_Global.extBufPtr[ch].src;
    }
    else {
        /* No support */
        SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
        *length = 0;
        buf = NULL;
    }
    return buf;
}

uint32 Spi_Internal_GetTxDefValue(Spi_ChannelType ch) {
    return Spi_Global.configPtr->SpiChannelConfig[ch].SpiDefaultData;
}

/* ----------------------------[public functions]----------------------------*/

#if defined(__DMA_INT)
static void Spi_Isr_DMA( void )
{
    // Clear interrupt
    Dma_ClearInterrupt(5);
}
#endif

static void Spi_JobWrite(Spi_UnitType *uPtr);



static void Spi_FailAll( Spi_UnitType *uPtr )  {
	assert((uPtr!=NULL) && (uPtr->seqPtr!=NULL) && (uPtr->jobPtr!=NULL));
    uint32 i = uPtr->jobIdxInSeq; /*lint !e632 Assignment: uint32 = uint8 ok */

    /* @req SWS_Spi_00057 */
    /* @req SWS_Spi_00119 */
    /* @req SWS_Spi_00118 */

    /* Fail both job and sequence */
    Spi_SetHWUnitStatus(uPtr, SPI_IDLE);
    /* @req SWS_Spi_00293 */

    /* Fail all the jobs in that sequence */
    while (uPtr->seqPtr->SpiJobAssignment[i] != NULL ) {
        Spi_SetJobResult(uPtr->jobPtr->SpiJobId, SPI_JOB_FAILED);
        i++;
    }

    Spi_SetSequenceResult(uPtr->seqPtr->SpiSequenceId, SPI_SEQ_FAILED);
    /* @req SWS_Spi_00120 */
    /* @req SWS_Spi_00281 */
    if (uPtr->seqPtr->SpiSeqEndNotification != NULL) {
        uPtr->seqPtr->SpiSeqEndNotification();
    }
}

/**
 * Function to see if two sequences share jobs
 *
 * @param seq1 - Seqence 1
 * @param seq2 - Seqence 2
 * @return 0  - if the don't share any jobs
 *        !=0 - if they share jobs
 */

static boolean Spi_ShareJobs(Spi_SequenceType seq1, Spi_SequenceType seq2) {
    uint32 seqMask1 = 0;
    uint32 seqMask2 = 0;
    uint32 i = 0;

    const Spi_SequenceConfigType *seqConfig;

    /* Search for jobs in sequence 1 */
    seqConfig = Config_GetSequence(seq1);

    while (seqConfig->SpiJobAssignment[i] != NULL ) {
        if (seqConfig->SpiJobAssignment[i]->SpiJobId <= 31uL) {
            seqMask1 |= (1uL << seqConfig->SpiJobAssignment[i]->SpiJobId);
        }
        else {
            SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
        }
        i++;
    }

    /* Search for jobs in sequence 2 */
    seqConfig = Config_GetSequence(seq2);

    i = 0;
    while (seqConfig->SpiJobAssignment[i] != NULL ) {
        if (seqConfig->SpiJobAssignment[i]->SpiJobId <= 31uL) {
            seqMask2 |= (1uL << seqConfig->SpiJobAssignment[i]->SpiJobId);
        }
        else {
            SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
        }
        i++;
    }

    return (seqMask1 & seqMask2)!=0; /*lint !e632 This is boolean. */
}


//-------------------------------------------------------------------

/* !req SWS_Spi_00177 !req SWS_Spi_00304 !req SWS_Spi_00305 */
/* !req SWS_Spi_00306 !req SWS_Spi_00307 !req SWS_Spi_00018 */
/* !req SWS_Spi_00024 !req SWS_Spi_00023 !req SWS_Spi_00137 */

Std_ReturnType Spi_WriteIB(Spi_ChannelType Channel, const Spi_DataBufferType *DataBufferPtr) {
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_WRITEIB_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    VALIDATE_W_RV(( Channel<SPI_MAX_CHANNEL ), SPI_WRITEIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK);
    VALIDATE_W_RV(( DataBufferPtr != NULL ), SPI_WRITEIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK);
    VALIDATE_W_RV((SPI_IB == Spi_Global.configPtr->SpiChannelConfig[Channel].SpiChannelType), SPI_WRITEIB_SERVICE_ID,
            SPI_E_PARAM_CHANNEL, E_NOT_OK);

    /* According to SPI051 it seems that we only have to a "depth" of 1 */

    Std_ReturnType rv = E_NOT_OK;
    return rv;
}

//-------------------------------------------------------------------
/* @req SWS_Spi_00299 */
void Spi_Init(const Spi_ConfigType *ConfigPtr) {
    static Spi_EbType      Spi_Eb[SPI_MAX_CHANNEL];    /*lint !e9046 */
    uint32 confMask;
    uint32 ctrlNr;

    VALIDATE(( NULL != ConfigPtr ), SPI_INIT_SERVICE_ID, SPI_E_PARAM_POINTER);
    /* @req SWS_Spi_00246 */
    VALIDATE(( TRUE != Spi_Global.initRun), SPI_INIT_SERVICE_ID, SPI_E_ALREADY_INITIALIZED);

    /* @req SWS_Spi_00082 */
    memset(&Spi_Global, 0, sizeof(Spi_Global));
    Spi_Global.configPtr = ConfigPtr;
    Spi_Global.extBufPtr = Spi_Eb;

    /* @req SWS_Spi_00151 */
    Spi_Global.asyncMode = SPI_POLLING_MODE;

    /* Set sequences to SPI_SEQ_OK */
    for (Spi_SequenceType i = (Spi_SequenceType) 0; i < SPI_MAX_SEQUENCE; i++) {
        Spi_SetSequenceResult(i, SPI_SEQ_OK);
    }

    /* Set jobs to SPI_JOB_OK */
    for (Spi_JobType j = 0; j < SPI_MAX_JOB; j++) {
        Spi_SetJobResult(j, SPI_JOB_OK);
    }

    confMask = SPI_CHANNELS_CONFIGURED;     /*lint !e835 */

    for (uint8 i = 0; i < SPI_CONTROLLER_CNT; i++) {
        ctrlNr = ((uint32)ffs(confMask) - 1uL);	/*lint !e713 For ghs the argument is signed but the function looks at bits so it's ok */
        confMask &= ~(1uL << ctrlNr); /* clear mask */
        Spi_CtrlToUnit[ctrlNr] = i;
        /* @req SWS_Spi_00013 */
        Spi_SetHWUnitStatus(&Spi_Unit[i], SPI_IDLE);
    }

    Spi_Hw_Init(ConfigPtr);

    /* @req SWS_Spi_00015 */

    Spi_Global.initRun = TRUE;
}

//-------------------------------------------------------------------

Std_ReturnType Spi_DeInit(void) {
    uint32 confMask;
    uint32 ctrlNr;
    Spi_UnitType *uPtr;

    /* @req SWS_Spi_00303 */

    /* @req SWS_Spi_00302 */
    /* @req SWS_Spi_00253 */
    /* @req SWS_Spi_00046 */
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_DEINIT_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    if (Spi_GetStatus() == SPI_BUSY) {
        return E_NOT_OK ;
    }

    // Disable the HW modules
    /* @req SWS_Spi_00021 */
    confMask = SPI_CHANNELS_CONFIGURED;     /*lint !e835 */

    // Disable the SPI hw
    /* @req SWS_Spi_00252 */

    for (uint8 i = 0; i < SPI_CONTROLLER_CNT; i++) {
        ctrlNr = (uint32)ffs(confMask) - 1uL; /*lint !e713 For ghs the argument is signed but the function looks at bits so it's ok */
        confMask &= ~(1uL << ctrlNr); /* clear mask */

        uPtr = &Spi_Unit[i];

        Spi_Hw_DeInitController(ctrlNr);
        Spi_SetHWUnitStatus(uPtr, SPI_UNINIT);
    }

    /* @req SWS_Spi_00022 */
    Spi_Global.configPtr = NULL;
    Spi_Global.initRun = FALSE;

    /* @req SWS_Spi_00301 */
    return E_OK ;
}

/**
 * Write a job to the SPI bus
 *
 * @param uPtr Pointer to a structure representing a Spi controller unit
 */
static void Spi_JobWrite(Spi_UnitType *uPtr) {
	assert((uPtr!=NULL) && (uPtr->jobPtr!=NULL));
    sint32 rv;
    /* @req SWS_Spi_00286 */
    Spi_SetJobResult(uPtr->jobPtr->SpiJobId, SPI_JOB_PENDING);

    LDEBUG_PRINTF("Spi_JobWrite: JobId=%d(index=%d)\n",uPtr->jobPtr->SpiJobId,uPtr->jobIdxInSeq);

    uPtr->rxChIdxInJob = 0;
    uPtr->txChIdxInJob = 0;

#if (USE_DIO_CS == STD_ON)
    if( cb != NULL ) {
        cb(1);
    }
#endif
    /* @req SWS_Spi_00330 */
    rv = Spi_Hw_Tx(uPtr);

    if (rv == SPIE_BAD) {
        Spi_FailAll(uPtr);
    }
}

/**
 * Write a sequence to the SPI bus
 *
 * @param seqIndex The sequence
 * @param callType Async or Sync
 */
static Std_ReturnType Spi_SeqWrite(Spi_SequenceType seqIndex, Spi_CallTypeType callType) {
    Spi_JobResultType            jobResult;
    const Spi_SequenceConfigType *seqConfig = Config_GetSequence(seqIndex);
    Spi_UnitType                 *uPtr      = GET_SPI_UNIT_PTR(seqConfig->SpiJobAssignment[0]->SpiHwUnit);

    LDEBUG_PRINTF("Spi_SeqWrite: SeqId=%d\n", seqIndex);

    /*  Set JobResult */

    /* @req SWS_Spi_00194 */
    if( callType == SPI_ASYNC_CALL ) {
        jobResult = SPI_JOB_QUEUED;
        if( uPtr->status == SPI_BUSY ) {
            /* Queue the request for later */
            if( QueuePush(&uPtr->queueHead,seqIndex) != 0uL ) {
                return E_NOT_OK;
            }
        }
    } else {
        jobResult = SPI_JOB_PENDING;
        if( uPtr->status == SPI_BUSY ) {
            /* Transmitting on the same bus -> same controller -> SWS_Spi_00135 */
            SPI_DET_REPORT_ERROR(SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_SEQ_IN_PROCESS);

            /* Note!
             * Seems that we don't need to set the sequence result here since
             * we return E_NOT_OK. See SWS_Spi_00254 */
            return E_NOT_OK;
        }
    }

    /* @req SWS_Spi_00194 */
    /* @req SWS_Spi_00157 */
    for (uint32 i = 0; seqConfig->SpiJobAssignment[i] != NULL; i++) {
        Spi_SetJobResult(seqConfig->SpiJobAssignment[i]->SpiJobId, jobResult);
    }

    /* @req SWS_Spi_00285 */
    Spi_SetSequenceResult(seqIndex, SPI_SEQ_PENDING);

    if( uPtr->status == SPI_BUSY ) {
        /* We have queue, so not more to do */
        return E_OK;
    }

    uPtr->callType = callType;
    uPtr->seqPtr = seqConfig;
    uPtr->jobPtr = seqConfig->SpiJobAssignment[0];
    uPtr->jobIdxInSeq = 0;

    /* @req SWS_Spi_00020 */
    /* @req SWS_Spi_00134 */
    Spi_SetHWUnitStatus(uPtr, SPI_BUSY);

    Spi_JobWrite(uPtr);

    /* Busy wait */
    if (uPtr->callType == SPI_SYNC_CALL) {
        while (Spi_GetSequenceResult(seqIndex) == SPI_SEQ_PENDING) {
            if (Spi_Hw_IsTransmissionDone((uint32)uPtr->hwUnit)) {
                Spi_Internal_Isr(uPtr);
            }
        }
    }
    return E_OK;
}

//-------------------------------------------------------------------
static boolean Spi_AnyPendingJobs(Spi_SequenceType Sequence) {

    // Check that we don't share any jobs with another sequence that is SPI_SEQ_PENDING
    for (Spi_SequenceType i = 0; i < SPI_MAX_SEQUENCE; i++) {
        if (i == Sequence) {
            continue;
        }

        if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
            // We have found a pending sequence... check that we don't share any jobs
            // with that sequence
            if (Spi_ShareJobs(Sequence, i)) {
                return TRUE ;
            }
        }
    }

    return FALSE ;
}

//-------------------------------------------------------------------

/**
 * Blocking write
 *
 * @param Sequence The sequence
 * @return E_OK When the command has been accepted.
 */
#if (SPI_LEVEL_DELIVERED == 0) || (SPI_LEVEL_DELIVERED == 2)
Std_ReturnType Spi_SyncTransmit(Spi_SequenceType Sequence) {
    Std_ReturnType rv;

    /* @req SWS_Spi_00329 */
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    /* @req SWS_Spi_00032 */
    VALIDATE_W_RV(( SPI_MAX_SEQUENCE>Sequence ), SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_PARAM_SEQ, E_NOT_OK);


    if (Spi_GetSequenceResult(Sequence) == SPI_SEQ_PENDING) {
        /* @req SWS_Spi_00329 */
        return E_NOT_OK ;
    }

    /* @req SWS_Spi_00135*/
#if (SPI_SUPPORT_CONCURRENT_SYNC_TRANSMIT == STD_OFF)
    for (Spi_SequenceType i = 0; i < SPI_MAX_SEQUENCE; i++) {
        if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
            SPI_DET_REPORT_ERROR(SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_SEQ_IN_PROCESS);
            return E_NOT_OK ;
        }
    }
#endif

    if (SPI_SEQ_OK != Spi_GetSequenceResult(Sequence)) {
        SPI_DET_REPORT_ERROR(SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_SEQUENCE_NOT_OK);
        return E_NOT_OK ;
    }
    if (Spi_AnyPendingJobs(Sequence)) {
        return E_NOT_OK ;
    }

    rv = Spi_SeqWrite(Sequence, SPI_SYNC_CALL);

    /* @req SWS_Spi_00309 */
    return rv ;
}
#endif
//-------------------------------------------------------------------
/* @req SWS_Spi_00133 */
#if (SPI_LEVEL_DELIVERED == 1) || (SPI_LEVEL_DELIVERED == 2)
Std_ReturnType Spi_AsyncTransmit(Spi_SequenceType Sequence) {
    Spi_SeqResultType isSeqPending;
    boolean anyJobPending;
    Std_ReturnType rv;


    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    VALIDATE_W_RV(( SPI_MAX_SEQUENCE>Sequence ), SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_PARAM_SEQ, E_NOT_OK);

    isSeqPending = Spi_GetSequenceResult(Sequence);
    anyJobPending = Spi_AnyPendingJobs(Sequence);

    /* @req SWS_Spi_00081 */
    /* @req SWS_Spi_00086 */
    if ((isSeqPending == SPI_SEQ_PENDING) || anyJobPending) {
        /* @req SWS_Spi_00266 */
        SPI_DET_REPORT_ERROR(SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_SEQ_PENDING);
        return E_NOT_OK ;
    }

    if (SPI_SEQ_OK != Spi_GetSequenceResult(Sequence)) {
        SPI_DET_REPORT_ERROR(SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_SEQUENCE_NOT_OK);
        return E_NOT_OK ;
    }

    rv = Spi_SeqWrite(Sequence, SPI_ASYNC_CALL);

    return rv;
}
#endif
//-------------------------------------------------------------------

Std_ReturnType Spi_ReadIB(Spi_ChannelType Channel, Spi_DataBufferType* DataBufferPtr) {
    (void) DataBufferPtr;   /*lint !e920 */

    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_READIB_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    VALIDATE_W_RV(( Channel<SPI_MAX_CHANNEL ), SPI_READIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK);
    VALIDATE_W_RV((SPI_IB == Spi_Global.configPtr->SpiChannelConfig[Channel].SpiChannelType), SPI_READIB_SERVICE_ID,
            SPI_E_PARAM_CHANNEL, E_NOT_OK);

    /* NOT SUPPORTED */

    Std_ReturnType rv = E_NOT_OK;
    return rv;
}

//-------------------------------------------------------------------
/* @req SWS_Spi_00317 */
Std_ReturnType Spi_SetupEB(Spi_ChannelType Channel, const Spi_DataBufferType* SrcDataBufferPtr,
        Spi_DataBufferType* DesDataBufferPtr, Spi_NumberOfDataType Length) {
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_SETUPEB_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    /* @req SWS_Spi_00031 */
    VALIDATE_W_RV(( Channel<SPI_MAX_CHANNEL ), SPI_SETUPEB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK);
#if ( SPI_CHANNEL_BUFFERS_ALLOWED == 1 )
    VALIDATE_W_RV((SPI_EB == Spi_Global.configPtr->SpiChannelConfig[Channel].SpiChannelType), SPI_SETUPEB_SERVICE_ID,
            SPI_E_PARAM_CHANNEL, E_NOT_OK);
#endif

    /* @req SWS_Spi_00060 */
    VALIDATE_W_RV((Length <= Spi_Global.configPtr->SpiChannelConfig[Channel].SpiEbMaxLength), SPI_SETUPEB_SERVICE_ID,
            SPI_E_PARAM_CHANNEL, E_NOT_OK);

    const Spi_ChannelConfigType *chConfig = &Spi_Global.configPtr->SpiChannelConfig[Channel];
    Std_ReturnType ret = E_OK;
    /* @req SWS_Spi_00318 */
    /* @req SWS_Spi_00028 */
    /* @req SWS_Spi_00067 */
    Spi_EbType *extChBuff = &Spi_Global.extBufPtr[Channel];
    if (chConfig->SpiChannelType == SPI_EB) {
        extChBuff->src = SrcDataBufferPtr;
        extChBuff->dest = DesDataBufferPtr;
        extChBuff->length = Length;
        extChBuff->active = 1;
    }
    else {
        /* NOT SUPPORTED */
        SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
        ret = E_NOT_OK;
    }

    return ret;
}

//-------------------------------------------------------------------
/* @req SWS_Spi_00181 */
/* @req SWS_Spi_00320 */
/* @req SWS_Spi_00025 */
Spi_StatusType Spi_GetStatus(void) {
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_GETSTATUS_SERVICE_ID, SPI_E_UNINIT, SPI_UNINIT);

    // Check all sequences if they have any job pending
    for (Spi_SequenceType i = 0; i < SPI_MAX_SEQUENCE; i++) {
        if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
            return SPI_BUSY;
        }
    }

    return SPI_IDLE;
}

//-------------------------------------------------------------------

/* @req SWS_Spi_00182 */

Spi_JobResultType Spi_GetJobResult(Spi_JobType Job) {
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_GETJOBRESULT_SERVICE_ID, SPI_E_UNINIT, SPI_JOB_FAILED);
    VALIDATE_W_RV(!( SPI_MAX_JOB<=Job ), SPI_GETJOBRESULT_SERVICE_ID, SPI_E_PARAM_JOB, SPI_JOB_FAILED);

    /* @req SWS_Spi_00322 */
    /* @req SWS_Spi_00026 */
    return Spi_JobUnit[Job].jobResult;
}

//-------------------------------------------------------------------

/* @req SWS_Spi_00183 */
Spi_SeqResultType Spi_GetSequenceResult(Spi_SequenceType Sequence) {

    /* @req SWS_Spi_00324 */
    /* @req SWS_Spi_00039 */
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_GETSEQUENCERESULT_SERVICE_ID, SPI_E_UNINIT, SPI_SEQ_FAILED);
    VALIDATE_W_RV(( SPI_MAX_SEQUENCE>Sequence ), SPI_GETSEQUENCERESULT_SERVICE_ID, SPI_E_PARAM_SEQ, SPI_SEQ_FAILED);

    return Spi_SeqUnit[Sequence].seqResult;

}

//-------------------------------------------------------------------
#if (SPI_HW_STATUS_API == STD_ON)
/* @req SWS_Spi_00186 */
/* @req SWS_Spi_00142 */
Spi_StatusType Spi_GetHWUnitStatus(Spi_HWUnitType HWUnit) {
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_GETHWUNITSTATUS_SERVICE_ID, SPI_E_UNINIT, SPI_UNINIT);
    /* @req SWS_Spi_00143 */
    /* @req SWS_Spi_00288 */
    VALIDATE_W_RV(!(SPI_CONTROLLER_CNT <= HWUnit), SPI_GETHWUNITSTATUS_SERVICE_ID, SPI_E_PARAM_UNIT, SPI_UNINIT);

    /* @req SWS_Spi_00332 */
    /* @req SWS_Spi_00141 */
    return (GET_SPI_UNIT_PTR(HWUnit))->status;
}
#endif
//-------------------------------------------------------------------

#if (SPI_CANCEL_API == STD_ON )
void Spi_Cancel(Spi_SequenceType Sequence) {
    VALIDATE(( TRUE == Spi_Global.initRun ), SPI_CANCEL_SERVICE_ID, SPI_E_UNINIT);
    VALIDATE(( SPI_MAX_SEQUENCE<Sequence ), SPI_CANCEL_SERVICE_ID, SPI_E_PARAM_SEQ);

    /* NOT SUPPORTED */
}
#endif

//-------------------------------------------------------------------

/* @req SWS_Spi_00154 */
#if ( SPI_LEVEL_DELIVERED == 2)
/* @req SWS_Spi_00188 */
Std_ReturnType Spi_SetAsyncMode(Spi_AsyncModeType Mode) {
    Std_ReturnType retval = E_OK;
    boolean hwUnitBusy = false;
    Spi_CallTypeType currentTransmissionMode = SPI_SYNC_CALL;
    uint32 confMask;
    uint32 ctrlNr;

    /* @req SWS_Spi_00337 */
    VALIDATE_W_RV(( TRUE == Spi_Global.initRun ), SPI_SETASYNCMODE_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK);
    VALIDATE_W_RV(( (SPI_POLLING_MODE == Mode) || (SPI_INTERRUPT_MODE == Mode) ), SPI_SETASYNCMODE_SERVICE_ID, SPI_E_UNEXPECTED_EXECUTION, E_NOT_OK);

    confMask = SPI_CHANNELS_CONFIGURED; /*lint !e835 */

    for (uint32_t i = 0; i < SPI_CONTROLLER_CNT; i++) {
        ctrlNr = (uint32)ffs(confMask) - 1uL; /*lint !e713 For ghs the argument is signed but the function looks at bits so it's ok */
        confMask &= ~(1uL << ctrlNr); /* clear mask */

        if (Spi_Unit[ctrlNr].status == SPI_BUSY) {
            hwUnitBusy = true;
            currentTransmissionMode = Spi_Unit[i].callType;
        }
    }

    if ((currentTransmissionMode == SPI_ASYNC_CALL) && hwUnitBusy) {
        /* @req SWS_Spi_00171 */
        /* @req SWS_Spi_00337 */
        retval = E_NOT_OK;
    }
    else {
        /* @req SWS_Spi_00338 */
        /* @req SWS_Spi_00172 */
        Spi_Global.asyncMode = Mode;
        retval = E_OK;
    }

    /* @req SWS_Spi_00336 */
    return retval;
}
#endif

//-------------------------------------------------------------------

void Spi_MainFunction_Handling(void) {
    uint32 confMask;
    uint32 ctrlNr;
    boolean xDone;

    if (Spi_Global.asyncMode == SPI_POLLING_MODE) {

        confMask = SPI_CHANNELS_CONFIGURED; /*lint !e835 */

        for (uint32_t i = 0; i < SPI_CONTROLLER_CNT; i++) {
            ctrlNr = (uint32)ffs(confMask) - 1uL; /*lint !e713 For ghs the argument is signed but the function looks at bits so it's ok */
            confMask &= ~(1uL << ctrlNr); /* clear mask */

            xDone = Spi_Hw_IsTransmissionDone((uint32)Spi_Unit[i].hwUnit);
            if ((Spi_Unit[i].status == SPI_BUSY) && xDone ) {
                Spi_Internal_Isr(&Spi_Unit[i]);
            }
        }
    }
}

/**
 *
 * @param uPtr
 */
void Spi_Internal_Isr(Spi_UnitType *uPtr) {
    sint32 rv;

    LDEBUG_PRINTF("Spi_Isr\n");
    assert(uPtr != NULL);
    assert(uPtr->jobPtr != NULL);
    assert(uPtr->seqPtr != NULL);

    Spi_Global.totalNbrOfTranfers++;

    Spi_Hw_DisableInterrupt((uint32)uPtr->hwUnit);

    rv = Spi_Hw_Rx(uPtr);

    if ( SPIE_JOB_PENDING == rv) {
        /* The job is not complete, so send more */
        LDEBUG_PRINTF("Pending jobs\nSpi_Isr END\n");

        rv = Spi_Hw_Tx(uPtr);

        if( rv == SPIE_BAD ) {
            /* Fail all */
            Spi_FailAll(uPtr);
        }
        return;
    }

    //Not supported yet.
#if (USE_DIO_CS == STD_ON)
    void (*cb)(int) = Spi_Global.configPtr->SpiExternalDevice[Spi_Global.configPtr->SpiJobConfig[*uPtr->currJobIndexPtr].DeviceAssignment].SpiCsCallback;
    if( cb != NULL ) {
        cb(0);
    }
#endif

    //Notification
    /* @req SWS_Spi_00057 */
    /* @req SWS_Spi_00119 */
    /* @req SWS_Spi_00118 */
    void (*notif)(void) = uPtr->jobPtr->SpiJobEndNotification;
    if (notif != NULL) {
        notif();
    }

    if ( SPIE_BAD == rv) {
        Spi_FailAll(uPtr);
    }
    else {

        /* Job is done so set job result */
        /* @req SWS_Spi_00292 */
        Spi_SetJobResult(uPtr->jobPtr->SpiJobId, SPI_JOB_OK);

        uPtr->jobIdxInSeq++;
        uPtr->jobPtr = uPtr->seqPtr->SpiJobAssignment[uPtr->jobIdxInSeq];

        /* Find the next job for this sequence if there is any */
        if (uPtr->jobPtr != NULL) {
            LDEBUG_PRINTF("****** more_jobs %d******\n", uPtr->jobIdxInSeq);
            Spi_JobWrite(uPtr);
        } else {
            Spi_SequenceType seq;
            LDEBUG_PRINTF("######SE####\n");
            // No more jobs, so set HwUnit and sequence IDLE/OK also.
            Spi_SetHWUnitStatus(uPtr, SPI_IDLE);
            Spi_SetSequenceResult(uPtr->seqPtr->SpiSequenceId, SPI_SEQ_OK);

            /* @req SWS_Spi_00120 */
            /* @req SWS_Spi_00281 */
            if (uPtr->seqPtr->SpiSeqEndNotification != NULL) {
                uPtr->seqPtr->SpiSeqEndNotification();
            }

            /* Check for queued sequences to this unit */
            if( uPtr->callType == SPI_ASYNC_CALL ) {
                /* According to lint this is a recursive call, and it is. However
                 * this is only done at the end of a sequence (ie Spi_MainFunction_Handling())
                 * and not during Spi_AsyncTransmit(..)
                 * */
                if( QueuePop(&uPtr->queueHead,&seq ) == 0 ) {	/*lint !e934 Taking address of near auto var ok */
                	Std_ReturnType tmpReturnValue = Spi_SeqWrite(seq, SPI_ASYNC_CALL );

                    if( tmpReturnValue == E_NOT_OK ) {
                        Spi_FailAll(uPtr);
                    }
                }
            }
        }
    }
}

uint16_t Spi_Internal_GetNbrChnInJob(const Spi_UnitType *uPtr) {
	assert((uPtr!=NULL) && (uPtr->jobPtr!=NULL));
    uint16_t retval = 0;
    const Spi_ChannelConfigType *chPtr;

    for (uint16_t n = 0; n < (SPI_MAX_CHANNEL + 1); n++) {
        chPtr = uPtr->jobPtr->SpiChannelAssignment[n];

        if (chPtr == NULL) {
            /* we are done */
            retval = n;
            break;
        }
    }

    return retval;
}

void Spi_GetVersionInfo(Std_VersionInfoType* versioninfo) {

    /* @req SWS_Spi_00371 */
    VALIDATE(( NULL != versioninfo ), SPI_GETVERSIONINFO_SERVICE_ID, SPI_E_PARAM_POINTER);

    versioninfo->vendorID = SPI_VENDOR_ID;
    versioninfo->moduleID = SPI_MODULE_ID;
    versioninfo->sw_major_version = SPI_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SPI_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SPI_SW_PATCH_VERSION;

}
//lint -restore
