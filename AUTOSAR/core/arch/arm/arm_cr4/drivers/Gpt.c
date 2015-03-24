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

/* ----------------------------[includes]------------------------------------*/

/* @req SWS_Gpt_00293 */
/* @req SWS_Gpt_00375 */

#include "Std_Types.h"
#include "Gpt.h"
#include "Cpu.h"
#include <assert.h>
#include <string.h>
#include "core_cr4.h"
#include "Mcu.h"
#include "debug.h"
#include "Gpt_ConfigTypes.h"

#if defined(USE_DET) && (GPT_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif
#include "Os.h"
#include "isr.h"

#define GPT_CHANNELS_CNT       2
#define GPT_CHANNEL_LAST    (GPT_CHANNELS_CNT-1)

#if ( GPT_DEV_ERROR_DETECT == STD_ON )
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          Det_ReportError(GPT_MODULE_ID,0,_api,_err); \
          return; \
        }

#define VALIDATE_W_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          Det_ReportError(GPT_MODULE_ID,0,_api,_err); \
          return (_rv); \
        }

#define VALID_CHANNEL(_ch)		( Gpt_Global.configured & (1<<(_ch)) )

#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_W_RV(_exp,_api,_err,_rv )
#endif

#define COMP2_INT (4)
#define COMP3_INT (5)

#define ENABLE_COMPARE_INTERRUPT(Channel) \
	if (Channel == GPT_CHANNEL_RTI_0) { \
		rtiREG1->SETINT |= (1 << 2); \
	} else if (Channel == GPT_CHANNEL_RTI_1) { \
		rtiREG1->SETINT |= (1 << 3); \
	}

#define CLEAR_PENDING_INTERRUPT(Channel) \
	if (Channel == GPT_CHANNEL_RTI_0) { \
		rtiREG1->INTFLAG |= (1 << 2); \
	} else if (Channel == GPT_CHANNEL_RTI_1) { \
		rtiREG1->INTFLAG |= (1 << 3); \
	}

#define START_TIMER (rtiREG1->GCTRL |= (1 << 1)); \
					rtiREG1->CNT[1].UCx = rtiREG1->CNT[1].UCx;

typedef enum {
	GPT_STATE_INITIALIZED = 0,
	GPT_STATE_STARTED,
	GPT_STATE_EXPIRED,
	GPT_STATE_STOPPED
} Gpt_StateType;

/**
 * Type that holds all global data for Gpt
 */
typedef struct {
	// Set if Gpt_Init() have been called
	boolean initRun;

	// Our config
	const Gpt_ConfigType *config;

#if ( GPT_WAKEUP_FUNCTIONALITY_API == STD_ON )
	uint8 wakeupEnabled;
#endif

	// One bit for each channel that is configured.
	// Used to determine if validity of a channel
	// 1 - configured
	// 0 - NOT configured
	uint8 configured;

	// Maps the a channel id to a configured channel id
	uint8 channelMap[GPT_CHANNEL_CNT];

	//One bit for each channel's state
	//0 - stopped
	//1 - started
	//See Gpt_StateType
	Gpt_StateType channelState[GPT_CHANNEL_CNT];

	//We need to remember some stuff for each channel
	//At index n * 3, we have FRC register value for channel n
	//At index n * 3 + 1, we have UC register value for channel n
	//At index n * 3 + 2, we have the configured target time for channel n
	//They are accessed through the MEM_ macros, and set depending on
	//channel state, such as when starting or stopping the channel
	uint32_t channelMemory[GPT_CHANNEL_CNT * 2];

} Gpt_GlobalType;

#define MEM_FRC(_ch) 	(_ch * 2)
#define MEM_TARGET(_ch)  	(_ch * 2 + 1)

Gpt_GlobalType Gpt_Global;

static void Gpt_Isr(uint32_t channel);
ISR(Gpt_Isr0);
ISR(Gpt_Isr1);

static void WritePeriod(Gpt_ChannelType Channel, uint32_t ticks, uint32_t gptTicksPerMs, uint32_t rtiTicksPerMs);
#if GPT_TIME_REMAINING_API == STD_ON || GPT_TIME_ELAPSED_API == STD_ON
static uint32_t Internal_GetElapsedTicks(Gpt_ChannelType Channel);
#endif

