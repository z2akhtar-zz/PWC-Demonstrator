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
/** @tagSettings DEFAULT_ARCHITECTURE=RH850F1H */






// @todo just used for stubbed functions for start of rh850. Do not use later.










/* ----------------------------[includes]------------------------------------*/


#include <string.h>
#include "Std_Types.h"
#include "SchM_Mcu.h"
#include "Mcu.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#if defined(USE_DEM)
#include "Dem.h"
#endif
#include "Cpu.h"
#include "io.h"
#if defined(USE_KERNEL)
#include "irq.h"
#endif
#include "dr7f701503.dvf.h"



/* Global requirements */
/* !req SWS_Mcu_00130 */

/* API parameter checking */
/* @req SWS_Mcu_00017 OK*/
/* !req SWS_Mcu_00125 */


//#define USE_LDEBUG_PRINTF 1
#include "debug.h"
//#include "zynq.h"
//#include "irq_zynq.h"

/* ----------------------------[private define]------------------------------*/

#define CLK_CTRL_SRCSEL(_x)		((_x)<<4)

#define CLK_CTRL_DIVISOR_M 		0x3f00u
#define CLK_CTRL_DIVISOR1_M		0x3f00000u
#define CLK_CTRL_SRCSEL_M		0x30u


#define SRCSEL_ARM_PLL		0
#define SRCSEL_DDR_PLL		2
#define SRCSEL_IO_PLL		3


#define ZYNQ_SILICON_REV_1_0	0
#define ZYNQ_SILICON_REV_2_0	1
#define ZYNQ_SILICON_REV_3_0	2
#define ZYNQ_SILICON_REV_3_1	3

/* ----------------------------[private macro]-------------------------------*/

/* Development error macros. */
/* @req SWS_Mcu_00163 */
#if ( MCU_DEV_ERROR_DETECT == STD_ON )

/* Sanity check */
#if !defined(USE_DET)
#error DET not configured when MCU_DEV_ERROR_DETECT is set
#endif

#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          Det_ReportError(MCU_MODULE_ID,0,_api,_err); \
          return; \
        }

#define VALIDATE_W_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          Det_ReportError(MCU_MODULE_ID,0,_api,_err); \
          return (_rv); \
        }
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_W_RV(_exp,_api,_err,_rv )
#endif

#define MCU_ASSERT(_x)		if( !(_x) ) { while(1) {}; }

/* ----------------------------[private typedef]-----------------------------*/

/**
 * Type that holds all global data for Mcu
 */
typedef struct
{
    boolean 			 initRun;	   /* Set if Mcu_Init() have been called */
    const Mcu_ConfigType *config;	   /* Saved pointer to the configuration from Mcu_Init() */
    Mcu_ClockType        clockSetting; /* Saved clock setting from Mcu_InitClock() */
    Mcu_RawResetType rawResetvalue;
    Mcu_ResetType resetReason;
} Mcu_GlobalType;



/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/

Mcu_GlobalType Mcu_Global =
{
	.initRun = FALSE,
	.config = &McuConfigData[0],
};

/* ----------------------------[private functions]---------------------------*/



Mcu_ResetType Mcu_Arc_GetResetReason(void)
{
	return Mcu_Global.resetReason;
}

//static uint32 zynq_GetRevision( void ) {
//	return 0;//((DEVCFG_REG.MCTRL & 0xf0000000) >> 28);
//}

//static void zynq_GetAndClearResetCause(void) {
//	Mcu_ResetType resetReason = MCU_RESET_UNDEFINED;
//
//	SLCR.SLCR_UNLOCK = 0x0000DF0DU;	/* Unlock SLCR registers */
//
//	uint32 status = SLCR.REBOOT_STATUS;
//
//	/* Get RAW reset value and resetReson value and save it */
//	Mcu_Global.rawResetvalue = status;
//
//	if( status & ((1<<21) + (1<<19)) ) {					/* SRST_B (external reset) + SLC_RST (system software)  */
//		resetReason = MCU_SW_RESET;
//	} else if(status & ((1<<16) + (1<<17) + (1<<18)) ) {	/* SWDT_RST + AWDT0_RST + AWDT1_RST */
//		resetReason = MCU_WATCHDOG_RESET;
//	} else if( status & (1<<22) ) {							/* POR */
//		resetReason = MCU_POWER_ON_RESET;
//	}
//
//	Mcu_Global.resetReason = resetReason;
//
//	SLCR.SLCR_LOCK = 0x0000767BU;	/* Lock SLCR again */
//}


