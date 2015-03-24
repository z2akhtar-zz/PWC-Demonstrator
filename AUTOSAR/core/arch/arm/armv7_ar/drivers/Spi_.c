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

/* ----------------------------[includes]------------------------------------*/

#include <stdlib.h>
#include <assert.h>
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
#include "irq_zynq.h"
#endif

#if (SPI_MAX_JOB > 32)
#error Spi: The maximum number of 32 jobs is supported
#endif
/* ----------------------------[private define]------------------------------*/
#define USE_DIO_CS          STD_OFF  /* Not supported yet. */

#define MODULE_NAME 	"/driver/Spi"

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

Spi_GlobalType  Spi_Global = {0};
Spi_EbType      Spi_Eb[SPI_MAX_CHANNEL];
Spi_UnitType    Spi_Unit[SPI_CONTROLLER_CNT] = {{0}};
Spi_SeqUnitType Spi_SeqUnit[SPI_MAX_SEQUENCE];
Spi_JobUnitType Spi_JobUnit[SPI_MAX_JOB];
Spi_ChannelInfoType Spi_ChannelInfo[SPI_MAX_CHANNEL];
uint8 Spi_CtrlToUnit[4];

/* ----------------------------[private functions]---------------------------*/



static void Spi_SeqWrite(Spi_SequenceType seqIndex, Spi_CallTypeType sync);

/**
 * Get the buffer for a channel.
 *
 * @param ch
 * @param length
 * @return
 */
Spi_DataBufferType *Spi_Internal_GetRxChBuf(Spi_ChannelType ch, Spi_NumberOfDataType *length ) {
    Spi_DataBufferType *buf;
	if( (Spi_Global.configPtr->SpiChannelConfig[ch].SpiChannelType == SPI_EB)  &&
        (Spi_Global.extBufPtr[ch].active == 1))
	{
		*length = Spi_Global.extBufPtr[ch].length;
		buf = Spi_Global.extBufPtr[ch].dest;
	} else {
		/* No support */
	    SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
		*length = 0;
		buf = NULL;
	}
	return buf;
}

