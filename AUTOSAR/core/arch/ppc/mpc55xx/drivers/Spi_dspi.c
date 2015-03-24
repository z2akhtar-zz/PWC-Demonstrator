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
 * TODO:
 * - The queue, check OLD hw....
 *
 */

//#define SPI_IMPLEMENTATION SPI_DMA

/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */
/** @tagSettings DEFAULT_ARCHITECTURE=PPC */

//*PC-Lint Exception */
/*lint -save -e934 -e613 Taking address off near auto variable is ok. Our asserts check for null pointers.*/

#include "Spi_Internal.h"
#include "mpc55xx.h"
#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
#include "isr.h"
#include "irq_types.h"
#endif
#include "Mcu.h"
#include "Os.h"
#include "cirq_buffer.h"

#if (SPI_IMPLEMENTATION == SPI_DMA)
#include "Dma.h"
#endif

#include <string.h>
#include <stdlib.h>


/* ----------------------------[private define]------------------------------*/


#define DSPI_CTRL_A	0
#define DSPI_CTRL_B	1
#define DSPI_CTRL_C	2
#define DSPI_CTRL_D	3
#define DSPI_CTRL_E	4
#define DSPI_CTRL_F	5

#if defined(CFG_MPC560X) || defined(CFG_MPC5645S) || defined(CFG_MPC5643L) || defined(CFG_SPC56XL70)
#define DSPI_A_ISR_EOQF DSPI_0_ISR_EOQF
#define DSPI_B_ISR_EOQF DSPI_1_ISR_EOQF
#if defined(CFG_MPC560XB) || defined(CFG_MPC5604P) || defined(CFG_MPC5643L) || defined(CFG_SPC56XL70)
#define DSPI_C_ISR_EOQF DSPI_2_ISR_EOQF
#endif
#if defined(CFG_MPC5606B)
#define DSPI_D_ISR_EOQF DSPI_3_ISR_EOQF
#define DSPI_E_ISR_EOQF DSPI_4_ISR_EOQF
#define DSPI_F_ISR_EOQF DSPI_5_ISR_EOQF
#endif
#endif


#if defined(CFG_MPC560XB) || defined(CFG_MPC5604P)
#define CTAR_CNT    6
#elif defined(CFG_MPC5744P) || defined(CFG_MPC5643L) || defined(CFG_SPC56XL70) || defined(CFG_MPC5777M)
#define CTAR_CNT    (sizeof(((struct DSPI_tag*)0)->CTAR) / sizeof(((struct DSPI_tag*)0)->CTAR[0])) /* 4*/
#else
#define CTAR_CNT    8
#endif

#define USE_DIO_CS          STD_ON

// E2 read = cmd + addr + data = 1 + 2 + 64 ) = 67 ~ 72
#define SPI_INTERNAL_MTU    72

/* The depth of the HW FIFO */
#if defined(CFG_MPC5744P) || defined(CFG_MPC5643L) || defined(CFG_SPC56XL70) || defined(CFG_MPC5777M) || defined(CFG_MPC5645S)
#define FIFO_DEPTH    (sizeof(((struct DSPI_tag*)0)->TXFR) / sizeof(((struct DSPI_tag*)0)->TXFR[0])) /* 5 */
#else
#define FIFO_DEPTH			4uL
#endif
/* Define for debug purposes, checks that SPI/DMA is ok */
//#define STEP_VALIDATION		1

#define MODULE_NAME 	"/driver/Spi"

//#define USE_LDEBUG_PRINTF
#include "debug.h"


/* For debug */
#define STAY(_exp)   if( !(_exp) ) { DisableAllInterrupts(); while(1) {}}

/* ----------------------------[private macro]-------------------------------*/

#ifndef MIN
#define MIN(_x,_y) (((_x) < (_y)) ? (_x) : (_y))
#endif
#ifndef MAX
#define MAX(_x,_y) (((_x) > (_y)) ? (_x) : (_y))
#endif

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(_x)  (sizeof(_x)/sizeof((_x)[0]))
#endif

typedef volatile struct DSPI_tag DSPI_Type;

#if defined(CFG_MPC5744P)
static DSPI_Type * const SPI_HW[] = {&SPI_0, &SPI_1, &SPI_2, &SPI_3};
//#define GET_SPI_HW_PTR(_unit) SPI_HW[_unit]
#elif defined(CFG_MPC5777M)
static DSPI_Type * const SPI_HW[] = {&DSPI_0, &DSPI_1, &DSPI_2, &DSPI_3, &DSPI_4, &DSPI_5, &DSPI_6, [12] = &DSPI_12};

static inline DSPI_Type * GetSpiHwPtr(uint32 unit) {
	assert(unit<7 || unit==12);
    return SPI_HW[_unit];
}
#else
static inline DSPI_Type * GetSpiHwPtr(uint32 unit) {
    return ((DSPI_Type *)(0xFFF90000uL + (0x4000uL*(unit))));      /*lint !e9033 !e923 */
}
#endif


static inline void EnableEoqInterrupt( DSPI_Type *hwPtr) {
	assert(hwPtr!=NULL);
    hwPtr->RSER.B.EOQF_RE = 1;
}

static inline void DisableEoqInterrupt( DSPI_Type *hwPtr) {
	assert(hwPtr!=NULL);
    hwPtr->RSER.B.EOQF_RE = 0;
}

/* ----------------------------[private typedef]-----------------------------*/


#if (SPI_IMPLEMENTATION == SPI_DMA )
typedef struct Spi_DmaConfig {
	 Dma_ChannelType RxDmaChannel;
	 Dma_ChannelType TxDmaChannel;
} Spi_DmaConfigType;

#endif


/*lint --e{46} --e{9018} --e{754} Lint is confused */
typedef union {
	vuint32_t R;
	struct {
		vuint32_t CONT :1;
		vuint32_t CTAS :3;
		vuint32_t EOQ :1;
		vuint32_t CTCNT :1;
		vuint32_t :4;
		vuint32_t PCS5 :1;
		vuint32_t PCS4 :1;
		vuint32_t PCS3 :1;
		vuint32_t PCS2 :1;
		vuint32_t PCS1 :1;
		vuint32_t PCS0 :1;
		vuint32_t TXDATA :16;
	} B;
} Spi_CommandType;

typedef union {
     vuint32_t R;
     struct {
         vuint32_t DBR:1;
         vuint32_t FMSZ:4;
         vuint32_t CPOL:1;
         vuint32_t CPHA:1;
         vuint32_t LSBFE:1;
         vuint32_t PCSSCK:2;
         vuint32_t PASC:2;
         vuint32_t PDT:2;
         vuint32_t PBR:2;
         vuint32_t CSSCK:4;
         vuint32_t ASC:4;
         vuint32_t DT:4;
         vuint32_t BR:4;
     } B;
 } CTAR_type;

