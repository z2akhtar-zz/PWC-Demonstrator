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

/** @tagSettings DEFAULT_ARCHITECTURE=TMS570 */
/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */


#include "Std_Types.h"
#include "Mcu.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#if defined(USE_DEM)
#include "Dem.h"
#endif
#include <assert.h>
#include "Cpu.h"
#include <string.h>
#include "Ramlog.h"
#include "irq.h"

#include "core_cr4.h"

//#define USE_LDEBUG_PRINTF 1
#include "debug.h"

/* PLLCTL1 register */
#define	MCU_RESET_ON_SLIP	0  				// Reset on slip is off
#define	MCU_RESET_ON_SLIP_OFFSET	31  	// Offset in PLLCTL1

#define	MCU_BYPASS_ON_SLIP	2  				// Bypass on slip is off
#define	MCU_BYPASS_ON_SLIP_OFFSET	29  	// Offset in PLLCTL1 (2 bits)

#define	MCU_RESET_ON_OSC_FAIL	0  			// Reset on oscillator fail is off
#define	MCU_RESET_ON_OSC_FAIL_OFFSET	23  // Offset in PLLCTL1

#define	MCU_PLLDIV_OFFSET	24  			// Offset in PLLCTL1 (5 bits)
#define MCU_PLLDIV_MASK     (0x1F << MCU_PLLDIV_OFFSET)
#define	MCU_REFCLKDIV_OFFSET	16  		// Offset in PLLCTL1 (6 bits)
#define MCU_REFCLKDIV_MASK  (0x3F << MCU_REFCLKDIV_OFFSET)
#define	MCU_PLLMUL_OFFSET	0  				// Offset in PLLCTL1 (16 bits)
#define MCU_PLLMUL_MASK     (0xFFFF << MCU_PLLMUL_OFFSET)

/* PLLCTL2 register */
#define MCU_FM_ENABLE	0  					// Frequency modulation is off
#define MCU_FM_ENABLE_OFFSET	31 			// Offset in PLLCTL2

#define MCU_SPREADING_RATE	0 				// Spreading rate
#define MCU_SPREADING_RATE_OFFSET	22 		// Offset in PLLCTL2 (9 bits)

#define MCU_BWADJ	0 						// Bandwidth adjustment
#define MCU_BWADJ_OFFSET	12 				// Offset in PLLCTL2 (9 bits)

#define MCU_ODPLL_OFFSET	9				// Offset in PLLCTL2 (3 bits)
#define MCU_ODPLL_MASK     (0x7 << MCU_ODPLL_OFFSET)

#define MCU_SPREADING_AMOUNT	0 			// Spreading amount
#define MCU_SPREADING_AMOUT_OFFSET	0 		// Offset in PLLCTL2 (9 bits)


/* CSDIS (Clock source disable) register offsets */
#define MCU_CLK_SOURCE_OSC_OFFSET		0
#define MCU_CLK_SOURCE_FMZPLL_OFFSET	1
#define MCU_CLK_SOURCE_LPO_OFFSET		4
#define MCU_CLK_SOURCE_HPO_OFFSET		5
#define MCU_CLK_SOURCE_FPLL_OFFSET		6


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_x)  (sizeof(_x)/sizeof((_x)[0]))
#endif

/* Development error macros. */
#if ( MCU_DEV_ERROR_DETECT == STD_ON )
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


#define CORE_CPUID_CORTEX_M3   	0x411FC231UL

typedef struct {
	uint32 lossOfLockCnt;
	uint32 lossOfClockCnt;
} Mcu_Stats;






/**
 * Type that holds all global data for Mcu
 */
typedef struct
{
  // Set if Mcu_Init() have been called
  boolean initRun;

  // Our config
  const Mcu_ConfigType *config;

  Mcu_ClockType clockSetting;

  Mcu_Stats stats;

} Mcu_GlobalType;


// Global config
Mcu_GlobalType Mcu_Global =
{
		.initRun = 0,
		.config = &McuConfigData[0],
};

//-------------------------------------------------------------------

typedef struct {
  char *name;
  uint32 pvr;
} core_info_t;

typedef struct {
  char *name;
  uint32 pvr;
} cpu_info_t;


void Mcu_ConfigureFlash(void);



/* Haven't found any ID accessable from memory.
 * There is the DBGMCU_IDCODE (0xe0042000) found in RM0041 but it
 * you can't read from that address..
 */