const Spi_DataBufferType *Spi_Internal_GetTxChBuf(Spi_ChannelType ch, Spi_NumberOfDataType *length ) {
	const Spi_DataBufferType *buf;
    if( (Spi_Global.configPtr->SpiChannelConfig[ch].SpiChannelType == SPI_EB)  &&
        (Spi_Global.extBufPtr[ch].active == 1))
    {
		*length = Spi_Global.extBufPtr[ch].length;
		buf = Spi_Global.extBufPtr[ch].src;
	} else {
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

uint32 Spi_GetJobCnt(void);
uint32 Spi_GetChannelCnt(void);
uint32 Spi_GetExternalDeviceCnt(void);

#if defined(__DMA_INT)
static void Spi_Isr_DMA( void )
{
	// Clear interrupt
	Dma_ClearInterrupt(5);
}
#endif

static void Spi_JobWrite(Spi_UnitType *uPtr);

/**
 * Get external Ptr to device from index
 *
 * @param deviceType The device index.
 * @return Ptr to the external device
 */

static inline const Spi_ExternalDeviceType *Spi_GetExternalDevicePtrFromIndex(
		Spi_ExternalDeviceTypeType deviceType) {
	return (&(Spi_Global.configPtr->SpiExternalDevice[(deviceType)]));
}

/**
 * Get configuration job ptr from job index
 * @param jobIndex the job
 * @return Ptr to the job configuration
 */
static const Spi_JobConfigType *Spi_GetJobPtrFromIndex(Spi_JobType jobIndex) {
	return &Spi_Global.configPtr->SpiJobConfig[jobIndex];
}

/**
 * Get sequence ptr from sequence index
 * @param seqIndex the sequence
 * @return Ptr to the sequence configuration
 */
static const Spi_SequenceConfigType *Spi_GetSeqPtrFromIndex(
		Spi_SequenceType SeqIndex) {
	return &Spi_Global.configPtr->SpiSequenceConfig[SeqIndex];
}


/**
 * Function to see if two sequences share jobs
 *
 * @param seq - Seqence 1
 * @param seq - Seqence 2
 * @return 0 - if the don't share any jobs
 *        !=0 - if they share jobs
 */

static boolean Spi_ShareJobs(Spi_SequenceType seq1, Spi_SequenceType seq2) {
	uint32 seqMask1 = 0;
	uint32 seqMask2 = 0;
	const Spi_JobType *jobPtr;
	const Spi_SequenceConfigType *seqConfig;

	// Search for jobs in sequence 1
	seqConfig = Spi_GetSeqPtrFromIndex(seq1);
	jobPtr = &seqConfig->JobAssignment[0];

	while (*jobPtr != JOB_NOT_VALID) {
	    if( *jobPtr <= 31 ) {
	        seqMask1 |= (1 << *jobPtr);
	    } else {
	        SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
	    }
		jobPtr++;
	}

	// Search for jobs in sequence 2
	seqConfig = Spi_GetSeqPtrFromIndex(seq2);
	jobPtr = &seqConfig->JobAssignment[0];

	while (*jobPtr != JOB_NOT_VALID) {
        if( *jobPtr <= 31 ) {
            seqMask2 |= (1 << *jobPtr);
        } else {
            SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
        }
		jobPtr++;
	}

	return (seqMask1 & seqMask2);
}


//-------------------------------------------------------------------

/**
 * Sets a result for a sequence
 *
 * @param Sequence The sequence to set the result for
 * @param result The result to set.
 */
static void Spi_SetSequenceResult(Spi_SequenceType Sequence,
		Spi_SeqResultType result) {
	Spi_SeqUnit[Sequence].seqResult = result;
}


//-------------------------------------------------------------------

/* !req SWS_Spi_00177 !req SWS_Spi_00304 !req SWS_Spi_00305 */
/* !req SWS_Spi_00306 !req SWS_Spi_00307 !req SWS_Spi_00018 */
/* !req SWS_Spi_00024 !req SWS_Spi_00023 !req SWS_Spi_00137 */

Std_ReturnType Spi_WriteIB(Spi_ChannelType Channel,
		const Spi_DataBufferType *DataBufferPtr)
{
    VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_WRITEIB_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );
    VALIDATE_W_RV( ( Channel<SPI_MAX_CHANNEL ), SPI_WRITEIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );
    VALIDATE_W_RV( ( DataBufferPtr != NULL ), SPI_WRITEIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );
    VALIDATE_W_RV( ( SPI_IB==Spi_Global.configPtr->SpiChannelConfig[Channel].SpiChannelType ),
            SPI_WRITEIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );

    /* According to SPI051 it seems that we only have to a "depth" of 1 */


    Std_ReturnType rv = E_NOT_OK;
    return rv;
}

//-------------------------------------------------------------------
/* @req SWS_Spi_00299 */
void Spi_Init(const Spi_ConfigType *ConfigPtr) {
	const Spi_JobConfigType *jobConfig2;
	Spi_UnitType *uPtr;
	uint32 confMask;
	uint8 ctrlNr;

	VALIDATE( ( NULL != ConfigPtr ), SPI_INIT_SERVICE_ID, SPI_E_PARAM_POINTER);
	/* @req SWS_Spi_00246 */
	VALIDATE( ( TRUE != Spi_Global.initRun), SPI_INIT_SERVICE_ID, SPI_E_ALREADY_INITIALIZED);

	/* @req SWS_Spi_00082 */
	memset(&Spi_Global,0,sizeof(Spi_Global));
	Spi_Global.configPtr = ConfigPtr;
	Spi_Global.extBufPtr = Spi_Eb;

	/* @req SWS_Spi_00151 */
	Spi_Global.asyncMode = SPI_POLLING_MODE;

	// Set all sequence results to OK
	for (Spi_SequenceType i = (Spi_SequenceType) 0; i < SPI_MAX_SEQUENCE; i++) {
		Spi_SetSequenceResult(i, SPI_SEQ_OK);
	}

	// Figure out what HW controllers that are used and set all job results to OK

	for (uint32_t j = 0; j < Spi_GetJobCnt(); j++) {
		jobConfig2 = &Spi_Global.configPtr->SpiJobConfig[j];
		Spi_Global.spiHwConfigured |= (1 << jobConfig2->SpiHwUnit);
		Spi_SetJobResult(j, SPI_JOB_OK);
	}

	confMask = Spi_Global.spiHwConfigured;

	for (uint32_t i=0; confMask; confMask &= ~(1 << ctrlNr),i++)
	{
		ctrlNr = ilog2(confMask);
		Spi_CtrlToUnit[ctrlNr] = i;
		uPtr = &Spi_Unit[i];

		uPtr->hwUnit = ctrlNr;
		/* @req SWS_Spi_00013 */
		Spi_Hw_InitController(uPtr);
		Spi_SetHWUnitStatus(uPtr, SPI_IDLE);
	}

	/* @req SWS_Spi_00015 */

	Spi_Global.initRun = TRUE;
}

//-------------------------------------------------------------------

Std_ReturnType Spi_DeInit(void) {
	uint32 confMask;
	uint8 confNr;
	Spi_UnitType *uPtr;

	/* @req SWS_Spi_00303 */


	/* @req SWS_Spi_00302 */
	/* @req SWS_Spi_00253 */
	/* @req SWS_Spi_00046 */
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_DEINIT_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );
	if (Spi_GetStatus() == SPI_BUSY) {
		return E_NOT_OK;
	}

	// Disable the HW modules
	/* @req SWS_Spi_00021 */
	confMask = Spi_Global.spiHwConfigured;

	// Disable the SPI hw
	/* @req SWS_Spi_00252 */
	for (int i=0; confMask; confMask &= ~(1 << confNr),i++) {
		confNr = ilog2(confMask);
		uPtr = &Spi_Unit[i];

		spi_DeInitController(uPtr);
		Spi_SetHWUnitStatus(uPtr, SPI_UNINIT);
	}

	/* @req SWS_Spi_00022 */
	Spi_Global.configPtr = NULL;
	Spi_Global.initRun = FALSE;

	/* @req SWS_Spi_00301 */
	return E_OK;
}