//typedef SPICommandType Spi_CommandType;


#if 0
typedef struct Spi_SeqQueue {
	Spi_SequenceType seq;
	Spi_CallTypeType callType;
} Spi_QueueType;
#endif

struct privData {
    uint32 rxBytesCopied;
    uint32 txBytesCopied;
    uint32 fifoSent;
    DSPI_Type *hwPtr;
#if (SPI_IMPLEMENTATION==SPI_DMA)
    Dma_ChannelType dmaTxChannel;       // Tx DMA channel information
    Dma_TcdType     dmaTxTCD;
    Dma_ChannelType dmaRxChannel;      // Rx DMA channel information
    Dma_TcdType     dmaRxTCD;
    Spi_CommandType txQueue[SPI_INTERNAL_MTU];      // Pointed to by SADDR of DMA
    uint32          rxQueue[SPI_INTERNAL_MTU];      // Pointed to by DADDR of DMA
#endif
} ;

uint32 Spi_CTARIndex[SPI_MAX_CHANNEL];


/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/
/* ----------------------------[private functions]---------------------------*/
#if (SPI_USE_HW_UNIT_0 == STD_ON )
ISR(Spi_Isr_A);
#endif
#if (SPI_USE_HW_UNIT_1 == STD_ON )
ISR(Spi_Isr_B);
#endif
#if (SPI_USE_HW_UNIT_2 == STD_ON )
ISR(Spi_Isr_C);
#endif
#if (SPI_USE_HW_UNIT_3 == STD_ON )
ISR(Spi_Isr_D);
#endif
#if (SPI_USE_HW_UNIT_4 == STD_ON )
ISR(Spi_Isr_E);
#endif
#if (SPI_USE_HW_UNIT_5 == STD_ON )
ISR(Spi_Isr_F);
#endif

#if (SPI_USE_HW_UNIT_0 == STD_ON )
ISR(Spi_Isr_A) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(DSPI_CTRL_A));
}
#endif
#if (SPI_USE_HW_UNIT_1 == STD_ON )
ISR(Spi_Isr_B) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(DSPI_CTRL_B));
}
#endif
#if (SPI_USE_HW_UNIT_2 == STD_ON )
ISR(Spi_Isr_C) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(DSPI_CTRL_C));
}
#endif
#if (SPI_USE_HW_UNIT_3 == STD_ON )
ISR(Spi_Isr_D) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(DSPI_CTRL_D));
}
#endif
#if (SPI_USE_HW_UNIT_4 == STD_ON )
ISR(Spi_Isr_E) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(DSPI_CTRL_E));
}
#endif
#if (SPI_USE_HW_UNIT_5 == STD_ON )
ISR(Spi_Isr_F) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(DSPI_CTRL_F));
}
#endif
/* ----------------------------[public functions]----------------------------*/

#if defined(__DMA_INT)
static void Spi_Isr_DMA( void )
{
	// Clear interrupt
	Dma_ClearInterrupt(5);
}
#endif


//-------------------------------------------------------------------

#if (SPI_IMPLEMENTATION==SPI_DMA)

static sint32 checkJobTotalLength(Spi_UnitType *uPtr, uint16_t *numberOfChannels)
{
	assert(uPtr!=NULL && uPtr->jobPtr!=NULL );
	assert(numberOfChannels!=NULL);
    Spi_NumberOfDataType totalLength = 0;
    Spi_NumberOfDataType chnLength = 0;
    sint32 retval = SPIE_OK;
    uint16_t n;
    uint16_t nbrOfChan = Spi_Internal_GetNbrChnInJob(uPtr);
    const Spi_ChannelConfigType *chPtr;

    for(n = 0; n < nbrOfChan; n++) {
        chPtr = uPtr->jobPtr->SpiChannelAssignment[n];
        assert(chPtr!=NULL);

        Spi_Internal_GetTxChBuf( chPtr->SpiChannelId , &chnLength);     /*lint !e534 Ignoring return value. */
        totalLength += chnLength;
    }

#if (SPI_IMPLEMENTATION == SPI_FIFO)
    if (totalLength > FIFO_DEPTH) {
        retval = SPIE_BAD;
    }
#else
    if (totalLength > SPI_INTERNAL_MTU) {
        retval = SPIE_BAD;
    }
#endif


    *numberOfChannels = n;
    return retval;
}

#endif

//-------------------------------------------------------------------



#if (SPI_IMPLEMENTATION==SPI_DMA)

/**
 * Function to handle things after a transmit on the SPI is finished.
 * It copies data from it't local buffers to the buffers pointer to
 * by the external buffers
 *
 * @param spiUnit Ptr to a SPI unit
 */

sint32 Spi_Hw_Rx( Spi_UnitType *uPtr ) {
	assert(uPtr!=NULL && uPtr->pData!=NULL);
    Spi_DataBufferType *    rxBuf;
    Spi_NumberOfDataType    rxLength;
    Spi_NumberOfDataType    n;
    Spi_NumberOfDataType    i;
    uint16_t                numberOfChannels;
    uint32                  total = 0;
    const Spi_ChannelConfigType *chPtr;
    DSPI_Type                   *hwPtr = GetSpiHwPtr(uPtr->hwUnit);

    while( !Spi_Hw_IsTransmissionDone(uPtr->hwUnit) ) {};

    /* We have some sort of EOQ indication, so halt and clear */
    hwPtr->MCR.B.HALT = 1;
    hwPtr->SR.B.EOQF = 1;


    /* Stop the channels */
    Dma_StopChannel(uPtr->pData->dmaTxChannel);
    Dma_StopChannel(uPtr->pData->dmaRxChannel);

    numberOfChannels = Spi_Internal_GetNbrChnInJob(uPtr);

    for (i = 0; i < numberOfChannels; i++) { 			/*lint !e638 Spi_NumberOfDataType=uint16=numberOfChannels  */
        chPtr = uPtr->jobPtr->SpiChannelAssignment[i];
        assert(chPtr!=NULL);

        rxBuf = Spi_Internal_GetRxChBuf( chPtr->SpiChannelId, &rxLength);
        LDEBUG_PRINTF("RX-LEN:%p/%d (ch=%d)\n",rxBuf,rxLength,chPtr->SpiChannelId);

        /* Copy data depending on size */
       if (rxBuf != NULL) {
           if( chPtr->SpiDataWidth > 8 ) {
               /* 9 to 16-bit transfers */
               for (n = 0; n < rxLength; n+=2) {
                   rxBuf[n ] = ((uPtr->pData->rxQueue[total+n]>>8uL) & 0xffu);
                   rxBuf[n + 1] = (uPtr->pData->rxQueue[total+n] & 0xffu);
               }
               total += rxLength/2;
           } else {
               for (n = 0; n < rxLength; n++) {
                   rxBuf[n] = (uPtr->pData->rxQueue[total+n] & 0xffu);
                   LDEBUG_PRINTF("RX-DATA:%x %d %d\n",rxBuf[n],total,n);
               }
               total += rxLength;
           }

       } else {
           if( chPtr->SpiDataWidth > 8 ) {
               total += rxLength/2;
           } else {
               total += rxLength;
           }
       }
    }

    return SPIE_OK;
}