#if 0
cpu_info_t cpu_info_list[] = {
    {
    .name = "????",
    .pvr = 0,
    },
};
#endif

/* The supported cores
 */
core_info_t core_info_list[] = {
    {
    .name = "CORE_ARM_CORTEX_M3",
    .pvr = CORE_CPUID_CORTEX_M3,
    },
};

/**
 * Identify the core, just to check that we have support for it.
 *
 * @return
 */
static uint32 Mcu_CheckCpu( void ) {


	return 0;
}


/**
  * Initialize Peripherals clocks
  */
static void InitPerClocks( void  )
{

	/* USER CODE BEGIN (9) */
	/* USER CODE END */

	/** - Disable Peripherals before peripheral power-up*/
	systemREG1->CLKCNTL &= 0xFFFFFEFFU;

							 /** - Release peripherals from reset and enable clocks to all peripherals */
	/** - Power-up all peripherals */
	pcrREG->PSPWRDWNCLR0 = 0xFFFFFFFFU;
	pcrREG->PSPWRDWNCLR1 = 0xFFFFFFFFU;
	pcrREG->PSPWRDWNCLR2 = 0xFFFFFFFFU;
	pcrREG->PSPWRDWNCLR3 = 0xFFFFFFFFU;

	/** - Enable Peripherals */
	systemREG1->CLKCNTL |= 0x00000100U;

	/* USER CODE BEGIN (10) */
	/* USER CODE END */

}


static void mapClocks(const Mcu_ClockSettingConfigType *clockSettingsPtr) {

	/** @b Initialize @b Clock @b Tree: */
	/** - Disable / Enable clock domain */

	/* CSDIS - Clock Source Disable Register
	 * CDDIS - Clock Domain Disable Register
	 * */

	systemREG1->CDDIS =   (0<<11)	/*  VCLKA4            */
						| (0<<10)	/*  VCLKA3            */
						| (0<<9)	/*  VCLK4 (etpwm,etc)     */
						| (0<<8)	/*  VCLK3 (ethernet, etc) */	/* Must be on for PWM!!!!  */
						| (0<<7)	/*  RTICLK2	          */
						| (0<<6)	/*  RTICLK1			  */
						| (1<<5)	/*  VCLKA2 (Flexray)  */
						| (0<<4)	/*  VCLKA1 (DCAN)     */
						| (0<<3)	/*  VCLK2 (NHET)   	  */
						| (0<<2)	/*  VCLK_periph   	  */
						| (0<<1)	/*  HCLK and VCLK_sys */
						| (0<<0); 	/*  CPU 			  */

	systemREG1->CSDIS =    (1<<7)	/*  EXTCLKIN2 	    */
					     | (1<<6)	/*  PLL2 (FPLL) 	*/
						 | (0<<5)	/*  High Frequency LPO (Low Power Oscillator) clock */
						 | (0<<4)	/*  Low Frequency LPO (Low Power Oscillator) clock  */
						 | (1<<3)	/*  EXTCLKIN  	*/
						 | (1<<2)	/*  Not Implemented   	*/
						 | (0<<1)	/*  PLL1 (FMzPLL)  		*/
						 | (0<<0); 	/*  Oscillator 			*/

	/* ODPLL */
	systemREG1->PLLCTL2 = (systemREG1->PLLCTL2 & 0xfffff1ffU) | (uint32) (((clockSettingsPtr->ODPLL - 1) << MCU_ODPLL_OFFSET));


	/* Check again, that it's locked */
	while ((systemREG1->CSVSTAT & ((systemREG1->CSDIS ^ 0xFF) & 0xFF)) != ((systemREG1->CSDIS ^ 0xFF) & 0xFF)) ;

	/** - Map device clock domains to desired sources and configure top-level dividers */
	/** - All clock domains are working off the default clock sources until now */
	/** - The below assignments can be easily modified using the HALCoGen GUI */

	/** - Setup GCLK, HCLK and VCLK clock source for normal operation, power down mode and after wakeup */
	systemREG1->GHVSRC = (uint32) (   (uint32) SYS_OSC << 24U)
									| (uint32) ((uint32) SYS_OSC << 16U)
									| (uint32) ((uint32) SYS_PLL << 0U);

	/*
	 * Setup dividers
	 *
	 * We try to run all at HCLK/2 for now. Important clocks are
	 * - VCLK (ADC, SPI, etc)
	 * - VCLK4
	 * - RTICLK
	 *
	 * Known here:
	 * - GCLK =   HCLK (no dividers)
	 * - VCLK     CLKCNTL is divider register
     * ....
	 * - VCLKA4R  VCLKACON1
	 * - VCLKA3R  VCLKACON1
	 *
	 * */

	/**  VCLK1, VCLK2 setup at half HCLK frequency */
	/* Must write VCLK2R first */
	systemREG1->CLKCNTL = (systemREG1->CLKCNTL & 0xF0FFFFFFU) | (uint32) ((uint32) 1U << 24U);  /* divide by 2 */
	/* Then VCLKR */
	systemREG1->CLKCNTL = (systemREG1->CLKCNTL & 0xFFF0FFFFU)| (uint32) ((uint32) 1U << 16U);	/* divide by 2 */

	/* VCLK3 and VCLK4 is HCLK / 2 */
	systemREG2->CLK2CNTRL = ((1<<8) | (1<<0)) ;

	/** - Setup RTICLK1 and RTICLK2 clocks */
	systemREG1->RCLKSRC  = (0U << 8U) // RTICLK1 divider is 1
						   |  SYS_VCLK; // Select VCLK as source for RTICLK1.

	/** - Setup asynchronous peripheral clock sources for AVCLK1 and AVCLK2 */
	systemREG1->VCLKASRC = (SYS_VCLK << 8U)
						 |  SYS_VCLK;


}