/**
 * Write a job to the SPI bus
 *
 * @param jobIndex The job to write to the SPI bus
 */
static void Spi_JobWrite(Spi_UnitType *uPtr) {
    sint32 rv;
    /* @req SWS_Spi_00286 */
	Spi_SetJobResult(*uPtr->currJobIndexPtr, SPI_JOB_PENDING);

	uPtr->chIndexPtr      = &Spi_GetJobPtrFromIndex(*uPtr->currJobIndexPtr)->ChannelAssignment[0];

	//DIO_CS not supported yet
	//void (*cb)(int) = Spi_Global.configPtr->SpiExternalDevice[Spi_Global.configPtr->SpiJobConfig[*uPtr->currJobIndexPtr].DeviceAssignment].SpiCsCallback;

#if (USE_DIO_CS == STD_ON)
    if( cb != NULL ) {
        cb(1);
    }
#endif
    /* @req SWS_Spi_00330 */
    rv = spi_Tx( uPtr );

    if (rv == SPIE_BAD) {
        //Notification
        /* @req SWS_Spi_00057 */
        /* @req SWS_Spi_00119 */
        /* @req SWS_Spi_00118 */
        void (*notif)(void) = Spi_Global.configPtr->SpiJobConfig[*uPtr->currJobIndexPtr].SpiJobEndNotification;
        if( notif!= NULL ) {
            notif();
        }

        /* Fail both job and sequence */
        Spi_SetHWUnitStatus(uPtr, SPI_IDLE);
        /* @req SWS_Spi_00293 */
        Spi_SetJobResult(*uPtr->currJobIndexPtr, SPI_JOB_FAILED);
        Spi_SetSequenceResult(uPtr->currSeqPtr->SpiSequenceId,
                SPI_SEQ_FAILED);
        /* @req SWS_Spi_00120 */
        /* @req SWS_Spi_00281 */
        if (uPtr->currSeqPtr->SpiSeqEndNotification != NULL) {
                        uPtr->currSeqPtr->SpiSeqEndNotification();
        }
    }
}


/**
 * Write a sequence to the SPI bus
 *
 * @param seqIndex The sequence
 * @param sync 1 - make the call sync. 0 - make the call async
 */