#elif (SPI_IMPLEMENTATION==SPI_FIFO)

sint32 Spi_Hw_Rx(Spi_UnitType *uPtr) {
	assert((uPtr!=NULL) && (uPtr->jobPtr!=NULL) && (uPtr->pData!=NULL));
    uint32                  bytesInFifo;
    uint32                  bytesInChar;
    uint32                  copyCnt;
    sint32                  rv = SPIE_JOB_PENDING;
    Spi_DataBufferType *    rxBuf;
    Spi_NumberOfDataType    rxLength;
    const Spi_ChannelConfigType   *chPtr;
    uint16_t popVal;
    uint32 i;

    DSPI_Type * hwPtr = GetSpiHwPtr(uPtr->hwUnit);

    /* We have some sort of EOQ indication, so halt and clear */
    hwPtr->MCR.B.HALT = 1;
    hwPtr->SR.B.EOQF = 1;

    bytesInFifo = hwPtr->SR.B.RXCTR; /*lint !e632 Assignment from 4 bit register*/

#if defined(USE_LDEBUG_PRINTF)
    STAY(bytesInFifo == uPtr->pData->fifoSent); /*lint !e634  Equality ok. */
#endif

    LDEBUG_PRINTF(" RX-Bytes-in-FIFO=%d\n",bytesInFifo);

    do {
        chPtr = uPtr->jobPtr->SpiChannelAssignment[uPtr->rxChIdxInJob];
        assert(chPtr!=NULL);

        /* We need to keep track the current channel and many
         * bytes we have copied to the current channel.
         * Note that this is the same job
         */
        rxBuf = Spi_Internal_GetRxChBuf(chPtr->SpiChannelId, &rxLength);
        /*
         * We may now copy rxLength - uPtr->rxBytesCopied
         */
        copyCnt = MIN( ((uint32)rxLength - uPtr->pData->rxBytesCopied) >> ((chPtr->SpiDataWidth-1uL)/8uL), bytesInFifo );
        bytesInChar = (chPtr->SpiDataWidth > 8u) ? 2uL : 1uL;

        /* Pop the FIFO */
        if (rxBuf != NULL) {
            if( chPtr->SpiDataWidth > 8 ) {

                /* 9 to 16-bit transfers */
                for (i = 0; i < copyCnt; i++) {
                    popVal = hwPtr->POPR.B.RXDATA; /*lint !e632 Assignment from 16 bit register*/
                    rxBuf[uPtr->pData->rxBytesCopied ] = (Spi_DataBufferType) (popVal >> 8u);
                    rxBuf[uPtr->pData->rxBytesCopied + 1] = (Spi_DataBufferType) popVal;
                    uPtr->pData->rxBytesCopied += 2;
                }
            } else {
                for (i = 0; i < copyCnt; i++) {
                    rxBuf[uPtr->pData->rxBytesCopied] = (Spi_DataBufferType)hwPtr->POPR.B.RXDATA;
                    uPtr->pData->rxBytesCopied++;
                }
            }
        } else {
            /* We must still pop the data, although we are not going to use it */
            for (i = 0; i < copyCnt; i++) {
                popVal = hwPtr->POPR.B.RXDATA; /*lint !e632 Assignment from 16 bit register*/
                uPtr->pData->rxBytesCopied += bytesInChar;
            }
        }

        /* We have copied data to one channel from FIFO */
        bytesInFifo -= copyCnt;

#if defined(USE_LDEBUG_PRINTF)
        STAY(uPtr->pData->rxBytesCopied <= rxLength );
#endif

        if ((rxLength - uPtr->pData->rxBytesCopied) == 0) {
            /* advance to the next channel */
            if( rxBuf != NULL ) {
                LDEBUG_PRINTF("RX-DATA:%x cnt=%d\n",rxBuf[0],rxLength);
            }
            uPtr->pData->rxBytesCopied = 0;
            uPtr->rxChIdxInJob++;
            if ( uPtr->jobPtr->SpiChannelAssignment[uPtr->rxChIdxInJob] == NULL ) { /*lint !e9032 Comparison to NULL is ok */
                uPtr->rxChIdxInJob = 0;
                rv = SPIE_OK;
                break;
            }

        }
    } while ( bytesInFifo != 0);

    return rv;
}

#endif

#if (SPI_IMPLEMENTATION==SPI_FIFO)
/**
 *
 * @param spiUnit
 * @param jobConfig
 * @return
 */
