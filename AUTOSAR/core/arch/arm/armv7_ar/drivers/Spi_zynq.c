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


/* ----------------------------[includes]------------------------------------*/

#include "Spi_Internal.h"
#include "zynq.h"
#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
#include "isr.h"
#include "irq_zynq.h"
#endif
#include "Mcu.h"
#include "Os.h"

/* ----------------------------[private define]------------------------------*/
#define SPI_ISR_PRIORITY                4
#define FIFO_DEPTH		                128
#define MASK_INTERRUPT_DISABLE          0x0000007F
#define TX_FIFO_UNDERFLOW	            (1<<6)
#define TX_FIFO_NO_FULL                 (1<<2)

#define CONFIG_REG_RST_VALUE            0x00020000
#define INTRPT_EN_REG_RST_VALUE         0x00000000
#define INTRPT_DIS_REG_RST_VALUE        0x00000000
#define EN_REG_RST_VALUE                0x00000000
#define DELAY_REG_RST_VALUE             0x00000000
#define TX_DATA_REG_RST_VALUE           0x00000000
#define SLAVE_IDLE_COUNT_REG_RST_VALUE  0x000000FF
#define TX_THRES_REG_RST_VALUE          0x00000001
#define RX_THRES_REG_RST_VALUE          0x00000001

#define RX_FIFO_NOT_EMPTY_MASK          0x00000010
#define TX_FIFO_NOT_EMPTY_MASK          0x00000004

#define MASTER_MODE                     0x00000001
#define MAN_START_EN                    0x00008000

#define MASK_CLR_D_INIT                 0xFFFFFF00

#define BAUD_DIV_INVALID        0
#define BAUD_DIV_4              4
#define BAUD_DIV_8              8
#define BAUD_DIV_16             16
#define BAUD_DIV_32             32
#define BAUD_DIV_64             64
#define BAUD_DIV_128            128
#define BAUD_DIV_256            256
#define NBR_VALID_BAUD_DIV      7

#define SHIFT_CHIP_SELECT(_x)   ((_x)<<10)
#define CS_SIZE_MASK            0x0F
#define SHIFT_DIVIDER(_x)       ((_x)<<3)
#define CLK_PH_MASK             0x00000004
#define CLK_POL_MASK            0x00000002

#define MAX_CLK2CS_DELAY_TICKS 255

/* ----------------------------[private macro]-------------------------------*/

#define  GET_SPI_HW_PTR(_unit)    ((struct SPI_reg *)(ZYNQ_SPI_BASE + 0x1000*(_unit)))

/* ----------------------------[private typedef]-----------------------------*/

/*
struct Spi_Hw {
	struct SPI_reg *R;
};
*/

/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/
/* ----------------------------[private functions]---------------------------*/


#if (SPI_USE_HW_UNIT_0 == STD_ON )
ISR(Spi_Isr_0) {
    Spi_Internal_Isr(GET_SPI_UNIT_PTR(0));
}
#endif
#if (SPI_USE_HW_UNIT_1 == STD_ON )
ISR(Spi_Isr_1) {
	Spi_Internal_Isr(GET_SPI_UNIT_PTR(1));
}
#endif

static void emptyRxFifo(struct SPI_reg *hwPtr);
static void emptyTxFifo(struct SPI_reg *hwPtr);
static void emptyRxTxFifo(struct SPI_reg *hwPtr);
static sint32 checkJobTotalLength(Spi_UnitType *uPtr, uint16_t *numberOfChannels);

static const uint16_t baudrateDiv[NBR_VALID_BAUD_DIV] = {BAUD_DIV_4, BAUD_DIV_8, BAUD_DIV_16, BAUD_DIV_32, BAUD_DIV_64, BAUD_DIV_128, BAUD_DIV_256};

/* ----------------------------[public functions]----------------------------*/




/**
 *
 * @param hwPtr
 * @param ePtr
 */