static void Spi_SeqWrite(Spi_SequenceType seqIndex, Spi_CallTypeType sync) {
	Spi_UnitType *		uPtr;
	Spi_JobResultType 	jobResult;
	const Spi_SequenceConfigType *seqConfig = Spi_GetSeqPtrFromIndex(seqIndex);

	/*  Set JobResult */

	/* !req SWS_Spi_00194 */
	jobResult = ( sync == SPI_ASYNC_CALL) ? SPI_JOB_QUEUED : SPI_JOB_PENDING;

	{
		const Spi_JobType *jobPtr = &seqConfig->JobAssignment[0];
		/* queue the job */
		/* @req SWS_Spi_00194 */
		/* @req SWS_Spi_00157 */
		while (*jobPtr != JOB_NOT_VALID) {
			Spi_SetJobResult(*jobPtr, jobResult);
			jobPtr++;
		}
	}

	uPtr = GET_SPI_UNIT_PTR(Spi_GetJobPtrFromIndex(seqConfig->JobAssignment[0])->SpiHwUnit);

	/* @req SWS_Spi_00020 */
	/* @req SWS_Spi_00134 */
	Spi_SetHWUnitStatus(uPtr, SPI_BUSY);
	/* @req SWS_Spi_00285 */
	Spi_SetSequenceResult(seqIndex, SPI_SEQ_PENDING);

	/* Fill in the required fields for job and sequence.. */
	uPtr->currJobIndexPtr = &seqConfig->JobAssignment[0];

	uPtr->callType   = sync;
	uPtr->currSeqPtr = seqConfig;

	Spi_JobWrite(uPtr);

	/* Busy wait */
	if (uPtr->callType == SPI_SYNC_CALL) {
		while (Spi_GetSequenceResult(seqIndex) == SPI_SEQ_PENDING) {
		    if (Spi_Hw_IsTransmissionDone(uPtr) == E_OK) {
		        Spi_Internal_Isr(uPtr);
		    }
		}
	}
}

//-------------------------------------------------------------------
static boolean Spi_AnyPendingJobs(Spi_SequenceType Sequence) {

	// Check that we don't share any jobs with another sequence that is SPI_SEQ_PENDING
	for (uint32 i = 0; i < SPI_MAX_SEQUENCE; i++) {
		if (i == Sequence) {
			continue;
		}

		if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
			// We have found a pending sequence... check that we don't share any jobs
			// with that sequence
			if (Spi_ShareJobs(Sequence, i)) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

//-------------------------------------------------------------------


/**
 * Blocking write
 *
 * @param Sequence
 * @return
 */
#if SPI_LEVEL_DELIVERED == 0 || SPI_LEVEL_DELIVERED == 2
Std_ReturnType Spi_SyncTransmit(Spi_SequenceType Sequence) {

    /* @req SWS_Spi_00329 */
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );
	/* @req SWS_Spi_00032 */
	VALIDATE_W_RV( ( SPI_MAX_SEQUENCE>Sequence ), SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_PARAM_SEQ, E_NOT_OK );


	if (Spi_GetSequenceResult(Sequence) == SPI_SEQ_PENDING) {
		/* @req SWS_Spi_00329 */
		return E_NOT_OK;
	}

    /* @req SWS_Spi_00135*/
#if (SPI_SUPPORT_CONCURRENT_SYNC_TRANSMIT == STD_OFF)
    for (int i = 0; i < SPI_MAX_SEQUENCE; i++) {
        if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
            SPI_DET_REPORT_ERROR(SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_SEQ_IN_PROCESS);
            return E_NOT_OK;
        }
    }
#endif

    if( SPI_SEQ_OK != Spi_GetSequenceResult(Sequence) ) {
        SPI_DET_REPORT_ERROR(SPI_SYNCTRANSMIT_SERVICE_ID, SPI_E_SEQUENCE_NOT_OK);
        return E_NOT_OK;
    }
	if (Spi_AnyPendingJobs(Sequence)) {
		return E_NOT_OK;
	}

	Spi_SeqWrite(Sequence, SPI_SYNC_CALL);

	/* @req SWS_Spi_00309 */
	return E_OK;
}
#endif
//-------------------------------------------------------------------
/* @req SWS_Spi_00133 */
#if SPI_LEVEL_DELIVERED == 1 || SPI_LEVEL_DELIVERED == 2
Std_ReturnType Spi_AsyncTransmit(Spi_SequenceType Sequence) {
    Spi_SeqResultType isSeqPending;
    boolean anyJobPending;

	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );
	VALIDATE_W_RV( ( SPI_MAX_SEQUENCE>Sequence ), SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_PARAM_SEQ, E_NOT_OK );

	isSeqPending = Spi_GetSequenceResult(Sequence);
	anyJobPending = Spi_AnyPendingJobs(Sequence);

	/* @req SWS_Spi_00081 */
	/* @req SWS_Spi_00086 */
	if ((isSeqPending == SPI_SEQ_PENDING) || anyJobPending) {
	    /* @req SWS_Spi_00266 */
	    SPI_DET_REPORT_ERROR(SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_SEQ_PENDING);
	    return E_NOT_OK;
	}

	/* @req SWS_Spi_00035*/