sint32 Spi_Hw_Tx( Spi_UnitType *uPtr )
{
	assert((uPtr!=NULL) && (uPtr->jobPtr!=NULL) && (uPtr->pData!=NULL) && (uPtr->jobPtr->SpiDeviceAssignment!=NULL));
    Spi_CommandType                 cmd;
    Spi_NumberOfDataType            bufLen;
    uint32				            copyCnt;
    uint32				            fifoCnt;
    const Spi_ChannelConfigType *   chPtr;
    const Spi_DataBufferType *      buf;
    Spi_CommandType	tmpFifo[FIFO_DEPTH];

    Spi_CommandType *tmpPtr = &tmpFifo[0];
    sint32          rv    = SPIE_JOB_PENDING;
    uint32          i     = 0;
    boolean         done  = FALSE;

    DSPI_Type * hwPtr = GetSpiHwPtr(uPtr->hwUnit);
    fifoCnt = 0;

	/* This the ACTIVE polarity. Freescale have inactive polarity */
	if (uPtr->jobPtr->SpiDeviceAssignment->SpiCsPolarity == STD_HIGH) {
		hwPtr->MCR.R &= ~(1uL << (16uL + uPtr->jobPtr->SpiDeviceAssignment->SpiCsIdentifier));
	} else {
		hwPtr->MCR.R |= (1uL << (16uL + uPtr->jobPtr->SpiDeviceAssignment->SpiCsIdentifier));
	}

	cmd.R = 0;
	/* Channels should keep CS active ( A job must assert CS continuously) */
	cmd.B.CONT = 1;
	/* Set the Chip Select (PCS) */
	cmd.R |= (1uL << (16uL + uPtr->jobPtr->SpiDeviceAssignment->SpiCsIdentifier));
    /* Loop over the channels */
    do {
        /* Iterate over the channels for this job */
		chPtr = uPtr->jobPtr->SpiChannelAssignment[uPtr->txChIdxInJob];
		assert(chPtr!=NULL);

        cmd.B.CTAS = Spi_CTARIndex[chPtr->SpiChannelId];
        bufLen = 0;
		buf = Spi_Internal_GetTxChBuf( chPtr->SpiChannelId, &bufLen);

		/* Minimum of how much data to copy and the limit of the FIFO */
		copyCnt = MIN( (bufLen - uPtr->pData->txBytesCopied) >> ((chPtr->SpiDataWidth-1u)/8u), FIFO_DEPTH - fifoCnt );

		LDEBUG_PRINTF("Tx-Ext-buf-Len=%d Ch=%d CTAS=%d CTAS-VAL=%x\n",bufLen,chPtr->SpiChannelId, cmd.B.CTAS,hwPtr->CTAR[cmd.B.CTAS].R);

		/* Fill tmp buffer */
		if( buf != NULL ) {
			if( chPtr->SpiDataWidth > 8 ) {
				/* 9 to 16-bit transfers */
				for (i = 0; i < copyCnt; i++) {
					tmpPtr[i].R = cmd.R;
                	uint8 tmpMisraBuf1 = buf[uPtr->pData->txBytesCopied]; /* Please MISRA */
                	uint8 tmpMisraBuf2 = buf[uPtr->pData->txBytesCopied + 1u];
					tmpPtr[i].B.TXDATA = 	((uint32)tmpMisraBuf1 << 8uL)
									   	  + ((uint32)tmpMisraBuf2 & 0xffuL);
					uPtr->pData->txBytesCopied+=2;
				}
			} else {
				/* <= 8-bit transfers */
				for (i = 0; i < copyCnt; i++) {
				    tmpPtr[i].R = cmd.R;
				    Spi_DataBufferType tmpMisraVar = buf[uPtr->pData->txBytesCopied]; /* Please MISRA */
				    tmpPtr[i].B.TXDATA = tmpMisraVar;
				    uPtr->pData->txBytesCopied++;
				}
			}
		} else {
			/* default data */
			for (i = 0; i < copyCnt; i++) {
			    tmpPtr[i].R = cmd.R;
			    tmpPtr[i].B.TXDATA = chPtr->SpiDefaultData;
			}
			uPtr->pData->txBytesCopied += copyCnt;
		}

		fifoCnt += copyCnt;
		tmpPtr = &tmpPtr[copyCnt]; /* Misra compliant ugly version of tmpPtr += copyCnt; */

		/* Space left in FIFO? */
		if( FIFO_DEPTH == fifoCnt ) {
			done = true;
		}

		/* Channel End? */
		if( 0 == (bufLen - uPtr->pData->txBytesCopied)) {
	           uPtr->pData->txBytesCopied = 0;
	           uPtr->txChIdxInJob++;
	           if ( uPtr->jobPtr->SpiChannelAssignment[uPtr->txChIdxInJob] == NULL ) { /*lint !e9032 Comparison to NULL is ok */
	               uPtr->txChIdxInJob = 0;
	               tmpFifo[fifoCnt-1].B.CONT = 0;  /* CS -> inactive */
	               done = true;
	               rv = SPIE_OK;
	               LDEBUG_PRINTF("TX-DONE\n");
	           } else {
	               LDEBUG_PRINTF("TX-PENDING\n");
	           }
		}
    } while (!done);

    tmpFifo[fifoCnt-1].B.EOQ = 1;	/* Last entry is EOQ */

    /* Push to FIFO */
    for(uint32 j=0;j<fifoCnt;j++) {
        LDEBUG_PRINTF("TX-DATA:%x (%d)\n",tmpFifo[j].R,j);
    	hwPtr->PUSHR.R = tmpFifo[j].R;
    }

    uPtr->pData->fifoSent = fifoCnt;

#if defined(USE_LDEBUG_PRINTF)
    STAY( fifoCnt == hwPtr->SR.B.TXCTR );
#endif

    if ( (Spi_Global.asyncMode == SPI_INTERRUPT_MODE) && (uPtr->callType == SPI_ASYNC_CALL)) {
        EnableEoqInterrupt(hwPtr);
    } else {
        DisableEoqInterrupt(hwPtr);
    }

    /* STOPPED to RUNNING */
    hwPtr->SR.B.EOQF = 1; /* Clear EOQF flag by writing 1 */
    hwPtr->MCR.B.HALT = 0;

    return rv;
}
#endif /* (SPI_IMPLEMENTATION==SPI_FIFO) */


/**
 * Function to setup CTAR's from configuration
 * @param spiHw - Pointer to HW SPI device
 * @param extDev - Pointer to external device configuration
 * @param ctar_unit - The ctar unit number to setup
 * @param width - The width in bits of the data to send with the CTAR
 */
