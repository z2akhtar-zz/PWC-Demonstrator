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


#include "Wdg.h"
#include "WdgIf.h"
#include "Std_Types.h"
#include "isr.h"
#include "core_cr4.h"
#include "irq_types.h"

#if defined(USE_DET)
#include "Det.h"
#endif

#if defined(WDG_DEV_ERROR_DETECT)
typedef enum {
	WDG_IDLE, WDG_UNINIT, WDG_BUSY
} Wdg_ModuleStateType;
#endif

#if (WDG_DEV_ERROR_DETECT)
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(WDG_MODULE_ID,0,_api,_err); \
          return; \
        }
#define VALIDATE_RV(_exp,_api,_err,_ret ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(WDG_MODULE_ID,0,_api,_err); \
          return _ret; \
        }
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_RV(_exp,_api,_err,_ret )
#endif

Wdg_ModuleStateType moduleState = WDG_UNINIT;

const Wdg_ConfigType* WdgConfigPtr;

static uint32_t ticksPerPeriod;
static uint32_t triggerCounter;
static uint16_t interruptsSinceCondition;

void* RTIDWDCTRL = (void*)0xFFFFFC90;
void* RTIDWDPRLD = (void*)0xFFFFFC94;
void* RTIDWDKEY  = (void*)0xFFFFFC9C;
void *RTIWWDSIZE = (void*)0xFFFFFCA8;

#define WDG_ENABLE_MAGIC (0xA98559DA)
#define WDG_KEY_ONE (0xE51A)
#define WDG_KEY_TWO (0xA35C)
#define WDG_INTERRUPT_PRIORITY 2

#define WDG_WINDOW_100_SETTING (0x5)
#define WDG_WINDOW_50_SETTING (0x50)
#define WDG_WINDOW_25_SETTING (0x500)
#define WDG_WINDOW_12_5_SETTING (0x5000)
#define WDG_WINDOW_6_25_SETTING (0x50000)
/*Any value other than the previous five sets window to 3.125%
Therefore the next define's value is arbitrary
Ref: 13.3.34 in the TMS570LS1227 manual*/
#define WDG_WINDOW_3_125_SETTING (0x500000)

static void Internal_SetMode(WdgIf_ModeType Mode);

ISR(Wdg_Isr);

void Wdg_Init(const Wdg_ConfigType* ConfigPtr) {
	WdgConfigPtr = ConfigPtr;

	/* @req SWS_Wdg_00089 */
	VALIDATE((ConfigPtr != NULL), WDG_INIT_SERVICE_ID, WDG_E_PARAM_POINTER);

	if (ConfigPtr->Wdg_ModeConfig->Wdg_DefaultMode == WDGIF_OFF_MODE) {
		if (WDG_DISABLE_ALLOWED == STD_OFF) {
			/* Report DEM error */
			return;
		}
	}

	uint32_t rtiClock = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_WDG);

	/* @req SWS_Wdg_00090*/
	/* Check config against hardware constraints:
	 * The longest WDG time interval that can be supported is 4096 * 8192 / RTICLCK
	 * The shortest interval is 8192 / RTICLK
	 * At 90 MHz RTICLK, the range corresponds to 3 - 10986 Hz
	 * See Internal_SetMode for formula
	 * We don't need to do the whole calculation with those factors here, it's
	 * a bit faster to just calculate with reference to the known values at 90 MHz
	 * because the formula is linear anyway
	 */
	uint32_t minFreq = (uint32_t)(3 * (double)(rtiClock / 90000000));
	uint32_t maxFreq = (uint32_t)(10986 * (double)(rtiClock / 90000000));
	VALIDATE((ConfigPtr->Wdg_ModeConfig->WdgSettingsFast->period > minFreq), WDG_INIT_SERVICE_ID, WDG_E_PARAM_CONFIG);
	VALIDATE((ConfigPtr->Wdg_ModeConfig->WdgSettingsFast->period < maxFreq), WDG_INIT_SERVICE_ID, WDG_E_PARAM_CONFIG);


	ticksPerPeriod = rtiClock / (1000 * ConfigPtr->Wdg_ModeConfig->ArcWdgInterruptPeriod);

	/** - Setup compare 1 value. This value is compared with selected free running counter. */
	rtiREG1->CMP[1U].COMPx = ticksPerPeriod;

	/** - Setup update compare 1 value. This value is added to the compare 1 value on each compare match. */
	rtiREG1->CMP[1U].UDCPx = ticksPerPeriod;

	triggerCounter = 0;

	rtiREG1->SETINT |= (1 << 1); /*enable compare 1 interrupt*/

	/*Set the mode before starting the Watchdog device
	 * The hardware requires that this happens before the
	 * actual Watchdog module is started*/

	Internal_SetMode(ConfigPtr->Wdg_ModeConfig->Wdg_DefaultMode);

	ISR_INSTALL_ISR2("WdgIsr", Wdg_Isr, RTI_COMPARE_1, WDG_INTERRUPT_PRIORITY, 0);

	/* Start the actual watchdog hardware */
	*(uint32_t*)RTIDWDCTRL = WDG_ENABLE_MAGIC;


	/* @req SWS_Wdg_00019 */
	#if defined(WDG_DEV_ERROR_DETECT)
	moduleState = WDG_IDLE;
	#endif
}