#if ( MCU_NO_PLL == STD_OFF )

/**
 * Init PLL's for Zynq 7020
 *
 * @return
 */
//static int32_t zynq_InitPll( const Mcu_ClockSettingConfigType *cPtr ) {
//
//	SLCR.SLCR_UNLOCK = 0x0000DF0DU;	/* Unlock SLCR registers */
//
//	READWRITE32((uint32)&SLCR.ARM_PLL_CFG,  0x003FFFF0U , cPtr->ARM_PLL_CFG );
//
//	READWRITE32((uint32)&SLCR.ARM_PLL_CTRL, 0x0007F000U , cPtr->ARM_PLL_CTRL);
//	/* Set PLL_BYPASS_FORCE  */
//	READWRITE32((uint32)&SLCR.ARM_PLL_CTRL, 0x00000010U ,0x00000010U);
//	/* Assert PLL_RESET */
//	READWRITE32((uint32)&SLCR.ARM_PLL_CTRL, 0x00000001U ,0x00000001U);
//	/* De-Assert PLL_RESET */
//	READWRITE32((uint32)&SLCR.ARM_PLL_CTRL, 0x00000001U ,0x00000000U);
//
//    /* Check ARM_PLL_LOCK Status */
//	while( !(SLCR.PLL_STATUS & 1) ) {};	/* IMPROVEMENT: Add timeout */
//
//	READWRITE32((uint32)&SLCR.ARM_PLL_CTRL, 0x00000010U ,0x00000000U);
//	READWRITE32((uint32)&SLCR.ARM_CLK_CTRL,
//						0x1F000000U | CLK_CTRL_DIVISOR_M | CLK_CTRL_SRCSEL_M  ,
//						cPtr->ARM_CLK_CTRL | CLK_CTRL_SRCSEL(SRCSEL_ARM_PLL));
//
//    /* IO_PLL Setup */
//	READWRITE32((uint32)&SLCR.IO_PLL_CFG,  0x003FFFF0U , cPtr->IO_PLL_CFG );
//
//    READWRITE32((uint32)&SLCR.IO_PLL_CTRL, 0x0007F000U , cPtr->IO_PLL_CTRL);
//    READWRITE32((uint32)&SLCR.IO_PLL_CTRL, 0x00000010U ,0x00000010U);
//    READWRITE32((uint32)&SLCR.IO_PLL_CTRL, 0x00000001U ,0x00000001U);
//    READWRITE32((uint32)&SLCR.IO_PLL_CTRL, 0x00000001U ,0x00000000U);
//
//    while( !(SLCR.PLL_STATUS & 4) ) {};	/* IMPROVEMENT: Add timeout */
//
//    READWRITE32((uint32)&SLCR.IO_PLL_CTRL, 0x00000010U ,0x00000000U);
//
//    SLCR.SLCR_LOCK = 0x0000767BU;	/* Lock SLCR again */
//
//    return 0;
//}


/**
 * Init clocks
 *
 * @param cPtr Pointer to the clock settings configuration
 * @return
 */