void Gpt_Init(const Gpt_ConfigType* ConfigPtr) {
	uint32_t i = 0;
	const Gpt_ConfigType *cfg;
	/* @req SWS_Gpt_00307 */
	VALIDATE( (Gpt_Global.initRun == STD_OFF), GPT_INIT_SERVICE_ID, GPT_E_ALREADY_INITIALIZED);
	/* @req SWS_Gpt_00294 */
	VALIDATE( (ConfigPtr != NULL ), GPT_INIT_SERVICE_ID, GPT_E_PARAM_POINTER);

	for (i = 0; i < GPT_CHANNEL_CNT; i++) {
		Gpt_Global.channelMap[i] = GPT_CHANNEL_ILL;
		Gpt_Global.channelState[i] = GPT_STATE_INITIALIZED;
	}

	Gpt_ChannelType ch;
	i = 0;
	cfg = ConfigPtr;
	while (cfg->GptChannelId != GPT_CHANNEL_ILL) {
		ch = cfg->GptChannelId;

		// Assign the configuration channel used later..
		Gpt_Global.channelMap[cfg->GptChannelId] = i;
		Gpt_Global.configured |= (1 << ch);

		if (ch <= GPT_CHANNEL_LAST) {
			if (cfg->GptNotification != NULL) {
				switch (ch) {
				case 0:
					ISR_INSTALL_ISR2("Gpt_0", Gpt_Isr0, COMP2_INT, 2, 0)
					;
					break;
				case 1:
					ISR_INSTALL_ISR2("Gpt_1", Gpt_Isr1, COMP3_INT, 2, 0)
					;
					break;
				default:
					// Unknown channel.
					assert(0)
					;
					break;
				}
			}
		}
		cfg++;
		i++;
	}

	Gpt_Global.config = ConfigPtr;

	Gpt_Global.initRun = STD_ON;

	//Disable the hardware interrupts for compare registers 2 and 3
	rtiREG1->CLEARINT |= (1 << 2);
	rtiREG1->CLEARINT |= (1 << 3);

	//GPT will use the hardware Counter 1, we preset it to zero.
	//Counter must be stopped and then re-started to preset
	rtiREG1->GCTRL &= ~(1 << 1);
	rtiREG1->CNT[1].FRCx = 0U;
	rtiREG1->GCTRL |= (1 << 1);
	/* Configure the up-counter compare to correspond to a fixed number
	 * So that the FRC will increment every millisecond	 */
	uint32_t rticlk = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_RTICLK);
	uint32_t ticksPerMs = rticlk / 1000;
	rtiREG1->CNT[1].CPUCx = ticksPerMs;

	START_TIMER;
}

//@req SWS_Gpt_00194
#if GPT_DEINIT_API == STD_ON
void Gpt_DeInit(void) {
	int anyRunning = 0;
	/* @req SWS_Gpt_00220 */
	VALIDATE( (Gpt_Global.initRun == STD_ON), GPT_DEINIT_SERVICE_ID, GPT_E_UNINIT);
	for (int i = 0; i < GPT_CHANNEL_CNT; i++) {
		if (Gpt_Global.channelState[i] == GPT_STATE_STARTED) {
			anyRunning = 1;
			break;
		}
	}
	/* @req SWS_Gpt_00234 */
	VALIDATE( (anyRunning == 0), GPT_DEINIT_SERVICE_ID, GPT_E_BUSY);
	/* @req SWS_Gpt_00105 */
	rtiREG1->CLEARINT |= (1 << 2);
	rtiREG1->CLEARINT |= (1 << 3);

	/* Do not do anything here with the actual hardware counters, as they're shared
	 * by the RTI module, are responsibility of the MCU, and may be needed elsewhere
	 */

	/* @req SWS_Gpt_00234 */
	Gpt_Global.configured = 0;
	Gpt_Global.initRun = STD_OFF;
}
#endif

//@req SWS_Gpt_00195
#if GPT_TIME_ELAPSED_API == STD_ON
Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel) {

	/* @req SWS_Gpt_00222
	 * @req SWS_Gpt_00210
	 */
	VALIDATE_W_RV( (Gpt_Global.initRun == STD_ON), GPT_GETTIMEELAPSED_SERVICE_ID, GPT_E_UNINIT, 0 );
	VALIDATE_W_RV( VALID_CHANNEL(Channel), GPT_GETTIMEELAPSED_SERVICE_ID, GPT_E_PARAM_CHANNEL, 0 );

	return Internal_GetElapsedTicks(Channel);
}
#endif

