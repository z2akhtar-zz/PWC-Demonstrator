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

#ifndef ZYNQ_H_
#define ZYNQ_H_


#define ZYNQ_SLCR_BASE      0xF8000000

#define ZYNQ_DEVCFG_BASE 	0xF8007000

#define ZYNQ_MPCORE_BASE    0xF8F00000
#define ZYNQ_SPI_BASE		0xE0006000

/* zynq RM B.19 */
#define ZYNQ_GPIO_BASE		0xE000A000

/* CPU register MAP, see chapter 4.4 in RM */
#define ZYNQ_SCU_BASE   0xF8F00000
#define ZYNQ_GIC_BASE   0xF8F00100
/* Miscellaneous PS registers, see chapter 4.7 in RM */
#define ZYNQ_DEVCFG_BASE 0xF8007000

/* TTC RM B.32 */

#define ZYNQ_TTC0_BASE	0xF8001000
#define ZYNQ_TTC1_BASE	0xF8002000

/* SWDT B.31 */
#define ZYNQ_SWDT_BASE  0xF8005000

#include "Arc_Types.h"

typedef union {                     /* 0x100 */
    vuint32_t R;
    struct {
        vuint32_t :10;
        vuint32_t LOCK_CNT:10;
        vuint32_t PLL_CP:4;
        vuint32_t PLL_RES:4;
        vuint32_t :4;
    } B;
} ARM_PLL_CTRL_t;



struct SLCR_reg {
    vuint32_t SCL;                  /* 0x00000000 Secure Configuration Lock    */
    vuint32_t SLCR_LOCK;            /* 0x00000004 SLCR Write Protection Lock   */
    vuint32_t SLCR_UNLOCK;          /* 0x00000008 SLCR Write Protection Unlock */
    vuint32_t SLCR_LOCKSTA;         /* 0x0000000c SLCR Write Protection Status */
    vuint32_t pad_1[60];
    ARM_PLL_CTRL_t ARM_PLL_CTRL;
    vuint32_t DDR_PLL_CTRL;
    vuint32_t IO_PLL_CTRL;
    vuint32_t PLL_STATUS;
    vuint32_t ARM_PLL_CFG;          /* 0x00000110  ARM PLL Configuration   */
    vuint32_t DDR_PLL_CFG;          /* 0x00000114  DDR PLL Configuration   */
    vuint32_t IO_PLL_CFG;           /* 0x00000118  IO PLL Configuration    */
    vuint32_t pad_2;
    vuint32_t ARM_CLK_CTRL;         /* 0x00000120  CPU Clock Control       */
    vuint32_t DDR_CLK_CTRL;         /* 0x00000124  DDR Clock Control       */
    vuint32_t DCI_CLK_CTRL;         /* 0x00000128  DCI clock control        */
    vuint32_t APER_CLK_CTRL;        /* 0x0000012C  AMBA Peripheral Clock Control */
    vuint32_t USB0_CLK_CTRL;        /* 0x00000130  USB 0 ULPI Clock control */
    vuint32_t USB1_CLK_CTRL;        /* 0x00000134  USB 1 ULPI Clock control */
    vuint32_t GEM0_RCLK_CTRL;       /* 0x00000138  GigE 0 Rx Clock and Rx Signals */
    vuint32_t GEM1_RCLK_CTRL;       /* 0x0000013C  GigE 1 Rx Clock and Rx Signals */
    vuint32_t GEM0_CLK_CTRL;        /* 0x00000140  GigE 0 Ref Clock Control */
    vuint32_t GEM1_CLK_CTRL;        /* 0x00000144  GigE 1 Ref Clock Control */
    vuint32_t SMC_CLK_CTRL;         /* 0x00000148  SMC Ref Clock Control */
    vuint32_t LQSPI_CLK_CTRL;       /* 0x0000014C  Quad SPI Ref Clock Control */
    vuint32_t SDIO_CLK_CTRL;        /* 0x00000150  SDIO Ref Clock Control */
    vuint32_t UART_CLK_CTRL;        /* 0x00000154  UART Ref Clock Control */
    vuint32_t SPI_CLK_CTRL;         /* 0x00000158  SPI Ref Clock Control */
    vuint32_t CAN_CLK_CTRL;         /* 0x0000015C  CAN Ref Clock Control */
    vuint32_t CAN_MIOCLK_CTRL;      /* 0x00000160 CAN MIO Clock Control */
    vuint32_t DBG_CLK_CTRL;         /* 0x00000164 SoC Debug Clock Control */
    vuint32_t PCAP_CLK_CTRL;        /* 0x00000168 PCAP Clock Control */
    vuint32_t TOPSW_CLK_CTRL;       /* 0x0000016C Central Interconnect Clock */
	vuint32_t pad_3[0x90/4];		/* 0x00000170-- */
	vuint32_t PSS_RST_CTRL;			/* 0x00000200 PS Software Reset Control */
	vuint32_t pad_4[(0x22C-0x200-4)/4];
    vuint32_t GPIO_RST_CTRL;    	/* 0x0000022C GPIO reset control */
	vuint32_t pad_5[(0x250-0x22c-4)/4];
	vuint32_t RST_REASON;			/* 0x00000250 Reset status */
	vuint32_t RST_REASON_CLR;
	vuint32_t REBOOT_STATUS;		/* 0x00000258 Reboot Status, persistent */
	vuint32_t pad_51[(0x530-0x258-4)/4];
	vuint32_t PSS_IDCODE;			/* 0x00000530 32 ro x PS IDCODE */
	vuint32_t pad_6[(0x700-0x530-4)/4];
	vuint32_t MIO_PIN[54];       	/* 0x00000700 to 0x000007D4 MIO Pin Control*/
	vuint32_t pad_7[(0x804-0x7D4-4)/4];
    vuint32_t MIO_LOOPBACK;     	/* 0x00000804 Loopback within MIO */
    vuint32_t UNUSED_4;
    vuint32_t MIO_MST_TRI0;     	/* 0x0000080C Tri State enable */
    vuint32_t MIO_MST_TRI1;     	/* 0x00000810 Tri State enable */
    vuint32_t pad_8[(0x900-0x810-4)/4];
    vuint32_t LVL_SHFTR_EN;
};