//static int32_t zynq_InitClocks( const Mcu_ClockSettingConfigType *cPtr  ) {
//
//	SLCR.SLCR_UNLOCK = 0x0000DF0DU;	/* Unlock SLCR registers */
//
//	/* CAN 0 and 1*/
//    READWRITE32((uint32)&SLCR.CAN_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_DIVISOR1_M + CLK_CTRL_SRCSEL_M + 3 ,
//    			cPtr->CAN_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//    READWRITE32( (uint32)&SLCR.CAN_MIOCLK_CTRL , 0xffffffff , cPtr->CAN_MIOCLK_CTRL);
//
//
//    /* Eth 0 */
//    READWRITE32((uint32)&SLCR.GEM0_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_DIVISOR1_M + CLK_CTRL_SRCSEL_M + 1 ,
//				cPtr->GEM0_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//
//    /* Eth 0 */
//    READWRITE32((uint32)&SLCR.GEM1_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_DIVISOR1_M + CLK_CTRL_SRCSEL_M + 1 ,
//				cPtr->GEM1_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//
//    /* SMC */
//    READWRITE32((uint32)&SLCR.SMC_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_SRCSEL_M + 1 ,
//    			cPtr->SMC_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//    /* QuadSPI */
//    READWRITE32((uint32)&SLCR.LQSPI_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_SRCSEL_M + 1 ,
//    			cPtr->LQSPI_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//
//
//
//    /* Skip FPGA clocks for now, ie PllFckDivisor_0 and PllFckDivisor_1 */
//
//
//    /* SDIO 0 and 1 */
//    READWRITE32((uint32)&SLCR.SDIO_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_SRCSEL_M + 3 ,
//    			cPtr->SDIO_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//
//    /* Uart 0 and 1 */
//    READWRITE32((uint32)&SLCR.UART_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_SRCSEL_M + 3 ,
//    			cPtr->UART_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//    /* SPI 0 and 1 */
//    READWRITE32((uint32)&SLCR.SPI_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_SRCSEL_M + 3 ,
//    			cPtr->SPI_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//    /* Trace, Skip for now */
//
//
//    /* PCAP */
//    READWRITE32((uint32)&SLCR.PCAP_CLK_CTRL,
//    			CLK_CTRL_DIVISOR_M + CLK_CTRL_SRCSEL_M + 1 ,
//    			cPtr->PCAP_CLK_CTRL + CLK_CTRL_SRCSEL(SRCSEL_IO_PLL));
//
//
//    /* Write APER */
//    {
//    	uint32 val = 	((cPtr->SMC_CLK_CTRL & 1)<<24) +
//						((cPtr->LQSPI_CLK_CTRL & 1)<<23) +
//						/* 22 GPIO */
//						((cPtr->UART_CLK_CTRL & 2)<<20) +
//						((cPtr->UART_CLK_CTRL & 1)<<20) +
//						/* 19-18 I2C */
//						((cPtr->CAN_CLK_CTRL & 2)<<16) +
//						((cPtr->CAN_CLK_CTRL & 1)<<16) +
//						((cPtr->SPI_CLK_CTRL & 2)<<14) +
//						((cPtr->SPI_CLK_CTRL & 1)<<14) +
//						/* 13,12 reserved */
//						((cPtr->SDIO_CLK_CTRL & 2)<<10) +
//						((cPtr->SDIO_CLK_CTRL & 1)<<10) +
//						/* 9,8 reserved */
//						((cPtr->GEM0_CLK_CTRL & 1)<<7) +
//						((cPtr->GEM1_CLK_CTRL & 1)<<6)
//						/* 1 reserved */
//						/* 0 DMA */
//						;
//
//        READWRITE32((uint32)&SLCR.APER_CLK_CTRL,
//        			(3<<23)+(3<<20)+(3<<16)+(3<<14)+(3<<10)+(3<<6),
//        			val);
//
//    }
//
//    SLCR.SLCR_LOCK = 0x0000767BU;	/* Lock SLCR again */
//
//	return 0;
//}

#endif /*  ( MCU_NO_PLL == STD_OFF ) */


/**
 * Get PLL status. Only ARM PLL is checked here..
 */
