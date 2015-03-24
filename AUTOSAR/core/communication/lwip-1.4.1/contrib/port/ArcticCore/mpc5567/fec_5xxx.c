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

#include "mpc55xx.h"
#include "fec_5xxx.h"
#include "Mcu.h"
#include "lwip/pbuf.h"
#include "string.h"
#include "Dio.h"



/* Buf desc status macros */
#define BD_WRAP		0x2000
#define BD_INT		0x1000
#define BD_LAST		0x0800

#define RXBD_EMPTY		0x8000
#define RXBD_MISS		0x0100
#define RXBD_BCAST		0x0080
#define RXBD_MCAST		0x0040
#define RXBD_LG			0x0020
#define RXBD_NO			0x0010
#define RXBD_SH			0x0008
#define RXBD_CR			0x0004
#define RXBD_OV			0x0002
#define RXBD_TR			0x0001
#define RXBD_ERR		(RXBD_LG | RXBD_NO | RXBD_CR | RXBD_OV | RXBD_TR)

#define TXBD_READY		0x8000
#define TXBD_CRC		0x0400

/* MII macros */
#define FEC_MII_DATA_ST			0x40000000
#define FEC_MII_DATA_OP_RD		0x20000000
#define FEC_MII_DATA_OP_WR		0x10000000
#define FEC_MII_DATA_PA_MSK		0x0f800000
#define FEC_MII_DATA_RA_MSK		0x007c0000
#define FEC_MII_DATA_TA			0x00020000
#define FEC_MII_DATA_DATAMSK	0x0000ffff
#define FEC_MII_DATA_RA_SHIFT	18
#define FEC_MII_DATA_PA_SHIFT	23

#define ETH_MAX_PACKET_SIZE 1518
#define ETH_DESC_BUFFER_SIZE 1536

/* Adjusted for maximum download speed, possible to save RAM memory but at the cost of performance
 * Should be adjusted together with TCP_WND and TCP_SND_BUF.
 * At least FEC_NOF_TX_BUF >= (TCP_SND_BUF+1*1496)/1496 and FEC_NOF_RX_BUF >= (TCP_WND+1*1496)/1496.
 *
 * Going below 6 rx buffer will have significant impact on performance and going above 6 buffer only
 * marginally increases performance.
 */
#define FEC_NOF_TX_BUF 3
#define FEC_NOF_RX_BUF 6

static uint32 outOfMemoryCounter = 0;

typedef struct Desc
{
   volatile uint16 status;
   volatile uint16 length;
   volatile uint8 *pBuf;
} Desc_t;

/* Buffer descriptor rings must start
on a 32-bit boundary; however, it is recommended they are made 128-bit aligned. */
static Desc_t RxBD[FEC_NOF_RX_BUF] __attribute__ ((aligned (16)));// __attribute__ ((section (".no_cache")));
static Desc_t TxBD[FEC_NOF_TX_BUF] __attribute__ ((aligned (16)));// __attribute__ ((section (".no_cache")));

/* Buffers , let us set them aligned to just to be sure */
static uint8 RxBuf[FEC_NOF_RX_BUF][ETH_DESC_BUFFER_SIZE] __attribute__ ((aligned (16)));
static uint8 TxBuf[FEC_NOF_TX_BUF][ETH_DESC_BUFFER_SIZE] __attribute__ ((aligned (16)));

static volatile Desc_t *pRxBD = RxBD;
static volatile Desc_t *pTxBD = TxBD;

int fec_mii_read(uint8 phyAddr, uint8 regAddr, uint16 * retVal)
{
	uint32 reg;
	uint32 phy;
	volatile uint16 timeout = 0xffff;

	reg = regAddr << 18;
	phy = phyAddr << 23;

	FEC.MDATA.R = (FEC_MII_DATA_ST | FEC_MII_DATA_OP_RD | FEC_MII_DATA_TA | phy | reg);

	/* wait for int */
	while ((timeout--) && (!(FEC.EIR.R & 0x00800000))) ;

	if (timeout == 0) {
		return -1;
	}

	/* clear interrupt */
	FEC.EIR.R = 0x00800000;

	/* read data */
	*retVal = (uint16) FEC.MDATA.R;

	return 0;
}