/* In the zynq RM these registers are found under B.24 (mpcore) */

/* GIC */
struct GIC_PL390_reg {
    vuint32_t ICCICR;   /* 0x00000100 32 rw 0x00000000 CPU Interface Control Register */
    vuint32_t ICCPMR;   /* 0x00000104 32 rw 0x00000000 Interrupt Priority Mask Register */
    vuint32_t ICCBPR;   /* 0x00000108 32 rw 0x00000002 Binary Point Register */
    vuint32_t ICCIAR;   /* 0x0000010C 32 rw 0x000003FF Interrupt Acknowledge Register */
    vuint32_t ICCEOIR;  /* 0x00000110 32 rw 0x00000000 End Of Interrupt Register */
    vuint32_t ICCRPR;   /* 0x00000114 32 rw 0x000000FF Running Priority Register */
    vuint32_t ICCHPIR;  /* 0x00000118 32 rw 0x000003FF Highest Pending Interrupt Register */
    vuint32_t ICCABPR;  /* 0x0000011C 32 rw 0x00000003 Aliased Non-secure Binary */
    vuint32_t res[220];
    vuint32_t ICCIDR;   /* 0x000001FC 32 ro 0x3901243B CPU Interface Implementer Identification Register */

};


#if 1
typedef union {
    vuint32_t R;
    struct {
        vuint32_t SGIINTID:4;
        vuint32_t SBZ:11;
        vuint32_t SATT:1;
        vuint32_t CPU:8;
        vuint32_t TRGFILT:2;
        vuint32_t :6;
    } B;
} ICDSGIR_t;

#endif

/* Interrupt Distributor
 * */