//@req SWS_Gpt_00196
#if GPT_TIME_REMAINING_API == STD_ON
Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel) {
	/* @req SWS_Gpt_00223
	 * @req SWS_Gpt_00211
	 */
	VALIDATE_W_RV( (Gpt_Global.initRun == STD_ON), GPT_GETTIMEREMAINING_SERVICE_ID, GPT_E_UNINIT, 0 );
	VALIDATE_W_RV( VALID_CHANNEL(Channel), GPT_GETTIMEREMAINING_SERVICE_ID, GPT_E_PARAM_CHANNEL, 0 );

	uint8 confCh = Gpt_Global.channelMap[Channel];
	uint32_t rticlk = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_RTICLK);
	uint32_t rtiTicksPerGptTick = rticlk / Gpt_Global.config[confCh].GptTickFrequency;

	if (Gpt_Global.channelState[Channel] == GPT_STATE_INITIALIZED) {
		/* @req SWS_Gpt_00301 */
		return 0;
	} else if (Gpt_Global.channelState[Channel] == GPT_STATE_EXPIRED) {
		/* @req SWS_Gpt_00305 */
		return 0;
	} else if (Gpt_Global.channelState[Channel] == GPT_STATE_STOPPED ||
			Gpt_Global.channelState[Channel] == GPT_STATE_STARTED) {
		/* @req SWS_Gpt_00303 */
		/* @req SWS_Gpt_00083 */
		/* Remaining time is the configured time minus elapsed time. For
		 * stopped channels, that becomes configured time minus elapsed time
		 * at the moment of stopping the channel.
		 */
		uint32_t target = Gpt_Global.channelMemory[MEM_TARGET(confCh)];
		/*If target time is less than current counter value, there was
		  an overflow, so handle it */
		uint32_t currentFrc = rtiREG1->CNT[1].FRCx;
		if (target < currentFrc && currentFrc - target > 1000) {
			target = 0xFFFFFFFF - currentFrc + target;
		}
		//target /= rtiTicksPerGptTick;
		return target - Internal_GetElapsedTicks(Channel);
	}
	return 0;
}
#endif

void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value) {
	uint8 confCh;

	/* @req SWS_Gpt_00212 */
	/* @req SWS_Gpt_00218 */
	/* @req SWS_Gpt_00224 */
	VALIDATE( (Gpt_Global.initRun == STD_ON), GPT_STARTTIMER_SERVICE_ID, GPT_E_UNINIT ); VALIDATE( VALID_CHANNEL(Channel), GPT_STARTTIMER_SERVICE_ID, GPT_E_PARAM_CHANNEL ); VALIDATE( (Gpt_Global.channelState[Channel] != GPT_STATE_STARTED), GPT_STARTTIMER_SERVICE_ID, GPT_E_BUSY ); VALIDATE ((Value != 0), GPT_STARTTIMER_SERVICE_ID, GPT_E_PARAM_VALUE );

	confCh = Gpt_Global.channelMap[Channel];

	if (Channel <= GPT_CHANNEL_LAST) {
		//There should be an interrupt after Value ticks of this Gpt channel
		//Calculate how many ticks that is of the board's RTICLK
		uint32_t rticlk = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_RTICLK);
		uint32_t ticks = rticlk / Gpt_Global.config[confCh].GptTickFrequency;
		uint32_t rticlkPerMs = rticlk / 1000;
		uint32_t gptTicksPerMs = rticlkPerMs / ticks;
		ticks *= Value;
		CLEAR_PENDING_INTERRUPT(Channel);
		WritePeriod(Channel, ticks, gptTicksPerMs, rticlkPerMs);
	}

	if (Gpt_Global.config[confCh].GptNotification != NULL) {
		/* @req SWS_Gpt_00275 */
		Gpt_EnableNotification(Channel);
	}

	uint32_t currentFrc = rtiREG1->CNT[1].FRCx;
	Gpt_Global.channelMemory[MEM_FRC(confCh)] = currentFrc;

	Gpt_Global.channelState[Channel] = GPT_STATE_STARTED;
}