//static Mcu_PllStatusType zynq_GetPllStatus( void ) {
//	Mcu_PllStatusType rv = MCU_PLL_UNLOCKED;
////
////	SLCR.SLCR_UNLOCK = 0x0000DF0DU;	/* Unlock SLCR registers */
////
////	/* Check ARM_PLL_LOCK Status */
////	if ( SLCR.PLL_STATUS & 1) {
////		rv = MCU_PLL_LOCKED;
////	}
////
////	SLCR.SLCR_LOCK =  0x0000767BU;	/* Lock SLCR again */
//
//	return rv;
//}



/* ----------------------------[public functions]----------------------------*/



/* @req SWS_Mcu_00026  */
/* @req SWS_Mcu_00116  */
/* @req SWS_Mcu_00244  */
/* @req SWS_Mcu_00245  */
/* @req SWS_Mcu_00246  */
/* @req SWS_Mcu_00247  */
void Mcu_Init(const Mcu_ConfigType *ConfigPtr)
{
	/* @req SWS_Mcu_00018 */
	VALIDATE( ( NULL != ConfigPtr ), MCU_INIT_SERVICE_ID, MCU_E_PARAM_CONFIG );
//
//	/* Check Revision, assume backward compatible  */
//	MCU_ASSERT( zynq_GetRevision() >= ZYNQ_SILICON_REV_3_1 );
//
//    /* Get reset cause and clear */
//    zynq_GetAndClearResetCause();

    Mcu_Global.config = ConfigPtr;
    Mcu_Global.initRun = TRUE;
}



void Mcu_Arc_DeInit( void )
{
	Mcu_Global.initRun = FALSE; // Very simple Deinit. Should we do more?
}


//-------------------------------------------------------------------

Std_ReturnType Mcu_InitRamSection(Mcu_RamSectionType RamSection)
{
    /* @req SWS_Mcu_00021 */
    /* @req SWS_Mcu_00125 */
	VALIDATE_W_RV( ( TRUE == Mcu_Global.initRun ), MCU_INITRAMSECTION_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
	VALIDATE_W_RV( ( RamSection <= Mcu_Global.config->McuRamSectors ), MCU_INITRAMSECTION_SERVICE_ID, MCU_E_PARAM_RAMSECTION, E_NOT_OK );

	memset(	(void *)Mcu_Global.config->McuRamSectorSettingConfig[RamSection].McuRamSectionBaseAddress,
			Mcu_Global.config->McuRamSectorSettingConfig[RamSection].McuRamDefaultValue,
			Mcu_Global.config->McuRamSectorSettingConfig[RamSection].McuRamSectionSize);

    return E_OK;
}


#if ( MCU_GET_RAM_STATE == STD_ON )
Mcu_RamStateType Mcu_GetRamState( void )
{
	/* NOT SUPPORTED */
	return MCU_RAMSTATE_INVALID;
}
#endif 


//-------------------------------------------------------------------

#define protected_write(preg,reg,value)   preg=0xa5;\
                                          reg=value;\
                                          reg=~value;\
                                          reg=value;


static void init_clocks(void)
{

  /* Prepare 16MHz MainOsz */
  MOSCC=0x05;                            // Set MainOSC gain (8MHz < MOSC frequency =< 16MHz)
  MOSCST=0xFFFF;	                       // Set MainOSC stabilization time to max (8,19 ms)
  protected_write(PROTCMD0,MOSCE,0x01);  // Trigger Enable (protected write)
  while ((MOSCS&0x04) != 0x4);           // Wait for aktive MainOSC

  /* Prepare PLL0*/
  PLL0C=0x0000021D;                       //16 MHz MainOSC -> 120MHz PLL0
  protected_write(PROTCMD1,PLL0E,0x01);   //enable PLL
  while((PLL0S&0x04) != 0x04);            //Wait for aktive PLL

	/* Prepare PLL1*/
  PLL1C=0x00000a27;                       //16 MHz MainOSC -> 80MHz PLL1
  protected_write(PROTCMD1,PLL1E,0x01);   //enable PLL
  while((PLL1S&0x04) != 0x04);            //Wait for aktive PLL

  /* CPU Clock devider = PLL0/1 */
  protected_write(PROTCMD0,CKSC_CPUCLKD_CTL,0x01);
  while(CKSC_CPUCLKD_ACT!=0x01);

  /* PLL0 -> CPU Clock */
  protected_write(PROTCMD1,CKSC_CPUCLKS_CTL,0x03);
  while(CKSC_CPUCLKS_ACT!=0x03);

  /* PPLLCLK -> C_ISO_CAN */
  protected_write(PROTCMD1,CKSC_ICANS_CTL,0x2);
  while(CKSC_ICANS_ACT!=0x2);

  /* MainOSC -> C_ISO_CANOSC */
  protected_write(PROTCMD1,CKSC_ICANOSCD_CTL,0x1);
  while(CKSC_ICANOSCD_ACT!=0x1);
}


/* @req SWS_Mcu_00137  */
/* @req SWS_Mcu_00210  */
/* @req SWS_Mcu_00138 Waiting for PLL to look unless CFG_SIMULATOR is defined*/
#if ( MCU_INIT_CLOCK == STD_ON )
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting)
{
//    const Mcu_ClockSettingConfigType *clockSettingsPtr;
//
//    /* @req SWS_Mcu_00125  */
//    /* @req SWS_Mcu_00019  */
//    VALIDATE_W_RV( ( TRUE == Mcu_Global.initRun ), MCU_INITCLOCK_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
//    VALIDATE_W_RV( ( ClockSetting < Mcu_Global.config->McuClockSettings ), MCU_INITCLOCK_SERVICE_ID, MCU_E_PARAM_CLOCK, E_NOT_OK );
//
//    Mcu_Global.clockSetting = ClockSetting;
//    clockSettingsPtr = &Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting];
//
//    zynq_InitPll(clockSettingsPtr);
//    zynq_InitClocks(clockSettingsPtr);

	init_clocks();

    return E_OK;
}