/**
  * Initialize Flash, PLL and clocks.
  */
static void InitMcuClocks(const Mcu_ClockSettingConfigType *clockSettingsPtr)
{

	// INTCLK 1.63MHz - 6.53MHz
	// NR (REFCLKDIV) 1 - 64
	// INTCLK = CLKIN / REFCLKDIV;

	// Output CLK 120MHz - 500MHz
	// NF (PLLMUL) 92 - 184
	// OutputCLK = INTCLK * PLLMUL;

	// OD (ODPLL) 1 - 8
	// R (PLLDIV)  1 - 32
	// PLLCLK = OutputCLK / ODPLL / PLLDIV;

	// Algorithm
	// PLLCLK = (CLKIN * PLLMUL) / (REFCLKDIV * ODPLL * PLLDIV);

	/* Disable the PLL1 and PLL2 to set them up */
	systemREG1->CSDISSET = 0x00000002U | 0x00000040U;


    while((systemREG1->CSDIS & 0x42U) != 0x42U) {
    	/* Wait for lock */
    }

   /* Clear Global Status Register */
    systemREG1->GLBSTAT = 0x301U;


	/** - Setup pll control register 1:
	*     - Setup reset on oscillator slip
	*     - Setup bypass on pll slip
	*     - Setup reset on oscillator fail
	*      - Setup Pll output clock divider
	*     - Setup reference clock divider
	*     - Setup Pll multiplier*
	*/
	systemREG1->PLLCTL1 =
		  (MCU_RESET_ON_SLIP << MCU_RESET_ON_SLIP_OFFSET)
		| (MCU_BYPASS_ON_SLIP << MCU_BYPASS_ON_SLIP_OFFSET)
		| (MCU_RESET_ON_OSC_FAIL << MCU_RESET_ON_OSC_FAIL_OFFSET)
		| (((clockSettingsPtr->NF - 1) * 256) << 0)	/* PLL_MULT */
		| ((clockSettingsPtr->NR - 1) << 16)	    /* REF_CLK_DIV is 6 */
		| ((clockSettingsPtr->R - 1) << 24);		/* PLLDIV */

	/* ODPLL */
	systemREG1->PLLCTL2 = (systemREG1->PLLCTL2 & 0xfffff1ffU) | (uint32) (((clockSettingsPtr->ODPLL - 1) << MCU_ODPLL_OFFSET));

//	| ((clockSettingsPtr->Pll3 - 1) << 16)

}

//-------------------------------------------------------------------

void Mcu_Init(const Mcu_ConfigType *configPtr)
{
	VALIDATE( ( NULL != configPtr ), MCU_INIT_SERVICE_ID, MCU_E_PARAM_CONFIG );

#if !defined(USE_SIMULATOR)
	Mcu_CheckCpu();
#endif

	memset(&Mcu_Global.stats,0,sizeof(Mcu_Global.stats));

	Mcu_ConfigureFlash();

	Mcu_Global.config = configPtr;
	Mcu_Global.initRun = 1;
}