void Gpt_StopTimer(Gpt_ChannelType Channel) {
	/* @req SWS_Gpt_00213
	 * @req SWS_Gpt_00225
	 */
	VALIDATE( (Gpt_Global.initRun == STD_ON), GPT_STOPTIMER_SERVICE_ID, GPT_E_UNINIT ); VALIDATE( VALID_CHANNEL(Channel), GPT_STOPTIMER_SERVICE_ID, GPT_E_PARAM_CHANNEL );

	// @req SWS_Gpt_00344
	if (Gpt_Global.channelState[Channel] == GPT_STATE_STOPPED
			|| Gpt_Global.channelState[Channel] == GPT_STATE_EXPIRED
			|| Gpt_Global.channelState[Channel] == GPT_STATE_INITIALIZED) {
		return;
	}
	Gpt_DisableNotification(Channel);
	uint8 confCh = Gpt_Global.channelMap[Channel];
	/* Store counter values at the time of stopping so that they
	 * may be used when Gpt_GetTimeElapsed() gets called for a
	 * stopped channel */
	uint32_t currentFrc = rtiREG1->CNT[1].FRCx;
	uint32_t startFrc = Gpt_Global.channelMemory[MEM_FRC(confCh)];
	Gpt_Global.channelMemory[MEM_FRC(confCh)] = currentFrc - startFrc;
	// @req SWS_Gpt_00343
	Gpt_Global.channelState[Channel] = GPT_STATE_STOPPED;
}

#if GPT_TIME_REMAINING_API == STD_ON || GPT_TIME_ELAPSED_API == STD_ON
static uint32_t Internal_GetElapsedTicks(Gpt_ChannelType Channel) {
	uint8 confCh = Gpt_Global.channelMap[Channel];
	uint32_t rticlk = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_RTICLK);
	uint32_t rtiTicksPerMs = rticlk	/ 1000;
	uint32_t rtiTicksPerGptTick = rticlk / Gpt_Global.config[confCh].GptTickFrequency;
	if (Gpt_Global.channelState[Channel] == GPT_STATE_INITIALIZED) {
		/* @req SWS_Gpt_00295 */
		return 0;
	} else if (Gpt_Global.channelState[Channel] == GPT_STATE_STOPPED) {
		/* @req SWS_Gpt_00297 */
		/* For a stopped channel, the memory contains the FRC
		 * stored by Gpt_StopTimer(), which is FRC_stop - FRC_start.
		 * It also contains the period that was set for the channel,
		 * in ticks of RTICLK, which is also the value.
		 */
		uint32_t frc = Gpt_Global.channelMemory[MEM_FRC(confCh)];
		/* FRC increments once per millisecond */
		uint64_t rtiTicksSinceStop = (frc * rtiTicksPerMs);
		return rtiTicksSinceStop / rtiTicksPerGptTick;
	} else if (Gpt_Global.channelState[Channel] == GPT_STATE_EXPIRED) {
		/* @req SWS_Gpt_00299 */
		/* This is an expired one-shot timer, meaning it has reached the
		 * value it was configured for, so just return that
		 */
		//uint32_t periodInRticlk = Gpt_Global.channelMemory[MEM_TARGET(confCh)];
		uint32_t period = Gpt_Global.channelMemory[MEM_TARGET(confCh)];
		return period;
		//return periodInRticlk / rtiTicksPerGptTick;
	} else if (Gpt_Global.channelState[Channel] == GPT_STATE_STARTED) {
		/* For a started channel, the memory contains FRC and UC values at
		 * the time of starting, so we need to read those, read the current
		 * values and then calculate elapsed ticks
		 */
		uint32_t startFrc = Gpt_Global.channelMemory[MEM_FRC(confCh)];
		uint32_t currentFrc = rtiREG1->CNT[1].FRCx;
		uint32_t frc = currentFrc - startFrc;
		uint64_t ticksSinceStart = (frc * rtiTicksPerMs);
		return ticksSinceStart / rtiTicksPerGptTick;
	}
	return 0;
}
#endif

// @req SWS_Gpt_00199
#if (GPT_ENABLE_DISABLE_NOTIFICATION_API == STD_ON)
void Gpt_EnableNotification(Gpt_ChannelType Channel) {
	uint8 confCh = Gpt_Global.channelMap[Channel];
	// @req SWS_Gpt_00226
	// @req SWS_Gpt_00214
	// @req SWS_Gpt_00377
	VALIDATE( (Gpt_Global.initRun == STD_ON), GPT_ENABLENOTIFICATION_SERVICE_ID, GPT_E_UNINIT );
	VALIDATE( VALID_CHANNEL(Channel), GPT_ENABLENOTIFICATION_SERVICE_ID, GPT_E_PARAM_CHANNEL );
	VALIDATE( Gpt_Global.config[confCh].GptNotification != NULL, GPT_ENABLENOTIFICATION_SERVICE_ID, GPT_E_PARAM_CHANNEL );
	if (Channel == GPT_CHANNEL_RTI_0) {
		rtiREG1->COMPCTRL |= (1 << 8); //COMPSEL2
	} else if (Channel == GPT_CHANNEL_RTI_1) {
		rtiREG1->COMPCTRL |= (1 << 12); //COMPSEL3
	}
	ENABLE_COMPARE_INTERRUPT(Channel);
}