struct GIC_DIST_PL390_reg {
    vuint32_t ICDDCR;           /* 0x000 */
    vuint32_t ICDICTR;          /* 0x004 */
    vuint32_t CDIIDR;           /* 0x008 */
    vuint32_t res[29];          /* 0x00c -> 0x07c */
    vuint32_t ICDISR_SPI[32];   /* 0x080 Interrupt Security Registers */
    vuint32_t ICDISER[32];      /* 0x100 Enable Set Registers */
    vuint32_t ICDICER[32];      /* 0x180 Enable Clear Registers */
    vuint32_t ICDISPR[32];      /* 0x200 Pending Set Registers */
    vuint32_t ICDICPR[32];      /* 0x280 Pending Clear Registers */
    vuint32_t ICDABR[32];       /* 0x300 Active Status Registers */
    vuint32_t res5[32];
    vuint32_t ICDIPR[64*4];     /* 0x400 Priority Level Registers */
    vuint32_t ICDIPTR[64*4];        /* 0x800 Target Registers */
    vuint32_t ICDICR[64];       /* 0xc00 Interrupt Configuration Registers */
    vuint32_t status[64];       /* 0xd00 Status Registers + test */
    vuint32_t pad[64];          /* 0xe00 */
    ICDSGIR_t ICDSGIR;          /* 0xf00 */
    vuint32_t pad2[51];
    vuint32_t ICPIDR[8];            /* 0xfd0 */
    vuint32_t ICCIDR[4];            /* 0xff0 */

    /* .. */
};

/* Device Configuration interface (devcfg). These registers are found under chapter B.16 of Zynq RM. */
struct DEVCFG_reg {
    uint32_t CTRL;              /* 0x00000000 32 mixed 0x0C006000 Basic control registers. */
    uint32_t LOCK;              /* 0x00000004 32 mixed 0x00000000 Lock changes in the Control Register */
    uint32_t CFG;               /* 0x00000008 32 rw    0x00000508 Configuration information */
    uint32_t INT_STS;           /* 0x0000000C 32 mixed 0x00000000 Interrupt Status Register */
    uint32_t INT_MASK;          /* 0x00000010 32 rw    0xFFFFFFFF Interrupt Mask Register */
    uint32_t STATUS;            /* 0x00000014 32 mixed 0x40000820 Status Register */
    uint32_t DMA_SRC_ADDR;      /* 0x00000018 32 rw    0x00000000 DMA Source address Register */
    uint32_t DMA_DST_ADDR;      /* 0x0000001C 32 rw    0x00000000 DMA Destination address Register */
    uint32_t DMA_SRC_LEN;       /* 0x00000020 32 rw    0x00000000 DMA Source transfer Length Register */
    uint32_t DMA_DEST_LEN;      /* 0x00000024 32 rw    0x00000000 DMA Destination transfer Length Register */
    uint32_t ROM_SHADOW;        /* 0x00000028 32 wo    0x00000000 ROM Shadow Register */
    uint32_t MULTIBOOT_ADDR;    /* 0x0000002C 13 rw    0x00000000 MULTI Boot Addr Pointer Register */
    uint32_t SW_ID;             /* 0x00000030 32 mixed 0x00000000 Software ID Register */
    uint32_t UNLOCK;            /* 0x00000034 32 rw    0x00000000 Unlock Register */
    uint32_t padding_1[18];
    uint32_t MCTRL;             /* 0x00000080 32 mixed x          Miscellaneous control Register */
    uint32_t padding_2[31];
    uint32_t XADCIF_CFG;        /* 0x00000100 32 rw    0x00001114 XADC Interface Configuration Register */
    uint32_t XADCIF_INT_STS;    /* 0x00000104 32 mixed 0x00000200 XADC Interface Interrupt Status Register */
    uint32_t XADCIF_INT_MASK;   /* 0x00000108 32 rw    0xFFFFFFFF XADC Interface Interrupt Mask Register */
    uint32_t XADCIF_MSTS;       /* 0x0000010C 32 ro    0x00000500 XADC Interface miscellaneous Status Register */
    uint32_t XADCIF_CMDFIFO;    /* 0x00000110 32 wo    0x00000000 XADC Interface Command FIFO Register */
    uint32_t XADCIF_RDFIFO;     /* 0x00000114 32 ro    0x00000000 XADC Interface Data FIFO Register */
    uint32_t XADCIF_MCTL;       /* 0x00000118 32 rw    0x00000010 XADC Interface Miscellaneous Control Register */
};