//-------------------------------------------------------------------
Std_ReturnType Mcu_InitRamSection(const Mcu_RamSectionType RamSection)
{
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_INITRAMSECTION_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
	VALIDATE_W_RV( ( RamSection <= Mcu_Global.config->McuRamSectors ), MCU_INITRAMSECTION_SERVICE_ID, MCU_E_PARAM_RAMSECTION, E_NOT_OK );

	/* NOT SUPPORTED, reason: no support for external RAM */

	return E_NOT_OK;
}



//-------------------------------------------------------------------

Std_ReturnType Mcu_InitClock(const Mcu_ClockType ClockSetting)
{
	const Mcu_ClockSettingConfigType *clockSettingsPtr;
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_INITCLOCK_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );
	VALIDATE_W_RV( ( ClockSetting < Mcu_Global.config->McuClockSettings ), MCU_INITCLOCK_SERVICE_ID, MCU_E_PARAM_CLOCK, E_NOT_OK );

	Mcu_Global.clockSetting = ClockSetting;
	clockSettingsPtr = &Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting];

	InitMcuClocks(clockSettingsPtr);
	InitPerClocks();

	return E_OK;
}

//-------------------------------------------------------------------

Std_ReturnType Mcu_DistributePllClock(void)
{
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_DISTRIBUTEPLLCLOCK_SERVICE_ID, MCU_E_UNINIT, E_NOT_OK );

	mapClocks(&Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting]);

	return E_OK;
}

//-------------------------------------------------------------------


Mcu_PllStatusType Mcu_GetPllStatus(void) {
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_GETPLLSTATUS_SERVICE_ID, MCU_E_UNINIT, MCU_PLL_STATUS_UNDEFINED );

	if ((systemREG1->CSVSTAT & ((systemREG1->CSDIS ^ 0xFF) & 0xFF)) != ((systemREG1->CSDIS ^ 0xFF) & 0xFF)) {
		return MCU_PLL_UNLOCKED;
	}

	return MCU_PLL_LOCKED;
}

//-------------------------------------------------------------------

/**
 *
 * @return
 */
Mcu_ResetType Mcu_GetResetReason(void) {
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_GETRESETREASON_SERVICE_ID, MCU_E_UNINIT, MCU_RESET_UNDEFINED );
	 Mcu_ResetType reason = MCU_RESET_UNDEFINED;

	if (systemREG1->SYSESR & 0x00008000) {
		reason = MCU_POWER_ON_RESET;
		systemREG1->SYSESR = 0x00008000;
	} else if (systemREG1->SYSESR & 0x00004000) {
		reason = MCU_OSC_FAILURE_RESET;
		systemREG1->SYSESR = 0x00004000;
	} else if (systemREG1->SYSESR & 0x00002000) {
		reason = MCU_WATCHDOG_RESET;
		systemREG1->SYSESR = 0x00002000;
	} else if (systemREG1->SYSESR & 0x00000020) {
		reason = MCU_CPU_RESET;
		systemREG1->SYSESR = 0x00000020;
	} else if (systemREG1->SYSESR & 0x00000010) {
		reason = MCU_SW_RESET;
		systemREG1->SYSESR = 0x00000010;
	} else if (systemREG1->SYSESR & 0x00000008) {
		reason = MCU_EXT_RESET;
		systemREG1->SYSESR = 0x00000008;
	} else if (systemREG1->SYSESR & 0x00000004) {
		reason = MCU_VSW_RESET;
		systemREG1->SYSESR = 0x00000004;
	} else {
		reason = MCU_RESET_UNDEFINED;
	}

/* USER CODE BEGIN (23) */
/* USER CODE END */

	return reason;
}

//-------------------------------------------------------------------

/**
 * Shall read the raw reset value from hardware register if the hardware
 * supports this.
 *
 * @return
 */

Mcu_RawResetType Mcu_GetResetRawValue(void) {
	VALIDATE_W_RV( ( 1 == Mcu_Global.initRun ), MCU_GETRESETREASON_SERVICE_ID, MCU_E_UNINIT, MCU_GETRESETRAWVALUE_UNINIT_RV );

	Mcu_RawResetType reason = 0xFFFFFFFF;
	reason = systemREG1->SYSESR & 0x0000E03B;
	systemREG1->SYSESR = 0x0000E03B;

	return reason;
}