int fec_mii_write(uint8 phyAddr, uint8 regAddr, uint32 data)
{
	uint32 reg;
	uint32 phy;
	volatile uint16 timeout = 0xffff;

	reg = regAddr << 18;
	phy = phyAddr << 23;

	FEC.MDATA.R = (FEC_MII_DATA_ST | FEC_MII_DATA_OP_WR | FEC_MII_DATA_TA | phy | reg | data);

	/* wait for int */
	while ((timeout--) && (!(FEC.EIR.R & 0x00800000))) ;
	if (timeout == 0) {
		return -1;
	}

	/* clear int */
	FEC.EIR.R = 0x00800000;

	return 0;
}

uint8 fec_find_phy(uint8 startPhyAddr, uint8 endPhyAddr)
{
	uint16 regValue = 0xffff;
	uint8 phyAddr=startPhyAddr;

	while( ( regValue == 0xffff ) || ( regValue == 0 ) )
	{
		fec_mii_read(phyAddr, 0x2, &regValue);
		if(( regValue == 0xffff ) || ( regValue == 0 )){
			phyAddr++;
			if(phyAddr > endPhyAddr){
				phyAddr = 0xff;
				regValue = 1;
			}
		}
	}
	return phyAddr;
}

int fec_init_phy(uint8 phyAddr)
{
	uint16 timeout;
	uint16 phyStatus;
	uint16 regValue;

	// Reset phy
	fec_mii_write(phyAddr, 0x0, 0x8000);
	for(volatile int i=0; i < 1000; i++){};

	/* Set auto neg advert register */
	fec_mii_write(phyAddr, 0x4, 0x01e1);

	/* enable auto-negotiation */
	fec_mii_read(phyAddr, 0x0, &regValue);
	fec_mii_write(phyAddr, 0x0, (regValue | 0x1200));

	/* Wait for completion */
	timeout = 10000;
	do {
		for(volatile int i=0; i < 1000; i++){
		}

		if ((timeout--) == 0) {
			return -1;
		}

		if (fec_mii_read(phyAddr, 0x1, &phyStatus) != 0) {
			return -1;
		}
		if(phyStatus == 0xffff){
			return -1;
		}
	} while (!(phyStatus & 0x0020)); // Should be 0x786D

	return 0;
}

void fec_set_macaddr(uint8 *macAddr)
{
	uint8 currByte;
	uint32 crc = 0xffffffff;

	/* Calc crc */
	for (int byte = 0; byte < 6; byte++) {
		currByte = macAddr[byte];
		for (int bit = 0; bit < 8; bit++) {
			if ((currByte & 0x01) ^ (crc & 0x01)) {
				crc >>= 1;
				crc = crc ^ 0xedb88320;
			} else {
				crc >>= 1;
			}
			currByte >>= 1;
		}
	}
	crc = crc >> 26;

	/* Set hash */
	if (crc >= 32) {
		FEC.IAUR.B.IADDR1 = (1 << (crc - 32));
		FEC.IALR.B.IADDR2 = 0;
	} else {
		FEC.IAUR.B.IADDR1 = 0;
		FEC.IALR.B.IADDR2 = (1 << crc);
	}

	/* Set phy addr */
	FEC.PALR.B.PADDR1 = (macAddr[0] << 24) + (macAddr[1] << 16) + (macAddr[2] << 8) + macAddr[3];
	FEC.PAUR.B.PADDR2 = (macAddr[4] << 24) + (macAddr[5] << 16) + 0x8808;
}