#endif /* #if ( MCU_INIT_CLOCK == STD_ON )*/

//-------------------------------------------------------------------

/* @req SWS_Mcu_00205 */
#if ( MCU_NO_PLL == STD_OFF )
/* !req SWS_Mcu_00140  */
/* !req SWS_Mcu_00141 */
/* !req SWS_Mcu_00056 */
Std_ReturnType Mcu_DistributePllClock(void)
{
    /* @req SWS_Mcu_00125 */
    /* !req SWS_Mcu_00122 */

//    VALIDATE_W_RV( ( TRUE == Mcu_Global.initRun ), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
//    VALIDATE_W_RV( ( zynq_GetPllStatus() == MCU_PLL_LOCKED), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_PLL_NOT_LOCKED, E_NOT_OK);

    return E_OK;
}

#endif /* #if ( MCU_NO_PLL == STD_ON ) */

//-------------------------------------------------------------------

/* !req SWS_Mcu_00008 */
Mcu_PllStatusType Mcu_GetPllStatus(void)
{
//
//    /* !req SWS_Mcu_00206 */
//#if ( MCU_NO_PLL == STD_ON )
//    return MCU_PLL_STATUS_UNDEFINED;
//#else
//    /* @req SWS_Mcu_00125 */
//    /* @req SWS_Mcu_00132 */
//    VALIDATE_W_RV( ( TRUE == Mcu_Global.initRun ), MCU_GETPLLSTATUS_SERVICE_ID, MCU_E_UNINIT, MCU_PLL_STATUS_UNDEFINED );
//
//
//#if defined(CFG_SIMULATOR)
//    return MCU_PLL_LOCKED;
//#endif
//
//    return zynq_GetPllStatus();
//#endif
    return MCU_PLL_LOCKED;
}


//-------------------------------------------------------------------


/* !req SWS_Mcu_00005 */
Mcu_ResetType Mcu_GetResetReason(void)
{
	/* !req SWS_Mcu_00133 */
    /* @req SWS_Mcu_00125 */
	VALIDATE_W_RV( ( TRUE == Mcu_Global.initRun ), MCU_GETRESETREASON_SERVICE_ID, MCU_E_UNINIT, MCU_RESET_UNDEFINED );

	return Mcu_Global.resetReason;
}