static void setupExternalDevice( struct SPI_reg *hwPtr, const struct Spi_ExternalDevice *ePtr ) {
    uint8_t n;
    uint32_t chipSelect = SHIFT_CHIP_SELECT(ePtr->SpiCsIdentifier & CS_SIZE_MASK);
    uint32_t polarity = (ePtr->SpiShiftClockIdleLevel == STD_LOW) ? 0 : CLK_PH_MASK;
    uint32_t shiftLevel = (ePtr->SpiDataShiftEdge == SPI_EDGE_LEADING) ? 0 : CLK_POL_MASK;
	uint32_t wantedDivider = (MCU_ARC_CLOCK_SPI_0_FREQUENCY + (ePtr->SpiBaudrate/2))/ ePtr->SpiBaudrate;
	uint32_t wantedClk2CsDelay;
	uint32_t actualDivider = BAUD_DIV_INVALID;

	for (n = 0; n < NBR_VALID_BAUD_DIV; n++) {
	    if (wantedDivider >= baudrateDiv[n]) {
	        actualDivider = SHIFT_DIVIDER(n + 1);
	    }
	}

	if( BAUD_DIV_INVALID == actualDivider ) {
	    /* Did not find a suiting divider */
	    SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
	    actualDivider = SHIFT_DIVIDER(NBR_VALID_BAUD_DIV);
	}

	/* Not supported by HW
	 * - SpiDataShiftEdge
	 */

	hwPtr->Config_reg0 = MAN_START_EN | chipSelect | actualDivider | polarity | shiftLevel | MASTER_MODE;

	if (ePtr->SpiTimeClk2Cs != 0) {
	    wantedClk2CsDelay = (ePtr->SpiTimeClk2Cs * (MCU_ARC_CLOCK_SPI_0_FREQUENCY / 1000000) ) /1000;
	    //Delay register can only delay a max of 255 MCU_ARC_CLOCK_SPI_0_FREQUENCY cycles.
	    if( wantedClk2CsDelay > MAX_CLK2CS_DELAY_TICKS ) {
	        SPI_DET_REPORT_ERROR(SPI_GENERAL_SERVICE_ID, SPI_E_CONFIG_INVALID);
	        wantedClk2CsDelay = MAX_CLK2CS_DELAY_TICKS;
	    }
	    hwPtr->Delay_reg0 = (hwPtr->Delay_reg0 & MASK_CLR_D_INIT) | wantedClk2CsDelay;
	}

}


/**
 * Writes data to the SPI device. This is always a job.
 *
 * @param uPtr
 */
/* @req SWS_Spi_00311 */
sint32 Spi_Hw_Tx( Spi_UnitType *uPtr ) {

	Spi_NumberOfDataType 		txBufSize;
	const Spi_DataBufferType  	*txBuf;
	uint16_t numberOfChannels = 0;
	uint16_t n;
	uint32_t data;
	sint32 jobLengthCheck;
	struct SPI_reg *hwPtr = GET_SPI_HW_PTR(uPtr->hwUnit);
	uint32 defaultValue;
	const Spi_ChannelConfigType *chPtr;

	jobLengthCheck = checkJobTotalLength(uPtr, &numberOfChannels);
	if (jobLengthCheck == SPIE_BAD) {
	    return SPIE_BAD;
	}

    /* Setup HW to match configuration */
    setupExternalDevice(hwPtr,uPtr->jobPtr->SpiDeviceAssignment);

    hwPtr->En_reg0 = 1;

    //Buffer and transmit all channels of the job.
    for (n = 0; n < numberOfChannels; n++) {

        chPtr = uPtr->jobPtr->SpiChannelAssignment[n];

        /* Get buffer the channel uses (either EB or IB) */
        txBuf = Spi_Internal_GetTxChBuf(chPtr->SpiChannelId, &txBufSize);

        /* Copy from TX channel buffer to HW */
        /* @req SWS_Spi_00028 */
        defaultValue = Spi_Internal_GetTxDefValue(chPtr->SpiChannelId);

        for (int i=0; i < txBufSize ; i++ ) {
            data = (txBuf != NULL) ? (uint32_t)txBuf[i] : defaultValue;

            hwPtr->Tx_data_reg0 = data;
        }

    }

    if ((uPtr->callType == SPI_ASYNC_CALL) && (Spi_Global.asyncMode == SPI_INTERRUPT_MODE)) {
        Spi_Hw_EnableInterrupt(uPtr->hwUnit);
    }

    /* Start transmission */
    hwPtr->Config_reg0 |= (1<<16);

	return SPIE_OK;
}