int fec_init(uint8 *macAddr)
{
	FEC.ECR.B.RESET = 1;
	FEC.ECR.B.ETHER_EN = 0;

#ifdef CFG_BRD_MPC5567FLEXECU2
#define PIN_FP_PROG	187
#define PIN_FPGA_DP_PD_B	137
#define PIN_FPGA_DP_PD_A	138
#define PIN_FPGA_SEL_SRC	203
#define PIN_FPGA_SW_RST	204
#define PIN_FPGA_DP_RST	205

	SIU.PCR[PIN_FP_PROG].R = 0x0303;
	SIU.PCR[PIN_FPGA_DP_PD_B].R = 0x0303;
	SIU.PCR[PIN_FPGA_DP_PD_A].R = 0x0303;
	SIU.PCR[PIN_FPGA_SEL_SRC].R = 0x0303;
	SIU.PCR[PIN_FPGA_SW_RST].R = 0x0303;
	SIU.PCR[PIN_FPGA_DP_RST].R = 0x0303;

	SIU.GPDO[PIN_FP_PROG].R = 0;
	SIU.GPDO[PIN_FPGA_SEL_SRC].R = 0;
	SIU.GPDO[PIN_FPGA_SW_RST].R = 0;
	SIU.GPDO[PIN_FPGA_DP_RST].R = 0;
	SIU.GPDO[PIN_FPGA_DP_PD_A].R = 0;
	SIU.GPDO[PIN_FPGA_DP_PD_B].R = 0;
	SIU.GPDO[PIN_FP_PROG].R = 1;
#endif

	/* Wait for reset to complete */
	for(volatile int i=0; i<1000; i++){
	}

#ifdef CFG_BRD_MPC5567FLEXECU2
	SIU.GPDO[PIN_FPGA_SW_RST].R = 1;
	SIU.GPDO[PIN_FPGA_DP_RST].R = 1;
#endif

	/*  Interrupt Mask Register  */
	FEC.EIMR.R = 0;
	/*  Interrupt Event Register */
	FEC.EIR.R = 0xFFFFFFFF; /* Clear event register */
	/* FIFO Transmit FIFO Watermark Register */
	FEC.TFWR.B.X_WMRK = 0x0; /* default 0=64bytes, 0x2=128bytes 0x3=192bytes */
	/* Set multicast address filter */
	FEC.GALR.R = 0;
	FEC.GAUR.R = 0;
	FEC.OPD.B.PAUSE_DUR = 0x1;
	/* Receive Control Register */
	FEC.RCR.B.FCE = 0;
	FEC.RCR.B.BC_REJ = 0;
	FEC.RCR.B.PROM = 0;
	FEC.RCR.B.MII_MODE = 1;
	FEC.RCR.B.DRT = 0;
	FEC.RCR.B.LOOP = 0;
	FEC.RCR.B.MAX_FL = ETH_MAX_PACKET_SIZE;
    FEC.RCR.R = 0x05EE0004; // NOTE: Compiler or debugger error? above don't work according to debugger.
	/* Transmit Control Register */
	FEC.TCR.B.TFC_PAUSE = 0;
	FEC.TCR.B.FDEN = 1;
	FEC.TCR.B.HBC = 0;
	FEC.TCR.B.GTS = 0;
	/* MII Speed Control Register */
	FEC.MSCR.B.DIS_PREAMBLE = 0;
	FEC.MSCR.B.MII_SPEED = Mcu_Arc_GetSystemClock() / (10*1000000); /* If system clock 80 MHz, value 8 -> 80 MHz * 1/(4*8) = 2.5 MHz */

	/* Receive Buffer Size Register */
	FEC.EMRBR.R = ETH_DESC_BUFFER_SIZE;

	/* Initialize descriptors */
	for(int i = 0; i < FEC_NOF_RX_BUF; i++)
	{
		if(i == (FEC_NOF_RX_BUF - 1)){
			RxBD[i].status = RXBD_EMPTY | BD_WRAP; /* wrap */
		}else{
			RxBD[i].status = RXBD_EMPTY;
		}
		RxBD[i].length = 0;
		RxBD[i].pBuf = RxBuf[i];
	}
	for(int i = 0; i < FEC_NOF_TX_BUF; i++)
	{
		if(i == (FEC_NOF_TX_BUF - 1)){
			TxBD[i].status = BD_WRAP; /* wrap */
		}else{
			TxBD[i].status = 0;
		}
		TxBD[i].length = 0;
		TxBD[i].pBuf = TxBuf[i];
	}

	/* Set desciptor start addresses */
	FEC.ERDSR.R = (uint32)&RxBD[0];
	FEC.ETDSR.R = (uint32)&TxBD[0];

	/* Set MAC address */
	fec_set_macaddr(macAddr);

	/* Initialize phy */
	int res = (-1);
	uint8 startPhyAddr = 0;
	do{
		/* find phy */
		uint8 phyAddr = fec_find_phy(startPhyAddr++,31);
		if(phyAddr != 0xff){
			res = fec_init_phy(phyAddr);
		}
	}while(res!=0);

	/* Enable FEC */
	FEC.ECR.B.ETHER_EN = 1;

	/* Enable events in maskreg */
#if 1
	/* IMPROVEMENT Add handling for all different errors before enabling here */
	FEC.EIMR.R = HEARTBEAT_ERROR | BABBLINGRECEIVER | BABBLINGTRANSMIT | GRACEFULSTOPACK |
			     /* TRANSMITFRAMEINT | TRANSMITBUFINT | */ RECEIVEFRAMEINT |
			     /* RECEIVEBUFINT | MIIIINT | */ ETHBUS_ERROR | LATECOLLISION |
			     COLLISIONRETRYLIMIT | TRANSMITFIFOUNDERRUN;
#else
	FEC.EIMR.R = RECEIVEFRAMEINT;
#endif

	return 0;
}