struct AXI_XADC_reg {
    uint32_t SSR;
    uint32_t SR;
    uint32_t AOSR;
    uint32_t CONVSTR;
    uint32_t SYSMONRR;
    uint32_t padding_1[18];
    uint32_t GIER;
    uint32_t IPISR;
    uint32_t padding_2;
    uint32_t IPIER;
    uint32_t padding_3[101];
    uint32_t TEMP;
    uint32_t VCCINT;
    uint32_t VCCAUX;
    uint32_t VP_VN;
    uint32_t VREFP;
    uint32_t VREFN;
    uint32_t VBRAM;
    uint32_t UNDEFINED1;
    uint32_t SUP_OFFSET;
    uint32_t ADC_OFFSET;
    uint32_t GAIN_ERR;
    uint32_t UNDEFINED2[2];
    uint32_t VCCPINT;
    uint32_t VCCPAUX;
    uint32_t VCC_DRR;
    uint32_t VAUX0_PN;
    uint32_t VAUX1_PN;
    uint32_t VAUX2_PN;
    uint32_t VAUX3_PN;
    uint32_t VAUX4_PN;
    uint32_t VAUX5_PN;
    uint32_t VAUX6_PN;
    uint32_t VAUX7_PN;
    uint32_t VAUX8_PN;
    uint32_t VAUX9_PN;
    uint32_t VAUX10_PN;
    uint32_t VAUX11_PN;
    uint32_t VAUX12_PN;
    uint32_t VAUX13_PN;
    uint32_t VAUX14_PN;
    uint32_t VAUX15_PN;
    uint32_t padding_4[31];
    uint32_t FLAG_REG;
    uint32_t CONFG0;
    uint32_t CONFG1;
    uint32_t CONFG2;
    uint32_t TESTREG[5];
    uint32_t SEQ0;
    uint32_t SEQ1;
    uint32_t SEQ2;
    uint32_t SEQ3;
    uint32_t SEQ4;
    uint32_t SEQ5;
    uint32_t SEQ6;
    uint32_t SEQ7;
    uint32_t ALM0;
    uint32_t ALM1;
    uint32_t ALM2;
    uint32_t ALM3;
    uint32_t ALM4;
    uint32_t ALM5;
    uint32_t ALM6;
    uint32_t ALM7;
};

struct MPCORE_reg {
    uint8_t scu[0x100];
    struct GIC_PL390_reg        ICCI;
    struct GIC_DIST_PL390_reg   ICDI;
};

struct PRIVATE_TIMER_reg {
    vuint32_t load;
    vuint32_t counter;
    vuint32_t control;
    vuint32_t status;
};

struct GLOBAL_TIMER_reg {
	volatile uint32_t counterLow;
	volatile uint32_t counterHigh;
	volatile uint32_t control;
	volatile uint32_t intStatus;
	volatile uint32_t comparatorLow;
	volatile uint32_t comparatorHigh;
	volatile uint32_t autoIncrement;
};

/* GPIO registers for PS */
struct GPIO_reg {