#if (SPI_SUPPORT_CONCURRENT_SYNC_TRANSMIT == STD_OFF)
    for (int i = 0; i < SPI_MAX_SEQUENCE; i++) {
        if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
            SPI_DET_REPORT_ERROR(SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_SEQ_IN_PROCESS);
            return E_NOT_OK;
        }
    }
#endif

    if( SPI_SEQ_OK != Spi_GetSequenceResult(Sequence) ) {
        SPI_DET_REPORT_ERROR(SPI_ASYNCTRANSMIT_SERVICE_ID, SPI_E_SEQUENCE_NOT_OK);
        return E_NOT_OK;
    }

	Spi_SeqWrite(Sequence, SPI_ASYNC_CALL);

	return E_OK;
}
#endif
//-------------------------------------------------------------------


Std_ReturnType Spi_ReadIB(	Spi_ChannelType Channel,
        Spi_DataBufferType* DataBufferPtr)
{
	(void)DataBufferPtr;

	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_READIB_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );
	VALIDATE_W_RV( ( Channel<SPI_MAX_CHANNEL ), SPI_READIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );
	VALIDATE_W_RV( ( SPI_IB<Spi_Global.configPtr->SpiChannelConfig[Channel].SpiChannelType ),
			SPI_READIB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );

	/* NOT SUPPORTED */

	Std_ReturnType rv = E_NOT_OK;
	return rv;
}

//-------------------------------------------------------------------
/* @req SWS_Spi_00317 */
Std_ReturnType Spi_SetupEB(	Spi_ChannelType Channel,
							const Spi_DataBufferType* SrcDataBufferPtr,
							Spi_DataBufferType* DesDataBufferPtr,
							Spi_NumberOfDataType Length)
{
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_SETUPEB_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );
	/* @req SWS_Spi_00031 */
	VALIDATE_W_RV( ( Channel<SPI_MAX_CHANNEL ), SPI_SETUPEB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );
#if ( SPI_CHANNEL_BUFFERS_ALLOWED == 1 )
	VALIDATE_W_RV( ( SPI_EB==Spi_Global.configPtr->SpiChannelConfig[Channel].SpiChannelType ),
			SPI_SETUPEB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );
#endif

	/* @req SWS_Spi_00060 */
	VALIDATE_W_RV( ( Length<=Spi_Global.configPtr->SpiChannelConfig[Channel].SpiEbMaxLength ),
			SPI_SETUPEB_SERVICE_ID, SPI_E_PARAM_CHANNEL, E_NOT_OK );

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
	} else {
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
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_GETSTATUS_SERVICE_ID, SPI_E_UNINIT, SPI_UNINIT );

	// Check all sequences if they have any job pending
	for (int i = 0; i < SPI_MAX_SEQUENCE; i++) {
		if (Spi_GetSequenceResult(i) == SPI_SEQ_PENDING) {
			return SPI_BUSY;
		}
	}

	return SPI_IDLE;
}

//-------------------------------------------------------------------

/* @req SWS_Spi_00182 */