void fec_enable_reception()
{
	/* Enable reception */
	FEC.RDAR.B.R_DES_ACTIVE = 1;
}


uint8 * fec_get_buffer_to_send()
{
	uint8 *buf = NULL;

	if((pTxBD->status & TXBD_READY) == 0){
		buf = (uint8 *)pTxBD->pBuf;
	}
	return buf;
}

int fec_send(uint8 *buf, uint16 len)
{
	int ret = 0;

	if((pTxBD->status & TXBD_READY) == 0){
		/* Update buffer descriptor. */
		pTxBD->length = len;

		/* if caches flush them here... */

		pTxBD->pBuf = buf;

		/* Last descriptor, Wrap */
		if((pTxBD->status & BD_WRAP) == BD_WRAP)
		{
			pTxBD->status = TXBD_CRC | TXBD_READY | BD_LAST | BD_WRAP;
			pTxBD = &TxBD[0];
		}
		else
		{
			pTxBD->status = TXBD_CRC | TXBD_READY | BD_LAST;
			pTxBD++;
		}
		/* Send immediately */
		FEC.TDAR.B.X_DES_ACTIVE = 1;
	}
	else
	{
		ret = 1;
	}

	return ret;
}

boolean fec_is_rx_data_available()
{
	return ((pRxBD->status & RXBD_EMPTY) == 0);
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
struct pbuf *
low_level_input()
{
	struct pbuf *p = NULL;

	if((pRxBD->status & RXBD_EMPTY) == 0)
	{
		/* We allocate a pbuf chain of pbufs from the pool. */
		p = pbuf_alloc(PBUF_RAW, pRxBD->length, PBUF_POOL);
		if (p != NULL){
			memcpy((uint8_t*)p->payload, (uint8_t*)pRxBD->pBuf, pRxBD->length);
		}else{
			/* Out of memory, Nothing to do but to through away data */
			outOfMemoryCounter++;
 		}

		/* Update descriptor for new reception */
		if(pRxBD->status & BD_WRAP){ /* Last descriptor, Wrap */
			pRxBD->length = 0;
			pRxBD->status = RXBD_EMPTY | BD_WRAP;
			pRxBD = &RxBD[0];
		}
		else{
			pRxBD->length = 0;
			pRxBD->status = RXBD_EMPTY;
			pRxBD++;
		}
	}

	return p;
}