    vuint32_t MASK_DATA_0_LSW;       /* 0x00000000 Maskable Output Data */
    vuint32_t MASK_DATA_0_MSW;       /* 0x00000004 Maskable Output Data */
    vuint32_t MASK_DATA_1_LSW;       /* 0x00000008 Maskable Output Data */
    vuint32_t MASK_DATA_1_MSW;       /* 0x0000000C Maskable Output Data */
    vuint32_t MASK_DATA_2_LSW;       /* 0x00000010 Maskable Output Data */
    vuint32_t MASK_DATA_2_MSW;       /* 0x00000014 Maskable Output Data */
    vuint32_t MASK_DATA_3_LSW;       /* 0x00000018 Maskable Output Data */
    vuint32_t MASK_DATA_3_MSW;       /* 0x0000001C Maskable Output Data */
    vuint32_t UNUSED_1[8];           /* 0x00000020 to 0x0000003C */
    vuint32_t DATA_0;                /* 0x00000040 Output Data */
    vuint32_t DATA_1;                /* 0x00000044 Output Data */
    vuint32_t DATA_2;                /* 0x00000048 Output Data */
    vuint32_t DATA_3;                /* 0x0000004C Output Data */
    vuint32_t UNUSED_2[4];           /* 0x00000050 to 0x0000005C */
    vuint32_t DATA_0_RO;             /* 0x00000060 Input Data */
    vuint32_t DATA_1_RO;             /* 0x00000064 Input Data */
    vuint32_t DATA_2_RO;             /* 0x00000068 Input Data */
    vuint32_t DATA_3_RO;             /* 0x0000006C Input Data */
    vuint32_t UNUSED_3[101];          /* 0x00000070 to 0x00000200 */
    vuint32_t DIRM_0;                /* 0x00000204 Direction Mode */
    vuint32_t OEN_0;                 /* 0x00000208 Output Enable */
    vuint32_t INT_MASK_0;            /* 0x0000020C Interrupt Mask status */
    vuint32_t INT_EN_0;              /* 0x00000210 Interrupt Enable/Unmask */
    vuint32_t INT_DIS_0;             /* 0x00000214 Interrupt Disable/Mask */
    vuint32_t INT_STAT_0;            /* 0x00000218 Interrupt Status */
    vuint32_t INT_TYPE_0;            /* 0x0000021C Interrupt Type */
    vuint32_t INT_POLARITY_0;        /* 0x00000220 Interrupt Polarity */
    vuint32_t INT_ANY_0;             /* 0x00000224 Interrupt Any edge sensitive */
    vuint32_t UNUSED_4[7];           /* 0x00000228 to 0x00000240 */
    vuint32_t DIRM_1;                /* 0x00000244 Input Data */
    vuint32_t OEN_1;                 /* 0x00000248 Input Data */
    vuint32_t INT_MASK_1;            /* 0x0000024C Input Data */
    vuint32_t INT_EN_1;              /* 0x00000250 Input Data */
    vuint32_t INT_DIS_1;             /* 0x00000254 Input Data */
    vuint32_t INT_STAT_1;            /* 0x00000258 Input Data */
    vuint32_t INT_TYPE_1;            /* 0x0000025C Input Data */
    vuint32_t INT_POLARITY_1;        /* 0x00000260 Input Data */
    vuint32_t INT_ANY_1;             /* 0x00000264 Input Data */
    vuint32_t UNUSED_5[7];           /* 0x00000268 to 0x00000280 */
    vuint32_t DIRM_2;                /* 0x00000284 Direction Mode */
    vuint32_t OEN_2;                 /* 0x00000288 Output Enable */
    vuint32_t INT_MASK_2;            /* 0x0000028C Interrupt Mask status */
    vuint32_t INT_EN_2;              /* 0x00000290 Interrupt Enable/Unmask */
    vuint32_t INT_DIS_2;             /* 0x00000294 Interrupt Disable/Mask */
    vuint32_t INT_STAT_2;            /* 0x00000298 Interrupt Status */
    vuint32_t INT_TYPE_2;            /* 0x0000029C Interrupt Type */
    vuint32_t INT_POLARITY_2;        /* 0x000002A0 Interrupt Polarity */
    vuint32_t INT_ANY_2;             /* 0x000002A4 Interrupt Any edge sensitive */
    vuint32_t UNUSED_6[7];           /* 0x000002A8 to 0x000002C0 */
    vuint32_t DIRM_3;                /* 0x000002C4 Direction Mode */
    vuint32_t OEN_3;                 /* 0x000002C8 Output Enable */
    vuint32_t INT_MASK_3;            /* 0x000002CC Interrupt Mask status */
    vuint32_t INT_EN_3;              /* 0x000002D0 Interrupt Enable/Unmask */
    vuint32_t INT_DIS_3;             /* 0x000002D4 Interrupt Disable/Mask */
    vuint32_t INT_STAT_3;            /* 0x000002D8 Interrupt Status */
    vuint32_t INT_TYPE_3;            /* 0x000002DC Interrupt Type */
    vuint32_t INT_POLARITY_3;        /* 0x000002E0 Interrupt Polarity */
    vuint32_t INT_ANY_3;             /* 0x000002E4 Interrupt Any edge sensitive */

};