static uint32 CtarCreateValue(  const Spi_ExternalDeviceType *extPtr,
                                   const Spi_ChannelConfigType *chPtr )
{
	assert(extPtr!=NULL);
	assert(chPtr!=NULL);
	uint32 clock;
	uint32 pre_br;
	uint32 i;
	uint32 j;
	uint32 tmp;
	Mcu_Arc_PeriperalClock_t perClock = (Mcu_Arc_PeriperalClock_t)0;
	CTAR_type val;
	uint32 width = chPtr->SpiDataWidth;
	/* Clock tables */
	const uint32 clk_table_asc[] = { 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048,
	        4096, 8192, 16384, 32768, 65536 };
	const uint32 clk_table_cssck[] = { 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048,
	        4096, 8192, 16384, 32768, 65536 };
	const uint32 clk_table_br[] = { 2, 4, 6, 8, 16, 32, 64, 128, 256, 512, 1024, 2048,
	        4096, 8192, 16384, 32768 };
	const uint8 clk_table_pasc[] = { 1u, 3u, 5u, 7u };
	const uint8 clk_table_pcssck[] = { 1u, 3u, 5u, 7u };
	const uint8 clk_table_pbr[] = { 2u, 3u, 5u, 7u };


	/* BAUDRATE CALCULATION
	 * -----------------------------
	 * Baudrate = Fsys/ PBR * ( 1+ DBR) / BR
	 * PBR range: 2 to 7
	 * DBR range: 0 to 1
	 * BR : 2 to 32768
	 *
	 * To make this easy set DBR = 0 and PBR=2
	 * --> BR=Fsys/(Baudrate.* 2 )
	 *
	 */

	switch(extPtr->SpiHwUnit ) {
#if (SPI_USE_HW_UNIT_0 == STD_ON )
	case 0:
		perClock = PERIPHERAL_CLOCK_DSPI_A;
		break;
#endif
#if (SPI_USE_HW_UNIT_1 == STD_ON )
	case 1:
		perClock = PERIPHERAL_CLOCK_DSPI_B;
		break;
#endif
#if (SPI_USE_HW_UNIT_2 == STD_ON )
	case 2:
		perClock = PERIPHERAL_CLOCK_DSPI_C;
		break;
#endif
#if (SPI_USE_HW_UNIT_3 == STD_ON )
	case 3:
		perClock = PERIPHERAL_CLOCK_DSPI_D;
		break;
#endif
#if (SPI_USE_HW_UNIT_4 == STD_ON )
	case 4:
		perClock = PERIPHERAL_CLOCK_DSPI_E;
		break;
#endif
#if (SPI_USE_HW_UNIT_5 == STD_ON )
	case 5:
		perClock = PERIPHERAL_CLOCK_DSPI_F;
		break;
#endif

	default:
		assert(0==0);
		break;
	}
	clock = Mcu_Arc_GetPeripheralClock(perClock);

	DEBUG(DEBUG_MEDIUM,"%s: Peripheral clock at %d Mhz\n",MODULE_NAME,clock);

	DEBUG(DEBUG_MEDIUM,"%s: Want to run at %d Mhz\n",MODULE_NAME,extPtr->SpiBaudrate);

	val.B.DBR = 0;
	val.B.PBR = 0; // 2
	assert(extPtr->SpiBaudrate!=0); /* Prevent division by zero */
	pre_br = clock / (extPtr->SpiBaudrate * clk_table_pbr[val.B.PBR]);      /*lint !e9032 !e632 !e633 Everything is uint32 except clk_table_pbr (uint8) */

	// find closest lesser
	for (i = 0; i < (uint32)(sizeof(clk_table_br) / sizeof(clk_table_br[0])); i++) {
		if (clk_table_br[i] >= pre_br) {
			break;
		}
	}

	// Set it
	val.B.BR = i;

	DEBUG(DEBUG_LOW,"%s: CLK %d Mhz\n",MODULE_NAME,
			clock / clk_table_pbr[val.B.PBR] *
			( 1 + val.B.DBR)/clk_table_br[val.B.BR]);

	/* For other timings autosar only specifies SpiTimeClk2Cs == "After SCK delay"
	 * in Freescale language. The dumb thing is that this should be a relative time
	 * to the clock. Not fixed.
	 * Autosar specifies 0.0--100.0 ms(float)
	 * Our intepretation is 0--1000000 ns (uint32)
	 *
	 * AFTER SCK DELAY:
	 * -----------------------------
	 * Tasc = 1/Fsys * PASC  * ASC [s]
	 *
	 * Assume the Tasc get's here in ns( typical range is ~10ns )
	 */

	// Calc the PASC * ASC value...
	tmp = extPtr->SpiTimeClk2Cs * (clock / 1000000uL);

	// Nothing fancy here...
	{
		uint32 best_i = 0;
		uint32 best_j = 0;
		sint32 b_value = INT_MAX; /*lint !e9027 !e9053 GHS defines this with shifts. */
		sint32 tt;

		// Find the best match of Prescaler and Scaler value
		for (i = 0; i < (uint32)ARRAY_SIZE(clk_table_pasc); i++) {
			for (j = 0; j < (uint32)ARRAY_SIZE(clk_table_asc); j++) {
				tt = (sint32)abs( (sint32)(clk_table_pasc[i] * clk_table_asc[j] * 1000uL) - (sint32)tmp);  /*lint !e9032 !e9033*/
				if (tt < b_value) {
					best_i = i;
					best_j = j;
					b_value = tt;
				}
			}
		}

		/* After SCK delay. */
		val.B.PASC = best_i;
		val.B.ASC = best_j;
	}

	DEBUG(DEBUG_MEDIUM,"%s: Timing: Tasc %d ns\n",MODULE_NAME,
			clk_table_pasc[val.B.PASC] *
			clk_table_asc[val.B.ASC] * 1000/ (clock/1000000) );

	/* The PCS to SCK delay is the delay between the assertion of PCS and
	 * the first edge the SCK.
	 *
	 * PCS TO SCK DELAY:
	 * -----------------------------
	 * Tcsc = 1/Fsys * PCSSCK * CSSCK [s]
	 */

	// Calc the PCSSCK * CSSCK value...
	tmp = extPtr->SpiTimeCs2Clk * (clock / 1000000uL);

	// Nothing fancy here...
	{
		uint32 best_i = 0;
		uint32 best_j = 0;
		sint32 b_value = INT_MAX; /*lint !e9027 !e9053 GHS defines this with shifts. */
		sint32 tt;

		// Find the best match of Prescaler and Scaler value
		for (i = 0; i < (uint32)ARRAY_SIZE(clk_table_pcssck); i++) {
			for (j = 0; j < (uint32)ARRAY_SIZE(clk_table_cssck); j++) {
				tt = (sint32)abs( (sint32)(clk_table_pcssck[i] * clk_table_cssck[j] * 1000uL) - (sint32)tmp);    /*lint !e9032 !e9033*/
				if (tt < b_value) {
					best_i = i;
					best_j = j;
					b_value = tt;
				}
			}
		}

		/* PCS to SCK delay */
		val.B.PCSSCK = best_i;
		val.B.CSSCK = best_j;
	}

	DEBUG(DEBUG_MEDIUM,"%s: Timing: Tcsc %d ns\n",MODULE_NAME,
			clk_table_pcssck[val.B.PCSSCK] *
			clk_table_cssck[val.B.CSSCK]*1000/(clock/1000000));

	/* Time that PCS is high between transfers */
	val.B.PDT = 2;
	val.B.DT = 2;

	DEBUG(DEBUG_MEDIUM,"%s: Timing: Tdt  %d ns\n",MODULE_NAME,
			clk_table_pasc[val.B.PDT] *
			clk_table_asc[val.B.DT]*1000/(clock/1000000));

	/* Data is transferred MSB first */

	val.B.LSBFE = (chPtr->SpiTransferStart == SPI_TRANSFER_START_MSB ) ? 0 : 1;

	/* Set mode */
	val.B.FMSZ = width - 1;
	val.B.CPHA = (extPtr->SpiDataShiftEdge == SPI_EDGE_LEADING) ? 0 : 1;
	val.B.CPOL = (extPtr->SpiShiftClockIdleLevel == STD_LOW) ? 0 : 1;

	return val.R;
}