//-------------------------------------------------------------------

#if ( MCU_PERFORM_RESET_API == STD_ON )
/**
 * Shell perform a microcontroller reset by using the hardware feature
 * of the micro controller.
 */
void Mcu_PerformReset(void)
{
	VALIDATE( ( 1 == Mcu_Global.initRun ), MCU_PERFORMRESET_SERVICE_ID, MCU_E_UNINIT );
	systemREG1->SYSECR = 0x00008000;
}
#endif

//-------------------------------------------------------------------

void Mcu_SetMode(const Mcu_ModeType McuMode)
{
	VALIDATE( ( 1 == Mcu_Global.initRun ), MCU_SETMODE_SERVICE_ID, MCU_E_UNINIT );
  /* NOT SUPPORTED */
}

//-------------------------------------------------------------------

/**
 * Get the system clock in Hz. It calculates the clock from the
 * different register settings in HW.
 */
uint32 Mcu_Arc_GetSystemClock(void)
{
  uint32_t f_sys;

  // PLLCLK = (CLKIN * PLLMUL) / (REFCLKDIV * ODPLL * PLLDIV);

  uint32 odpll = ((systemREG1->PLLCTL2 & MCU_ODPLL_MASK) >> MCU_ODPLL_OFFSET) + 1;
  uint32 plldiv = ((systemREG1->PLLCTL1 & MCU_PLLDIV_MASK) >> MCU_PLLDIV_OFFSET) + 1;
  uint32 refclkdiv = ((systemREG1->PLLCTL1 & MCU_REFCLKDIV_MASK) >> MCU_REFCLKDIV_OFFSET) + 1;
  uint32 pllmult = (((systemREG1->PLLCTL1 & MCU_PLLMUL_MASK) >> MCU_PLLMUL_OFFSET) / 256) + 1;

  f_sys = Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting].McuClockReferencePointFrequency;
  f_sys = f_sys * pllmult / (refclkdiv * odpll * plldiv);

  return f_sys;
}

/**
 * Get the peripheral clock in Hz for a specific device
 */
uint32 Mcu_Arc_GetPeripheralClock(Mcu_Arc_PeriperalClock_t type)
{
	if (type == PERIPHERAL_CLOCK_CAN || type == PERIPHERAL_CLOCK_WDG) {
		uint8 vclockDiv = ((systemREG1->CLKCNTL >> 16) & 0xf);  // VCLKR;
		return Mcu_Arc_GetSystemClock() / (vclockDiv + 1);
	} else if (type == PERIPHERAL_CLOCK_PWM) {
		uint32_t vclockDiv = ((systemREG2->CLK2CNTRL >> 8) & 0xF); //VCLK4R;
		return Mcu_Arc_GetSystemClock() / (vclockDiv + 1);
	}
	return 0;
}

/**
 * Get frequency of the oscillator
 */
uint32 Mcu_Arc_GetClockReferencePointFrequency()
{
	return Mcu_Global.config->McuClockSettingConfig[Mcu_Global.clockSetting].McuClockReferencePointFrequency;
}

/**
 * Function to setup the internal flash for optimal performance
 */
void Mcu_ConfigureFlash(void)
{
	/* Enable pipeline reads from flash
	 * ZWT:
	 *   - 180Mhz, Flash
	 * PGE:
	 *   - 160Mhz, Flash
	 * */

	flashREG->FRDCNTL = (3<<8) +	/* Read wait states    */
						(1<<4) + 	/* Address wait states */
						(1<<0);		/* Enable pipeline mode */
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

/********************************************************************
 * Mcu_Arc_InitZero
 *
 * Function that perform all necessary initialization needed to to run software, such as disabling watchdog,
 * init ECC RAM, setup exception vectors etc
 ********************************************************************/


void Mcu_Arc_InitZero(void) {
    /********************************************************************************
     * NOTE
     *
     * This function will be called before BSS and DATA are initialized.
     * Ensure that you do not access any global or static variables before
     * BSS and DATA is initialized
     ********************************************************************************/

#if defined(USE_KERNEL)
    Irq_Init();
#endif
}