Std_ReturnType Wdg_SetMode(WdgIf_ModeType Mode) {

	VALIDATE_RV((Mode == WDGIF_FAST_MODE || Mode == WDGIF_OFF_MODE), WDG_SET_MODE_SERVICE_ID, WDG_E_PARAM_MODE, E_NOT_OK);
	VALIDATE_RV(moduleState == WDG_IDLE, WDG_SET_MODE_SERVICE_ID, WDG_E_DRIVER_STATE, E_NOT_OK);

	/* Disabling the watchdog is not allowed */
	if (Mode == WDGIF_OFF_MODE) {
		/*Report DEM error*/
		return E_NOT_OK;
	}

	Internal_SetMode(Mode);

	return E_OK;
}

static void Internal_SetMode(WdgIf_ModeType Mode)
{

#if defined(WDG_DEV_ERROR_DETECT)
	moduleState = WDG_BUSY;
#endif


	Wdg_SettingsType modeSettings;
	if (Mode == WDGIF_FAST_MODE) {
		modeSettings = *(WdgConfigPtr->Wdg_ModeConfig->WdgSettingsFast);
	}

	double seconds = (1.0 / modeSettings.period);
	uint32_t rticlk = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_WDG); /* the RTICLK1 clock */

	/*
	 * According to TMS570LS1227 manual, section 13.2.5.1
	 * Watchdog counter expiration time T is:
	 *
	 * 						13
	 * T = (DWDPRLD + 1) * 2   / RTICLK
	 *
	 * DWDPRLD is the value in range 0..4095 that is stored in a control register
	 *
	 * From that we calculate:
	 *
	 * DWDPRLD = (T*RTICLK / 2^13) - 1
	 */

	int dwdprld = (seconds * rticlk / 8192) - 1;
	*(uint32_t*)RTIDWDPRLD = (dwdprld << 11);

	/*Set the window size. Acts as timeout watchdog if window size == 100%*/
	int setting = WDG_WINDOW_100_SETTING;
	switch (modeSettings.windowSize)
	{
	case WDG_WINDOW_100:
		break;
	case WDG_WINDOW_50:
		setting = WDG_WINDOW_50_SETTING;
		break;
	case WDG_WINDOW_25:
		setting = WDG_WINDOW_25_SETTING;
		break;
	case WDG_WINDOW_12_5:
		setting = WDG_WINDOW_12_5_SETTING;
		break;
	case WDG_WINDOW_6_25:
		setting = WDG_WINDOW_6_25_SETTING;
		break;
	case WDG_WINDOW_3_125:
		setting = WDG_WINDOW_3_125_SETTING;
		break;
	}
	*(uint32_t*)RTIWWDSIZE = setting;

#if defined(WDG_DEV_ERROR_DETECT)
	moduleState = WDG_IDLE;
#endif
}

void Wdg_SetTriggerCondition(uint16 timeout) {
	VALIDATE((moduleState == WDG_IDLE), WDG_SET_TRIGGERING_CONDITION_SERVICE_ID, WDG_E_DRIVER_STATE);
#if (WDG_DEV_ERROR_DETECT == STD_ON)
	moduleState = WDG_BUSY;
#endif
	/* @req SWS_Wdg_00136
	 * @req SWS_Wdg_00138
	 * @req SWS_Wdg_00140
	 * !req SWS_Wdg_00146
	 */
	triggerCounter = timeout;
	if (timeout > 0) {
		triggerCounter--;
	}
	interruptsSinceCondition = 0;
#if (WDG_DEV_ERROR_DETECT == STD_ON)
	moduleState = WDG_IDLE;
#endif
}

#if (STD_ON == WDG_VERSION_INFO_API)
/* @req SWS_Wdg_00109 */
void Wdg_GetVersionInfo( Std_VersionInfoType* versioninfo )
{
    /* @req SWS_Wdg_00174 */
    VALIDATE((NULL != versioninfo), WDG_GET_VERSION_INFO_SERVICE_ID,WDG_E_PARAM_POINTER);
    versioninfo->vendorID = WDG_VENDOR_ID;
    versioninfo->moduleID = WDG_MODULE_ID;
    versioninfo->sw_major_version = WDG_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = WDG_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = WDG_SW_PATCH_VERSION;
}

#endif


ISR(Wdg_Isr) {
#if (WDG_DEV_ERROR_DETECT == STD_ON)
	if (moduleState != WDG_IDLE) {
		Det_ReportError(WDG_MODULE_ID, 0, WDG_INIT_SERVICE_ID, WDG_E_DRIVER_STATE);
	}
	moduleState = WDG_BUSY;
#endif
	interruptsSinceCondition++;
	if (interruptsSinceCondition * WdgConfigPtr->Wdg_ModeConfig->ArcWdgInterruptPeriod <= triggerCounter) {
		/*The watchdog is serviced by two separate write operations
		with constant magic byte sequences*/
		*(uint32_t*)RTIDWDKEY = WDG_KEY_ONE;
		*(uint32_t*)RTIDWDKEY = WDG_KEY_TWO;
	}
#if (WDG_DEV_ERROR_DETECT == STD_ON)
	moduleState = WDG_IDLE;
#endif
}