/* Setup CTARS, configuration */
static void CtarSetup(void) {
    const Spi_ChannelConfigType *chPtr;
    const Spi_JobConfigType *jobPtr;
    uint32 ctarIdx;
    boolean match = false;
    uint32 tmp;
    DSPI_Type *spiHw;
    static uint32 Spi_CtarMap[6][CTAR_CNT] = {{0}};
    static uint32 Spi_CtarCnt[6] = {0};


    /*
     * Mapping of CTAR to Autosar
     *
     * CTAR                   Autosar
     * ---------------------------------
     * Width                  Channel
     * Baud rate              External device
     * Clock phase            External device
     * Clock polarity         External device
     * MSB/LSB                Channel
     */

    /* One Job have several channels, width is in the channel -> One CTAR per channel.
     * However, we often have more channels that we have CTARs -> Share CTARS
     */

    /* Loop over all jobs and calc the CTAR values */
    for (Spi_JobType j = 0; j < SPI_MAX_JOB; j++) {
        jobPtr = Config_GetJob(j);


        /* Loop over channels */
        for (uint32 chIdx = 0; (chPtr = jobPtr->SpiChannelAssignment[chIdx])!= NULL; chIdx++) {
            tmp = CtarCreateValue(jobPtr->SpiDeviceAssignment, chPtr);

            match = false;

            /* Find ctarValue */
            for(ctarIdx=0;ctarIdx<Spi_CtarCnt[jobPtr->SpiHwUnit];ctarIdx++) {
                if( Spi_CtarMap[jobPtr->SpiHwUnit][ctarIdx]== tmp ) {
                    // LDEBUG_PRINTF("Re-use of index=%d ch=%d\n",ctarIdx, chPtr->SpiChannelId);
                    Spi_CTARIndex[chPtr->SpiChannelId] = ctarIdx;       /* Index for this SpiHwUnit */
                    match = true;
                    break;
                }
            }

            if(!match) {
                /* Assign CTAR value  */
                spiHw = GetSpiHwPtr(jobPtr->SpiHwUnit);
                spiHw->CTAR[ctarIdx].R = tmp;
                /* Create Ctar Channel -> CTAR map */
                Spi_CTARIndex[chPtr->SpiChannelId] = ctarIdx;       /* Index for this SpiHwUnit */
                /* Mark CTAR as taken */
                Spi_CtarMap[jobPtr->SpiHwUnit][ctarIdx] = tmp;
                Spi_CtarCnt[jobPtr->SpiHwUnit] += 1;
            }
        }
    }

}

//-------------------------------------------------------------------

void Spi_Hw_InitController(uint32 hwUnit ) {

    DSPI_Type *spiHw = GetSpiHwPtr(hwUnit);


	/* Module configuration register. */
	/* Master mode. */
	spiHw->MCR.B.MSTR = 1;
	/* No freeze. Run SPI when debugger is stopped. */
	spiHw->MCR.B.FRZ = 0;

    spiHw->MCR.B.MDIS = 0;

	/* Enable FIFO's. */
#if (SPI_IMPLEMENTATION == SPI_DMA)
	spiHw->MCR.B.DIS_RXF = 1;
	spiHw->MCR.B.DIS_TXF = 1;
#elif (SPI_IMPLEMENTATION == SPI_FIFO)
    spiHw->MCR.B.DIS_RXF = 0;
    spiHw->MCR.B.DIS_TXF = 0;
#endif


	/* Set all active low. */
	spiHw->MCR.B.PCSIS0 = 1;
	spiHw->MCR.B.PCSIS1 = 1;
	spiHw->MCR.B.PCSIS2 = 1;
	spiHw->MCR.B.PCSIS3 = 1;
	spiHw->MCR.B.PCSIS4 = 1;
	spiHw->MCR.B.PCSIS5 = 1;

#if (SPI_IMPLEMENTATION == SPI_DMA)
	/* DMA TX FIFO fill. */
	spiHw->RSER.B.TFFF_RE = 1;
	spiHw->RSER.B.TFFF_DIRS = 1;

	/* DMA RX FIFO drain. */
	spiHw->RSER.B.RFDF_RE = 1;
	spiHw->RSER.B.RFDF_DIRS = 1;
#endif

	/* Force to stopped state. */
	spiHw->MCR.B.HALT = 1;
	spiHw->SR.B.EOQF = 1;

	/* Enable clocks. */
	spiHw->MCR.B.MDIS = 0;


	// Install EOFQ int..
    switch (hwUnit) {
    #if (SPI_USE_HW_UNIT_0 == STD_ON )
        case 0:
            ISR_INSTALL_ISR2("SPI_A", Spi_Isr_A, DSPI_A_ISR_EOQF, 15, 0); /*lint !e641 Converting enum to int*/
            break;
    #endif
    #if (SPI_USE_HW_UNIT_1 == STD_ON )
            case 1:
            ISR_INSTALL_ISR2("SPI_B",Spi_Isr_B, DSPI_B_ISR_EOQF, 15, 0); /*lint !e641 Converting enum to int*/
            break;
    #endif
    #if (SPI_USE_HW_UNIT_2 == STD_ON )
            case 2:
            ISR_INSTALL_ISR2("SPI_C",Spi_Isr_C, DSPI_C_ISR_EOQF, 15, 0); /*lint !e641 Converting enum to int*/
            break;
    #endif
    #if (SPI_USE_HW_UNIT_3 == STD_ON )
            case 3:
            ISR_INSTALL_ISR2("SPI_D",Spi_Isr_D, DSPI_D_ISR_EOQF, 15, 0); /*lint !e641 Converting enum to int*/
            break;
    #endif
    #if (SPI_USE_HW_UNIT_4 == STD_ON )
            case 4:
            ISR_INSTALL_ISR2("SPI_E",Spi_Isr_E, DSPI_E_ISR_EOQF, 15, 0); /*lint !e641 Converting enum to int*/
            break;
    #endif
    #if (SPI_USE_HW_UNIT_5 == STD_ON )
            case 5:
            ISR_INSTALL_ISR2("SPI_F",Spi_Isr_F, DSPI_F_ISR_EOQF, 15, 0); /*lint !e641 Converting enum to int*/
            break;
    #endif
            default:
                assert(0==0);
                break;
    }
}

//-------------------------------------------------------------------