sint32 Spi_Hw_Rx( Spi_UnitType *uPtr ) {
    Spi_DataBufferType *rxBuf;
    uint32_t rxData;
    Spi_NumberOfDataType rxLength;
    Spi_NumberOfDataType n, i;
    struct SPI_reg *hwPtr = GET_SPI_HW_PTR(uPtr->hwUnit);
    uint16_t numberOfChannels = 0;
    const Spi_ChannelConfigType *chPtr;

    numberOfChannels = Spi_Internal_GetNbrChnInJob(uPtr);

    for (i = 0; i < numberOfChannels; i++) {
        chPtr = uPtr->jobPtr->SpiChannelAssignment[i];
        rxBuf = Spi_Internal_GetRxChBuf(chPtr->SpiChannelId, &rxLength);

        for (n = 0; n < rxLength; n++) {
            rxData = hwPtr->Rx_data_reg0;

            // If its a NULL pointer the data shall not be stored.
            /* @req SWS_Spi_00036 */
            if (rxBuf != NULL) {
                //Rx register is 32 bit but the data is only valid in bits [7:0].
                rxBuf[n] = (Spi_DataBufferType) rxData;
            }
        }
    }

    return SPIE_OK;
}


void Spi_Hw_Init( const Spi_ConfigType *ConfigPtr ) {
    uint32 confMask = SPI_CHANNELS_CONFIGURED;      /*lint !e835 */
    uint32 ctrlNr;

    (void)ConfigPtr;

    for (uint32_t i=0; i < SPI_CONTROLLER_CNT; i++) {
        ctrlNr = (uint32)ffs(confMask)-1uL; /*lint !e713 Signed/unsigned argument to ffs() doesn't really matter since it looks at bit level */
        Spi_Hw_InitController(ctrlNr);
    }
}

//-------------------------------------------------------------------

void Spi_Hw_InitController( uint32 hwUnit ) {
    struct SPI_reg *spiHw = GET_SPI_HW_PTR(hwUnit);

	/* Enable Controller */
    spiHw->Config_reg0 = (uint32_t)(MAN_START_EN | MASTER_MODE);

	//Empty all FIFOs
	emptyRxTxFifo(spiHw);

	spiHw->TX_thres_reg0 = TX_THRES_REG_RST_VALUE;
	spiHw->RX_thres_reg0 = RX_THRES_REG_RST_VALUE;

    //Disable all interrupts
	/* @req SWS_Spi_00151 */
	Spi_Hw_DisableInterrupt(hwUnit);

#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
	switch (hwUnit) {
#if (SPI_USE_HW_UNIT_0 == STD_ON )
	case 0:
	ISR_INSTALL_ISR2("SPI_0",Spi_Isr_0, IRQ_SPI_0, SPI_ISR_PRIORITY, 0);
	break;
#endif
#if (SPI_USE_HW_UNIT_1 == STD_ON )
	case 1:
	ISR_INSTALL_ISR2("SPI_1",Spi_Isr_1, IRQ_SPI_1, SPI_ISR_PRIORITY, 0);
	break;
#endif
	}
#endif
}