struct SPI_reg {
	vuint32_t Config_reg0; 				/* 0x00000000 SPI configuration register     */
	vuint32_t Intr_status_reg0;			/* 0x00000004 SPI interrupt status register  */
	vuint32_t Intrpt_en_reg0; 			/* 0x00000008 Interrupt Enable register      */
	vuint32_t Intrpt_dis_reg0; 			/* 0x0000000C Interrupt disable register     */
	vuint32_t Intrpt_mask_reg0;			/* 0x00000010 Interrupt mask register        */
	vuint32_t En_reg0; 					/* 0x00000014 SPI_Enable Register            */
	vuint32_t Delay_reg0; 				/* 0x00000018 Delay Register                 */
	vuint32_t Tx_data_reg0;				/* 0x0000001C Transmit Data Register.        */
	vuint32_t Rx_data_reg0;				/* 0x00000020 Receive Data Register          */
	vuint32_t Slave_Idle_count_reg0;   	/* 0x00000024 Slave Idle Count Register      */
	vuint32_t TX_thres_reg0; 			/* 0x00000028 TX_FIFO Threshold Register     */
	vuint32_t RX_thres_reg0; 			/* 0x0000002C RX FIFO Threshold Register     */
	vuint32_t padding[51];
	vuint32_t Mod_id_reg0; 				/* 0x000000FC Module ID register	         */
};

/* TTC registers for PS */
struct TTC_reg {
    vuint32_t CLOCK_CONTROL_1; 			/* 0x00000000 7 rw 0x00000000 Clock Control register */
    vuint32_t CLOCK_CONTROL_2; 			/* 0x00000004 7 rw 0x00000000 Clock Control register */
    vuint32_t CLOCK_CONTROL_3; 			/* 0x00000008 7 rw 0x00000000 Clock Control register */
    vuint32_t COUNTER_CONTROL_1; 		/* 0x0000000C 7 rw 0x00000021 Operational mode and reset */
    vuint32_t COUNTER_CONTROL_2; 		/* 0x00000010 7 rw 0x00000021 Operational mode and reset */
    vuint32_t COUNTER_CONTROL_3; 		/* 0x00000014 7 rw 0x00000021 Operational mode and reset */
    vuint32_t COUNTER_VALUE_1; 			/* 0x00000018 16 ro 0x00000000 Current counter value */
    vuint32_t COUNTER_VALUE_2; 			/* 0x0000001C 16 ro 0x00000000 Current counter value */
    vuint32_t COUNTER_VALUE_3; 			/* 0x00000020 16 ro 0x00000000 Current counter value */
    vuint32_t INTERVAL_COUNTER_1; 		/* 0x00000024 16 rw 0x00000000 Interval value */
    vuint32_t INTERVAL_COUNTER_2; 		/* 0x00000028 16 rw 0x00000000 Interval value */
    vuint32_t INTERVAL_COUNTER_3; 		/* 0x0000002C 16 rw 0x00000000 Interval value */
    vuint32_t MATCH_1_COUNTER_1; 		/* 0x00000030 16 rw 0x00000000 Match value */
    vuint32_t MATCH_1_COUNTER_2; 		/* 0x00000034 16 rw 0x00000000 Match value */
    vuint32_t MATCH_1_COUNTER_3; 		/* 0x00000038 16 rw 0x00000000 Match value */
    vuint32_t MATCH_2_COUNTER_1; 		/* 0x0000003C 16 rw 0x00000000 Match value */
    vuint32_t MATCH_2_COUNTER_2; 		/* 0x00000040 16 rw 0x00000000 Match value */
    vuint32_t MATCH_2_COUNTER_3; 		/* 0x00000044 16 rw 0x00000000 Match value */
    vuint32_t MATCH_3_COUNTER_1; 		/* 0x00000048 16 rw 0x00000000 Match value */
    vuint32_t MATCH_3_COUNTER_2; 		/* 0x0000004C 16 rw 0x00000000 Match value */
    vuint32_t MATCH_3_COUNTER_3; 		/* 0x00000050 16 rw 0x00000000 Match value */
	vuint32_t INTERRUPT_REGISTER_1; 	/* 0x00000054 6 clronrd 0x00000000 Counter 1 Interval, Match, Overflow and Event interrupts */
	vuint32_t INTERRUPT_REGISTER_2; 	/* 0x00000058 6 clronrd 0x00000000 Counter 2 Interval, Match,Overflow and Event interrupts */
	vuint32_t INTERRUPT_REGISTER_3; 	/* 0x0000005C 6 clronrd 0x00000000 Counter 3 Interval, Match, Overflow and Event interrupts */
	vuint32_t INTERRUPT_ENABLE_1; 		/* 0x00000060 6 rw 0x00000000 ANDed with corresponding Interrupt Register */
	vuint32_t INTERRUPT_ENABLE_2; 		/* 0x00000064 6 rw 0x00000000 ANDed with corresponding Interrupt Register */
	vuint32_t INTERRUPT_ENABLE_3; 		/* 0x00000068 6 rw 0x00000000 ANDed with corresponding Interrupt Register */
	vuint32_t EVENT_CONTROL_TIMER_1; 	/* 0x0000006C 3 rw 0x00000000 Enable, pulse and overflow */
	vuint32_t EVENT_CONTROL_TIMER_2; 	/* 0x00000070 3 rw 0x00000000 Enable, pulse and overflow */
	vuint32_t EVENT_CONTROL_TIMER_3; 	/* 0x00000074 3 rw 0x00000000 Enable, pulse and overflow */
	vuint32_t EVENT_REGISTER_1; 		/* 0x00000078 16 ro 0x00000000 pclk cycle count for event */
	vuint32_t EVENT_REGISTER_2; 		/* 0x0000007C 16 ro 0x00000000 pclk cycle count for event */
	vuint32_t EVENT_REGISTER_3; 		/* 0x00000080 16 ro 0x00000000 pclk cycle count for event */
};