#if (SPI_IMPLEMENTATION==SPI_DMA)
static void Spi_DmaSetup( const Spi_UnitType *uPtr ) {
	assert(uPtr!=NULL && uPtr->pData!=NULL);

    /*lint --e{940} Lint can't figure out designated initialiazers properly */
    const Dma_TcdType Spi_DmaTx = {
            .SADDR = 0, .SMOD = 0, .SSIZE = (vuint16_t)DMA_TRANSFER_SIZE_32BITS, .DMOD = 0,
            .DSIZE = (vuint16_t)DMA_TRANSFER_SIZE_32BITS, .SOFF = 4, .NBYTESu.R = 4, .SLAST = 0,
            .DADDR = 0, .CITERE_LINK = 0, .CITER = 0, .DOFF = 0, .DLAST_SGA = 0,
            .BITERE_LINK = 0, .BITER = 0, .BWC = 0, .MAJORLINKCH = 0, .DONE = 0,
            .ACTIVE = 0, .MAJORE_LINK = 0, .E_SG = 0, .D_REQ = 0, .INT_HALF = 0,
            .INT_MAJ = 0, .START = 0 };

    /*lint --e{940} Lint can't figure out designated initialiazers properly */
    const Dma_TcdType Spi_DmaRx = { .SADDR = 0, .SMOD = 0,
            .SSIZE = (vuint16_t)DMA_TRANSFER_SIZE_32BITS, .DMOD = 0,
            .DSIZE = (vuint16_t)DMA_TRANSFER_SIZE_32BITS, .SOFF = 0, .NBYTESu.R = 4, .SLAST = 0,
            .DADDR = 0, .CITERE_LINK = 0, .CITER = 1, .DOFF = 4, .DLAST_SGA = 0,
            .BITERE_LINK = 0, .BITER = 1, .BWC = 0, .MAJORLINKCH = 0, .DONE = 0,
            .ACTIVE = 0, .MAJORE_LINK = 0, .E_SG = 0, .D_REQ = 0, .INT_HALF = 0,
    #if defined(__DMA_INT)
            .INT_MAJ = 1,
    #else
            .INT_MAJ = 0,
    #endif
            .START = 0 };

	Dma_TcdType *tcd;
	const DSPI_Type  *spiHw = GetSpiHwPtr(uPtr->hwUnit);
	boolean     configValid;

	tcd = &uPtr->pData->dmaTxTCD;
	*tcd = Spi_DmaTx;
	tcd->SADDR = (uint32) uPtr->pData->txQueue; /*lint !e923 Hw related */
	tcd->DADDR = (uint32) &(spiHw->PUSHR.R);    /*lint !e923 Hw related */

	Dma_StopChannel(uPtr->pData->dmaTxChannel);
	configValid = Dma_CheckConfig();
	assert(configValid == true);

	// CITER and BITER set when we send
	tcd = &uPtr->pData->dmaRxTCD;
	*tcd = Spi_DmaRx;
	tcd->SADDR = (uint32) &(spiHw->POPR.R);	/*lint !e923 Ok, should be address */
	tcd->DADDR = (uint32) uPtr->pData->rxQueue; /*lint !e923 Ok, should be address */

	Dma_StopChannel(uPtr->pData->dmaRxChannel);
	Dma_CheckConfig(); /*lint !e534 Function always returns TRUE so... */

}
#endif

//-------------------------------------------------------------------

void Spi_Hw_Init(const Spi_ConfigType *ConfigPtr) {
	assert(ConfigPtr!=NULL);
	static struct privData    Spi_PrivateData[SPI_CONTROLLER_CNT];
    /* Setup HW Pointers */
    Spi_UnitType *uPtr;
    uint32 confMask = SPI_CHANNELS_CONFIGURED;      /*lint !e835 */
    uint32 ctrlNr;

    for (uint32_t i=0; i < SPI_CONTROLLER_CNT; i++)
    {
        ctrlNr = (uint32)ffs(confMask)-1uL; /*lint !e713 Signed/unsigned argument to ffs() doesn't really matter since it looks at bit level */
        confMask &= ~(1uL << ctrlNr); /* clear mask */
        uPtr = &Spi_Unit[i];
        uPtr->hwUnit = ctrlNr; /*lint !e734 Loss of precision is ok since ctrlNr is <=32 */
        uPtr->pData = &Spi_PrivateData[i];
        Spi_Hw_InitController(ctrlNr);
#if (SPI_IMPLEMENTATION  == SPI_DMA)
        /* When using DMA it assumes predefined names */
        static const Spi_DmaConfigType  Spi_DmaConfig[SPI_CONTROLLER_CNT] = {
#if (SPI_USE_HW_UNIT_0 == STD_ON )
		{
			.RxDmaChannel = DMA_DSPI_A_RESULT_CHANNEL,
			.TxDmaChannel = DMA_DSPI_A_COMMAND_CHANNEL,
		},
#endif
#if (SPI_USE_HW_UNIT_1 == STD_ON )
		{
			.RxDmaChannel = DMA_DSPI_B_RESULT_CHANNEL,
			.TxDmaChannel = DMA_DSPI_B_COMMAND_CHANNEL,
		},
#endif
#if (SPI_USE_HW_UNIT_2 == STD_ON )
		{
			.RxDmaChannel = DMA_DSPI_C_RESULT_CHANNEL,
			.TxDmaChannel = DMA_DSPI_C_COMMAND_CHANNEL,
		},
#endif
#if (SPI_USE_HW_UNIT_3 == STD_ON )
		{
			.RxDmaChannel = DMA_DSPI_D_RESULT_CHANNEL,
			.TxDmaChannel = DMA_DSPI_D_COMMAND_CHANNEL,
		},
#endif
#if (SPI_USE_HW_UNIT_4 == STD_ON )
		{
			.RxDmaChannel = DMA_DSPI_E_RESULT_CHANNEL,
			.TxDmaChannel = DMA_DSPI_E_COMMAND_CHANNEL,
		},
#endif
#if (SPI_USE_HW_UNIT_5 == STD_ON )
		{
			.RxDmaChannel = DMA_DSPI_F_RESULT_CHANNEL,
			.TxDmaChannel = DMA_DSPI_F_COMMAND_CHANNEL,
		}
#endif
        };
        /* Make sure that this channel shall be used. */
        uPtr->pData->dmaTxChannel = Spi_DmaConfig[i].TxDmaChannel;
        uPtr->pData->dmaRxChannel = Spi_DmaConfig[i].RxDmaChannel;

        Spi_DmaSetup(uPtr);

#endif
    }

    CtarSetup();
}/*lint !e715 ConfigPtr not used for now */