Spi_JobResultType Spi_GetJobResult(Spi_JobType Job) {
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_GETJOBRESULT_SERVICE_ID, SPI_E_UNINIT, SPI_JOB_FAILED );
	VALIDATE_W_RV( !( SPI_MAX_JOB<=Job ), SPI_GETJOBRESULT_SERVICE_ID, SPI_E_PARAM_JOB, SPI_JOB_FAILED );

	/* @req SWS_Spi_00322 */
	/* @req SWS_Spi_00026 */
	return Spi_JobUnit[Job].jobResult;
}

//-------------------------------------------------------------------

/* @req SWS_Spi_00183 */
Spi_SeqResultType Spi_GetSequenceResult(Spi_SequenceType Sequence) {

    /* @req SWS_Spi_00324 */
    /* @req SWS_Spi_00039 */
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_GETSEQUENCERESULT_SERVICE_ID, SPI_E_UNINIT, SPI_SEQ_FAILED );
	VALIDATE_W_RV( ( SPI_MAX_SEQUENCE>Sequence ), SPI_GETSEQUENCERESULT_SERVICE_ID, SPI_E_PARAM_SEQ, SPI_SEQ_FAILED );

	return Spi_SeqUnit[Sequence].seqResult;

}

//-------------------------------------------------------------------
#if (SPI_HW_STATUS_API == STD_ON)
/* @req SWS_Spi_00186 */
/* @req SWS_Spi_00142 */
Spi_StatusType Spi_GetHWUnitStatus(Spi_HWUnitType HWUnit) {
	VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_GETHWUNITSTATUS_SERVICE_ID, SPI_E_UNINIT, SPI_UNINIT );
	/* @req SWS_Spi_00143 */
	/* @req SWS_Spi_00288 */
	VALIDATE_W_RV( !( SPI_CONTROLLER_CNT <= HWUnit ), SPI_GETHWUNITSTATUS_SERVICE_ID, SPI_E_PARAM_UNIT, SPI_UNINIT );

	/* @req SWS_Spi_00332 */
	/* @req SWS_Spi_00141 */
	return (GET_SPI_UNIT_PTR(HWUnit))->status;
}
#endif
//-------------------------------------------------------------------

#if (SPI_CANCEL_API == STD_ON )
void Spi_Cancel( Spi_SequenceType Sequence ) {
	VALIDATE( ( TRUE == Spi_Global.initRun ), SPI_CANCEL_SERVICE_ID, SPI_E_UNINIT );
	VALIDATE( ( SPI_MAX_SEQUENCE<Sequence ), SPI_CANCEL_SERVICE_ID, SPI_E_PARAM_SEQ );

	/* NOT SUPPORTED */
}
#endif

//-------------------------------------------------------------------