void Gpt_DisableNotification(Gpt_ChannelType Channel) {
	uint8 confCh = Gpt_Global.channelMap[Channel];
	VALIDATE( (Gpt_Global.initRun == STD_ON), GPT_DISABLENOTIFICATION_SERVICE_ID, GPT_E_UNINIT );
	VALIDATE( VALID_CHANNEL(Channel), GPT_DISABLENOTIFICATION_SERVICE_ID, GPT_E_PARAM_CHANNEL );
	VALIDATE ((Gpt_Global.config[confCh].GptNotification != NULL), GPT_DISABLENOTIFICATION_SERVICE_ID, GPT_E_PARAM_CHANNEL);

	if (Channel == GPT_CHANNEL_RTI_0) {
		rtiREG1->CLEARINT |= (1 << 2);
	} else if (Channel == GPT_CHANNEL_RTI_1) {
		rtiREG1->CLEARINT |= (1 << 3);
	}
}
#endif

#if GPT_WAKEUP_FUNCTIONALITY_API == STD_ON
void Gpt_SetMode(Gpt_ModeType Mode) {

}

void Gpt_DisableWakeup(Gpt_ChannelType Channel) {

}

void Gpt_EnableWakeup(Gpt_ChannelType Channel) {

}

void Gpt_CheckWakeup(EcuM_WakeupSourceType WakeupSource) {

}
#endif

#if STD_OFF //No support for predef timers
Std_ReturnType Gpt_GetPredefTimerValue(Gpt_PredefTimerType PredefTimer, uint32* TimeValuePtr) {

}
#endif

static void Gpt_Isr(uint32_t channel) {
	uint8 confCh = Gpt_Global.channelMap[channel];
	if (Gpt_Global.config[confCh].GptChannelMode == GPT_CH_MODE_ONESHOT) {
		/*In one-shot mode, we need to expire and stop the timer
		 We need to keep the actual hw timer running, since it is shared,
		 so we disable the comparison of counters */
		if (channel == GPT_CHANNEL_RTI_0) {
			rtiREG1->CLEARINT |= (1 << 2);
		} else if (channel == GPT_CHANNEL_RTI_1) {
			rtiREG1->CLEARINT |= (1 << 3);
		}
		Gpt_Global.channelState[channel] = GPT_STATE_EXPIRED;
	}
	if (Gpt_Global.config[confCh].GptNotification != NULL) {
		Gpt_Global.config[confCh].GptNotification();
	}
}

ISR(Gpt_Isr0) {
	rtiREG1->INTFLAG |= (1 << 2);
	Gpt_Isr(GPT_CHANNEL_RTI_0);
}

ISR (Gpt_Isr1) {
	rtiREG1->INTFLAG |= (1 << 3);
	Gpt_Isr(GPT_CHANNEL_RTI_1);
}

static void WritePeriod(Gpt_ChannelType Channel, uint32_t ticks, uint32_t gptTicksPerMs, uint32_t rtiTicksPerMs) {
	uint8 confCh = Gpt_Global.channelMap[Channel];
	//I know in how many ticks I need the interrupt
	//I need to convert that to ms, and then correspondingly write
	//the compare value of FRC
	uint32_t gptTicks = ticks / rtiTicksPerMs;
	uint32_t ms = gptTicks / gptTicksPerMs;
	uint32_t current = rtiREG1->CNT[1].FRCx;
	if (Channel == GPT_CHANNEL_RTI_0) {
		rtiREG1->CMP[2].COMPx = current + ms;
		rtiREG1->CMP[2].UDCPx = 1;
	} else if (Channel == GPT_CHANNEL_RTI_1) {
		rtiREG1->CMP[3].COMPx = current + ms;
		rtiREG1->CMP[3].UDCPx = 1;
	}
	Gpt_Global.channelMemory[MEM_TARGET(confCh)] = gptTicks;
}