//-------------------------------------------------------------------

/* @req SWS_Mcu_00235 */
/* @req SWS_Mcu_00006 */
Mcu_RawResetType Mcu_GetResetRawValue(void)
{
	/* @req SWS_Mcu_00135 */
    /* @req SWS_Mcu_00125 */
	VALIDATE_W_RV( ( TRUE == Mcu_Global.initRun ), MCU_GETRESETREASON_SERVICE_ID, MCU_E_UNINIT, MCU_GETRESETRAWVALUE_UNINIT_RV );

	if( !Mcu_Global.initRun ) {
		return MCU_GETRESETRAWVALUE_UNINIT_RV;
	}

	return Mcu_Global.rawResetvalue;
}

//-------------------------------------------------------------------

#if ( MCU_PERFORM_RESET_API == STD_ON )
/* @req SWS_Mcu_00143 */
/* !req SWS_Mcu_00144 */
void Mcu_PerformReset(void)
{
    /* @req SWS_Mcu_00125 */
//	VALIDATE( ( TRUE == Mcu_Global.initRun ), MCU_PERFORMRESET_SERVICE_ID, MCU_E_UNINIT );
//
//	SLCR.SLCR_UNLOCK = 0x0000DF0DU;	/* Unlock SLCR registers */
//	SLCR.PSS_RST_CTRL = 1; 			/* SOFT_RST */
//	SLCR.SLCR_LOCK =  0x0000767BU;	/* Lock SLCR again */
}
#endif

//-------------------------------------------------------------------


/* @req SWS_Mcu_00164 */
/* @req SWS_Mcu_00147 This is realized using the Mcu_Arc_SetModePre/Mcu_Arc_SetModePost functions */
void Mcu_SetMode( Mcu_ModeType mcuMode)
{

//    /* @req SWS_Mcu_00125 */
//    /* @req SWS_Mcu_00020 */
//    VALIDATE( ( TRUE == Mcu_Global.initRun ), MCU_SETMODE_SERVICE_ID, MCU_E_UNINIT );
//    VALIDATE( ( (mcuMode <= Mcu_Global.config->McuNumberOfMcuModes)), MCU_SETMODE_SERVICE_ID, MCU_E_PARAM_MODE );
//
//    /* Low Power Mode
//     * --------------------------------------
//     *   APU
//     *   SCU
//     *   L2
//     *   Peripherals
//     *   Clock
//     * */
//
//    /* Shutdown core 2 (if on) */
//
//    /* TODO: Not done at all */

}

void Mcu_GetVersionInfo( Std_VersionInfoType* versioninfo) {

    /* @req SWS_Mcu_00204 */
    VALIDATE( ( NULL != versioninfo ), MCU_GETVERSIONINFO_SERVICE_ID, MCU_E_PARAM_POINTER);

    versioninfo->vendorID = MCU_VENDOR_ID;
    versioninfo->moduleID = MCU_MODULE_ID;
    versioninfo->sw_major_version = MCU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = MCU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = MCU_SW_PATCH_VERSION;

}


/**
 * Get frequency of the oscillator
 */
uint32_t Mcu_Arc_GetClockReferencePointFrequency(void)
{
	return Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting].McuClockReferencePointFrequency;
}


/********************************************************************
 * Mcu_Arc_InitZero
 *
 * Function that perform all necessary initialization needed to to run software, such as disabling watchdog,
 * init ECC RAM, setup exception vectors etc
 ********************************************************************/


void Mcu_Arc_InitZero(void) {
//    /********************************************************************************
//     * NOTE
//     *
//     * This function will be called before BSS and DATA are initialized.
//     * Ensure that you do not access any global or static variables before
//     * BSS and DATA is initialized
//     ********************************************************************************/
//
//#if defined(USE_KERNEL)
//    Irq_Init();
//#endif
}


uint32_t Mcu_Arc_GetSystemClock(void)
{
    return 0;
}