/* SWDT registers */
struct SWDT_reg {
    vuint32_t mode;         /* 0x00000000 WD zero mode register */
    vuint32_t control;      /* 0x00000004 Counter Control Register */
    vuint32_t restart;      /* 0x00000008 Restart key register */
    vuint32_t status;       /* 0x0000000C Status Register */
};


#define MPCORE          (*(struct SLCR_reg *)ZYNQ_MPCORE_BASE)
#define ICCI            (*(struct GIC_PL390_reg *)(ZYNQ_MPCORE_BASE+0x100))
#define GLOBAL_TIMER 	(*(struct GLOBAL_TIMER_reg *)(ZYNQ_MPCORE_BASE+0x200))
#define PRIVATE_TIMER   (*(struct PRIVATE_TIMER_reg *)(ZYNQ_MPCORE_BASE+0x600))

#define ICD             (*(struct GIC_DIST_PL390_reg *)(ZYNQ_MPCORE_BASE+0x1000))

#define SLCR        (*(struct SLCR_reg *)ZYNQ_SLCR_BASE)
//#define mpcore        *(vuint32_t)ZYNQ_GIC_BASE
#define GPIO_REG    (*(struct GPIO_reg *)ZYNQ_GPIO_BASE)
#define TTC0_REG    (*(struct TTC_reg *)ZYNQ_TTC0_BASE)
#define TTC1_REG    (*(struct TTC_reg *)ZYNQ_TTC1_BASE)
#define SWDT_REG    (*(struct SWDT_reg *)ZYNQ_SWDT_BASE)

#define SPI0_REG	(*(struct SPI_reg *)ZYNQ_SPI_BASE)
#define SPI1_REG	(*(struct SPI_reg *)(ZYNQ_SPI_BASE+0x1000)



#define DEVCFG_REG	(*(struct DEVCFG_reg *)ZYNQ_DEVCFG_BASE)

#endif /* ZYNQ_H_ */