/* @req SWS_Spi_00154 */
#if ( SPI_LEVEL_DELIVERED == 2)
/* @req SWS_Spi_00188 */
Std_ReturnType Spi_SetAsyncMode(Spi_AsyncModeType Mode) {
    Std_ReturnType retval = E_OK;
    uint8_t hwUnitBusy = 0;
    Spi_UnitType * uPtr = NULL;
    Spi_CallTypeType currentTransmissionMode = SPI_SYNC_CALL;
    uint32 confMask;
    uint8 ctrlNr;

    /* @req SWS_Spi_00337 */
    VALIDATE_W_RV( ( TRUE == Spi_Global.initRun ), SPI_SETASYNCMODE_SERVICE_ID, SPI_E_UNINIT, E_NOT_OK );

    confMask = Spi_Global.spiHwConfigured;

    for (int i=0; confMask; confMask &= ~(1 << ctrlNr),i++) {
        ctrlNr = ilog2(confMask);
        Spi_CtrlToUnit[ctrlNr] = i;
        uPtr = &Spi_Unit[i];

        if (uPtr->status == SPI_BUSY) {
            hwUnitBusy = 1;
            currentTransmissionMode = uPtr->callType;
        }
    }

    if ((currentTransmissionMode == SPI_ASYNC_CALL) && hwUnitBusy) {
        /* @req SWS_Spi_00171 */
        /* @req SWS_Spi_00337 */
        retval = E_NOT_OK;
    } else {
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
	Spi_UnitType *uPtr = NULL;
	uint32 confMask;
    uint8 ctrlNr;

	if (Spi_Global.asyncMode == SPI_POLLING_MODE) {

        confMask = Spi_Global.spiHwConfigured;

        for (int i=0; confMask; confMask &= ~(1 << ctrlNr),i++) {
            ctrlNr = ilog2(confMask);
            Spi_CtrlToUnit[ctrlNr] = i;
            uPtr = &Spi_Unit[i];

            if ((uPtr->status == SPI_BUSY) && (Spi_Hw_IsTransmissionDone(uPtr) == E_OK)) {
                Spi_Internal_Isr(uPtr);
            }
        }
	}
}


/**
 *
 * @param uPtr
 */
void Spi_Internal_Isr( Spi_UnitType *uPtr) {
//	volatile struct DSPI_tag *spiHw = uPtr->hwPtr;
	sint32 rv;

	RAMLOG_STR("Spi_Isr\n");

	/* Halt DSPI unit until we are ready for next transfer. */
	Spi_Hw_Halt(uPtr);

	Spi_Global.totalNbrOfTranfers++;

	Spi_Hw_DisableInterrupt(uPtr);

	rv = Spi_Hw_Rx(uPtr);

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
    void (*notif)(void) = Spi_Global.configPtr->SpiJobConfig[*uPtr->currJobIndexPtr].SpiJobEndNotification;
    if( notif!= NULL ) {
    	notif();
    }

    if (rv == SPIE_BAD) {
		/* Fail both job and sequence */
		Spi_SetHWUnitStatus(uPtr, SPI_IDLE);
		/* @req SWS_Spi_00293 */
		Spi_SetJobResult(*uPtr->currJobIndexPtr, SPI_JOB_FAILED);
		Spi_SetSequenceResult(uPtr->currSeqPtr->SpiSequenceId,
				SPI_SEQ_FAILED);
		/* @req SWS_Spi_00120 */
		/* @req SWS_Spi_00281 */
        if (uPtr->currSeqPtr->SpiSeqEndNotification != NULL) {
		    uPtr->currSeqPtr->SpiSeqEndNotification();
        }
	} else {

		/* Job is done so set job result */
	    /* @req SWS_Spi_00292 */
	    Spi_SetJobResult(*uPtr->currJobIndexPtr, SPI_JOB_OK);


		uPtr->currJobIndexPtr++;
		// Find the next job for this sequence if there is any
		if( *uPtr->currJobIndexPtr != JOB_NOT_VALID ) {
			Spi_JobWrite(uPtr);
			RAMLOG_STR("more_jobs\n");
		} else {
			// No more jobs, so set HwUnit and sequence IDLE/OK also.
			Spi_SetHWUnitStatus(uPtr, SPI_IDLE);
			Spi_SetSequenceResult(uPtr->currSeqPtr->SpiSequenceId,
					SPI_SEQ_OK);

			/* @req SWS_Spi_00120 */
            /* @req SWS_Spi_00281 */
			if (uPtr->currSeqPtr->SpiSeqEndNotification != NULL) {
				uPtr->currSeqPtr->SpiSeqEndNotification();
			}
		}
	}

	RAMLOG_STR("Spi_Isr END\n");
}

uint16_t Spi_Internal_GetNbrChnInJob(Spi_UnitType *uPtr)
{
    uint16_t retval = 0;
    uint16_t n;
    Spi_ChannelType spiChannel;
    uint16_t currJob = *uPtr->currJobIndexPtr;

    for(n = 0; n < SPI_MAX_CHANNEL+1; n++) {
        spiChannel = SpiConfigData.SpiJobConfig[currJob].ChannelAssignment[n];

        if (spiChannel == CH_NOT_VALID) {
          /* we are done */
          retval = n;
          break;
        }
    }

    return retval;
}

void Spi_GetVersionInfo(Std_VersionInfoType* versioninfo) {

    /* @req SWS_Spi_00371 */
    VALIDATE( ( NULL != versioninfo ), SPI_GETVERSIONINFO_SERVICE_ID, SPI_E_PARAM_POINTER);

    versioninfo->vendorID = SPI_VENDOR_ID;
    versioninfo->moduleID = SPI_MODULE_ID;
    versioninfo->sw_major_version = SPI_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SPI_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SPI_SW_PATCH_VERSION;

}