void Spi_Hw_DeInitController(uint32 hwUnit ) {
    uint32_t intr_status_wtc;
    struct SPI_reg *spiHw = GET_SPI_HW_PTR(hwUnit);

    //Empty the FIFO.
    emptyRxTxFifo(GET_SPI_HW_PTR(hwUnit));

    spiHw->Config_reg0             = CONFIG_REG_RST_VALUE;
    //write to clear
    intr_status_wtc = spiHw->Intr_status_reg0;
    spiHw->Intr_status_reg0 = intr_status_wtc;

    spiHw->Intrpt_en_reg0          = INTRPT_EN_REG_RST_VALUE;
    spiHw->Intrpt_dis_reg0         = INTRPT_DIS_REG_RST_VALUE;
    spiHw->Delay_reg0              = DELAY_REG_RST_VALUE;
    spiHw->Slave_Idle_count_reg0   = SLAVE_IDLE_COUNT_REG_RST_VALUE;
    spiHw->TX_thres_reg0           = TX_THRES_REG_RST_VALUE;
    spiHw->RX_thres_reg0           = RX_THRES_REG_RST_VALUE;
    spiHw->En_reg0                 = EN_REG_RST_VALUE;
}



static void emptyRxFifo(struct SPI_reg *hwPtr)
{
    uint32_t dummyRead;

    while (hwPtr->Intr_status_reg0 & RX_FIFO_NOT_EMPTY_MASK) {
        dummyRead = hwPtr->Rx_data_reg0;
    }

    (void)dummyRead;
}

static void emptyTxFifo(struct SPI_reg *hwPtr)
{
    // Check if the tx fifo is empty. If its not it must be flushed.
    if (!((hwPtr->Intr_status_reg0 & TX_FIFO_NOT_EMPTY_MASK) >> 2)) {

        hwPtr->En_reg0 = 1;
        hwPtr->Config_reg0 |= (1<<16);
        while(!((hwPtr->Intr_status_reg0 & TX_FIFO_NOT_EMPTY_MASK) >> 2));
        hwPtr->En_reg0 = 0;

    }
}

static void emptyRxTxFifo(struct SPI_reg *hwPtr)
{
    //emptyTxFifo must be done first.
    emptyTxFifo(hwPtr);
    emptyRxFifo(hwPtr);
}

static Std_ReturnType spi_isTxFifoEmpty( struct SPI_reg *hwPtr )
{
    return ((hwPtr->Intr_status_reg0 & TX_FIFO_NOT_EMPTY_MASK) >> 2) != 0;
}



static sint32 checkJobTotalLength(Spi_UnitType *uPtr, uint16_t *numberOfChannels)
{
    Spi_NumberOfDataType totalLength = 0;
    Spi_NumberOfDataType chnLength = 0;
    sint32 retval = SPIE_OK;
    uint16_t n;
    uint16_t nbrOfChan = Spi_Internal_GetNbrChnInJob(uPtr);

    for(n = 0; n < nbrOfChan; n++) {

        Spi_Internal_GetTxChBuf( uPtr->jobPtr->SpiChannelAssignment[n]->SpiChannelId, &chnLength);
        totalLength += chnLength;
    }

    if (totalLength > FIFO_DEPTH) {
        retval = SPIE_BAD;
    }

    *numberOfChannels = n;
    return retval;
}



boolean Spi_Hw_IsTransmissionDone( uint32 hwUnit )
{
    return spi_isTxFifoEmpty(GET_SPI_HW_PTR(hwUnit));
}

void Spi_Hw_Halt( uint32 hwUnit ) {
    struct SPI_reg *hwPtr = GET_SPI_HW_PTR(hwUnit);
    hwPtr->En_reg0 = 0;
}

void Spi_Hw_DisableInterrupt( uint32 hwUnit ) {
    struct SPI_reg *hwPtr = GET_SPI_HW_PTR(hwUnit);
    uint32 intr_status_wtc = hwPtr->Intr_status_reg0;
    (void)intr_status_wtc;

    hwPtr->Intrpt_dis_reg0 = MASK_INTERRUPT_DISABLE;
}
void Spi_Hw_EnableInterrupt( uint32 hwUnit ) {
    struct SPI_reg *hwPtr = GET_SPI_HW_PTR(hwUnit);
    hwPtr->Intrpt_en_reg0 = TX_FIFO_NO_FULL;
}