//-------------------------------------------------------------------


void  Spi_Hw_DeInitController( uint32 hwUnit) {

}  /*lint !e715 Not implemented */

#if (SPI_IMPLEMENTATION==SPI_DMA)


/**
 * Writes data to the SPI device. This is always a job.
 *
 * @param uPtr
 */
/* @req SWS_Spi_00311 */
sint32 Spi_Hw_Tx( Spi_UnitType *uPtr ) {
	assert(uPtr!=NULL);
    Spi_CommandType                 cmd;
    Spi_NumberOfDataType        txBufSize;
    const Spi_DataBufferType    *txBuf;
    const Spi_ChannelConfigType *chPtr;
    uint16_t numberOfChannels = 0;
    uint32 totalLength = 0;
    uint16_t n;
    uint32 data;
    sint32 jobLengthCheck;
    uint32 defaultValue;
    DSPI_Type *spiHw = GetSpiHwPtr(uPtr->hwUnit);

    jobLengthCheck = checkJobTotalLength(uPtr, &numberOfChannels);

    if (jobLengthCheck == SPIE_BAD) {
        return SPIE_BAD;
    }


    cmd.R = 0;
    /* Channels should keep CS active ( A job must assert CS continuously) */
    cmd.B.CONT = 1;
    /* Set the Chip Select (PCS) */
    cmd.R |= (1uL << (16uL + uPtr->jobPtr->SpiDeviceAssignment->SpiCsIdentifier));

    //Buffer and transmit all channels of the job.
    for (n = 0; n < numberOfChannels; n++) {
        chPtr = uPtr->jobPtr->SpiChannelAssignment[n];
        cmd.B.CTAS = Spi_CTARIndex[chPtr->SpiChannelId];

        /* Get buffer the channel uses (either EB or IB) */
        txBuf = Spi_Internal_GetTxChBuf(chPtr->SpiChannelId, &txBufSize);

        /* Copy from TX channel buffer to HW */
        /* @req SWS_Spi_00028 */

        defaultValue = Spi_Internal_GetTxDefValue(chPtr->SpiChannelId);

        LDEBUG_PRINTF("TX-LEN=%d Ch=%d CTAS=%d CTAS-VAL=%x\n",txBufSize,chPtr->SpiChannelId, cmd.B.CTAS,spiHw->CTAR[cmd.B.CTAS].R);

        if (chPtr->SpiDataWidth > 8) {
            if( txBuf == NULL ) {
                for (uint16 i=0; i < txBufSize ; i+=2 ) {
                    uPtr->pData->txQueue[totalLength].R = cmd.R;
                    uPtr->pData->txQueue[totalLength].B.TXDATA = (uint16)defaultValue;
                    totalLength+=1;
                }
            } else {
                for (uint16 i=0; i < txBufSize ; i+=2 ) {
                	uint8 tmpMisraTxBuf = txBuf[i];		/* Please Misra */
                	uint8 tmpMisraTxBuf2 = txBuf[i+1];
                	cmd.B.TXDATA = (((uint32)tmpMisraTxBuf)<<8uL) + (((uint32)tmpMisraTxBuf2) & 0xffuL);
                    uPtr->pData->txQueue[totalLength].R = cmd.R;
                    LDEBUG_PRINTF("TX-DATA:%x %d(%d)\n",uPtr->pData->txQueue[totalLength].R, totalLength, i);
                    totalLength+=1;
                }
            }
        } else {
            for (uint16 i=0; i < txBufSize ; i++ ) {
            	uint8 tmpMisra = txBuf[i]; /* Please Misra */
                data = (txBuf != NULL) ? (uint32)tmpMisra : defaultValue;


                uPtr->pData->txQueue[totalLength].R = cmd.R;
                uPtr->pData->txQueue[totalLength].B.TXDATA = (uint16)data;

                LDEBUG_PRINTF("TX-DATA:%x %d(%d)\n",uPtr->pData->txQueue[totalLength].R, totalLength, i);
                totalLength++;

            }
        }


    }

    uPtr->pData->txQueue[totalLength-1].B.EOQ = 1;   /* Last entry is EOQ */
    uPtr->pData->txQueue[totalLength-1].B.CONT = 0;

    if ((uPtr->callType == SPI_ASYNC_CALL) && (Spi_Global.asyncMode == SPI_INTERRUPT_MODE)) {
        Spi_Hw_EnableInterrupt(uPtr->hwUnit);
    }


    spiHw->TCR.B.SPI_TCNT = 0;

    // Set the length of the data to send
    uPtr->pData->dmaTxTCD.CITER = totalLength;
    uPtr->pData->dmaTxTCD.BITER = totalLength;


    Dma_ConfigureChannel((Dma_TcdType *) &uPtr->pData->dmaTxTCD, uPtr->pData->dmaTxChannel); /*lint !e929 Pointer casting ok */
    Dma_ConfigureChannel((Dma_TcdType *) &uPtr->pData->dmaRxTCD, uPtr->pData->dmaRxChannel); /*lint !e929 Pointer casting ok */
    /* Flush TX/Rx FIFO.  Ref. man. 23.5.1 step 8 */
    spiHw->MCR.B.CLR_RXF = 1;
    spiHw->MCR.B.CLR_TXF = 1;


    Dma_StartChannel(uPtr->pData->dmaRxChannel);
    Dma_StartChannel(uPtr->pData->dmaTxChannel);

    /* This will trig a new transfer. Ref. man. 23.5.1 step 11 */
    spiHw->SR.B.EOQF = 1;
    spiHw->MCR.B.HALT = 0;



    return SPIE_OK;
}


#endif



boolean Spi_Hw_IsTransmissionDone( uint32 hwUnit ) {
    const DSPI_Type * hwPtr = GetSpiHwPtr(hwUnit);
    return (boolean)(hwPtr->SR.B.TXRXS == 0);
}

void Spi_Hw_Halt(uint32 hwUnit) {
    DSPI_Type * hwPtr = GetSpiHwPtr(hwUnit);

    /* Halt DSPI unit until we are ready for next transfer. */
    hwPtr->MCR.B.HALT = 1;
    hwPtr->SR.B.EOQF = 1;
}

void Spi_Hw_DisableInterrupt( uint32 hwUnit ) {
    DSPI_Type * hwPtr = GetSpiHwPtr(hwUnit);
    DisableEoqInterrupt(hwPtr);
}

void Spi_Hw_EnableInterrupt( uint32 hwUnit ) {
    DSPI_Type * hwPtr = GetSpiHwPtr(hwUnit);
    EnableEoqInterrupt(hwPtr);
}

//lint -restore
