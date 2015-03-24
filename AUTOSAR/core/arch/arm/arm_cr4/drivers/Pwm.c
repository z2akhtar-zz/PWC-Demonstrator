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
#if defined(USE_DET)
#include "Det.h"
#endif
#include "Mcu.h"
#include "Os.h"
#include "Pwm.h"
#include "epwm.h"
#if PWM_NOTIFICATION_SUPPORTED==STD_ON
#include "isr.h"
#include "irq.h"
#include "arc.h"
#endif

#if defined(USE_DET)
#	define Pwm_Det_ReportError(ApiId, ErrorId) Det_ReportError( PWM_MODULE_ID, 0, ApiId, ErrorId);
#else
#   define Pwm_Det_ReportError(ApiId, ErrorId)
#endif

#if PWM_DEV_ERROR_DETECT==STD_ON
	#define PWM_VALIDATE(_exp, _apiid, _errid) \
		if (!(_exp)) { \
			Pwm_Det_ReportError( _apiid, _errid); \
			return; \
		}
	#define Pwm_VALIDATE_CHANNEL(_apiid, _ch) PWM_VALIDATE( (configuredChannels & (1 << _ch) ), _apiid, PWM_E_PARAM_CHANNEL)
	#define Pwm_VALIDATE_INITIALIZED(_apiid) PWM_VALIDATE( (Pwm_ModuleState == PWM_STATE_INITIALIZED), _apiid, PWM_E_UNINIT)
	#define Pwm_VALIDATE_UNINITIALIZED(_apiid) PWM_VALIDATE( (Pwm_ModuleState != PWM_STATE_INITIALIZED), _apiid, PWM_E_ALREADY_INITIALIZED)
#else
	#define PWM_VALIDATE(_exp, _apiid, _errid)
	#define Pwm_VALIDATE_CHANNEL(_apiid, _ch)
	#define Pwm_VALIDATE_INITIALIZED(_apiid)
	#define Pwm_VALIDATE_UNINITIALIZED(_apiid)
#endif

const Pwm_ConfigType* PwmConfigPtr = NULL;

void* PwmBaseAddresses[7] = {
		(void*)PWM_ePWM1_BASE_ADDR,
		(void*)PWM_ePWM2_BASE_ADDR,
		(void*)PWM_ePWM3_BASE_ADDR,
		(void*)PWM_ePWM4_BASE_ADDR,
		(void*)PWM_ePWM5_BASE_ADDR,
		(void*)PWM_ePWM6_BASE_ADDR,
		(void*)PWM_ePWM7_BASE_ADDR
};
PWM_Handle PwmHandles[7];

typedef enum {
	PWM_STATE_UNINITIALIZED, PWM_STATE_INITIALIZED
} Pwm_ModuleStateType;

#define PWM_IOMM_KICK0_MAGIC (0x83e70b13)
#define PWM_IOMM_KICK1_MAGIC (0x95a4f1e0)

static Pwm_ModuleStateType Pwm_ModuleState = PWM_STATE_UNINITIALIZED;

/* Local functions */
#if PWM_NOTIFICATION_SUPPORTED==STD_ON
static void Pwm_Isr(uint8 channel);
ISR(Pwm1_Isr);
ISR(Pwm2_Isr);
ISR(Pwm3_Isr);
ISR(Pwm4_Isr);
ISR(Pwm5_Isr);
ISR(Pwm6_Isr);
ISR(Pwm7_Isr);
#endif

static void EnablePwmChannel(uint8 channel);
#if (PWM_DE_INIT_API == STD_ON )
static void DisablePwmChannel(uint8 channel);
#endif
static void EnableInterrupts(uint8 channel);
static uint16 calculateUpDownDuty(uint32 absoluteDuty, uint32 period);
#if (PWM_SET_PERIOD_AND_DUTY_API==STD_ON) || (PWM_SET_DUTY_CYCLE_API == STD_ON)
static void Internal_SetDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle);
#endif

static Pwm_ChannelType channelConfigMap[PWM_TOTAL_NOF_CHANNELS];
static boolean channelForcedIdle[PWM_TOTAL_NOF_CHANNELS];
/*
 * Store which physical channels have been configured in a bit field
 * 0 - not configured
 * 1 - configured
 */
static uint8 configuredChannels;


void Pwm_Init( const Pwm_ConfigType *ConfigPtr )
{
    uint8 i;

    /* @req SWS_Pwm_00118 */
    Pwm_VALIDATE_UNINITIALIZED( PWM_INIT_SERVICE_ID );

    /* @req SWS_Pwm_00046
       @req SWS_Pwm_10120
       @req SWS_Pwm_20120 */
	#if PWM_STATICALLY_CONFIGURED==STD_OFF
		PWM_VALIDATE((ConfigPtr != NULL), PWM_INIT_SERVICE_ID, PWM_E_PARAM_CONFIG);
	#else
		#error "PWM_STATICALLY_CONFIGURED is not supported"
	#endif

	PwmConfigPtr = ConfigPtr;

	/* Unlock IOMM registers. */
	*((uint32*)PWM_IOMM_KICK0) = PWM_IOMM_KICK0_MAGIC;
	*((uint32*)PWM_IOMM_KICK1) = PWM_IOMM_KICK1_MAGIC;

	uint32_t pwmClockHz = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_PWM);
	pwmClockHz /= 2; /* This is the default PWM prescaler on reset */

	/*
	 * According to TI hardware manual SPNU515A
	 * Section 19.2.2.3.2 specifies procedure for enabling PWM clocks:
	 * Enable clocks in IOMM, set TBLCLKSYNC = 0, configure modules,
	 * set TBLCLKSYNC = 1
	 */


	/* @req SWS_Pwm_00007 */
	/* @req SWS_Pwm_00062 */
	/* @req SWS_Pwm_00052 */
	for (i = 0; i < PWM_NUMBER_OF_CHANNELS; i++) {
		Pwm_ChannelType hwChannel = ConfigPtr->Channels[i].channel;
		PwmHandles[hwChannel] = PWM_init(PwmBaseAddresses[hwChannel], sizeof(PWM_Obj));
		EnablePwmChannel(hwChannel);
		#if PWM_NOTIFICATION_SUPPORTED==STD_ON
		Pwm_DisableNotification(hwChannel);
		#endif
	}

	*((uint32*)PWM_PINMMR37_BASE_ADDR) &= ~(1 << 1); /*PINMM37[1] is TBLCLKSYNC */

	/* @req SWS_Pwm_10009 */
	/* @req SWS_Pwm_20009 */
	/* @req SWS_Pwm_30009 */
	configuredChannels = 0;
	for (i = 0; i < PWM_NUMBER_OF_CHANNELS; i++) {
		Pwm_ChannelType hwChannel = ConfigPtr->Channels[i].channel;
		PWM_Obj *pwm = (PWM_Obj*)PwmHandles[hwChannel];
		channelConfigMap[hwChannel] = i; /*to use in interrupt handler/channel class*/
		configuredChannels |= (1 << hwChannel);
		PWM_setCounterMode(PwmHandles[hwChannel], PWM_CounterMode_UpDown);
		/* Indexing the config with i below is correct! */
		uint32_t period = pwmClockHz / ConfigPtr->Channels[i].settings.frequency;
		/* Hardware has a 16-bit TBPRD register, which denotes the
		 * half-period value, so if period exceeds 65535 * 2, we have an invalid
		 * config.
		 * If we run PWM clock at 45 MHz, then PWM configured frequency would have
		 * to be >= 344 Hz, approx. 2.9 ms
		 * Period has to be >= 2 for it to make sense */
		PWM_VALIDATE((period <= 65535 * 2), PWM_INIT_SERVICE_ID, PWM_E_PARAM_CONFIG);
		PWM_VALIDATE((period >= 2), PWM_INIT_SERVICE_ID, PWM_E_PARAM_CONFIG);
		pwm->TBPRD = period / 2; /* TBPRD is @ half of PWM period in up-down mode */
		pwm->TBCTR = ConfigPtr->Channels[i].settings.counter;
		pwm->CMPA = ConfigPtr->Channels[i].settings.duty;
		uint16_t configDuty = ConfigPtr->Channels[i].settings.duty;
		uint32_t absoluteDuty = ((uint32_t)period * configDuty) >> 15;
		pwm->CMPA = calculateUpDownDuty(absoluteDuty, period);
		PWM_VALIDATE((pwm->CMPA > 1), PWM_INIT_SERVICE_ID, PWM_E_PARAM_CONFIG);
		/*
		 * The hardware can produce two PWM outputs on each channel, A and B
		 * In our driver, A has polarity as configured by the user
		 * B happens through the deadband module
		 */
		if (ConfigPtr->Channels[i].settings.polarity == PWM_HIGH) {
			/* Up-down PWM counter implies that we set the output
			 * to the configured polarity when the counter is at 0
			 * Then we toggle the output when the duty cycle compare
			 * value is reached
			 */
			PWM_setActionQual_Zero_PwmA(PwmHandles[hwChannel], PWM_ActionQual_Set);
			/* Force output once, thereby getting the correct initial state */
			pwm->AQSFRC &= 0xFFFC;
			pwm->AQSFRC |= (2 << 1);
		} else {
			PWM_setActionQual_Zero_PwmA(PwmHandles[hwChannel], PWM_ActionQual_Clear);
			/* Force output once, thereby getting the correct initial state */
			pwm->AQSFRC &= 0xFFFC;
			pwm->AQSFRC |= (1 << 1);
		}
		/* With duty cycle reached, we toggle, regardless of polarity */
		if (configDuty <= 0x4000) {
			PWM_setActionQual_CntUp_CmpA_PwmA(PwmHandles[hwChannel],
					PWM_ActionQual_Toggle);
		} else {
			PWM_setActionQual_CntDown_CmpA_PwmA(PwmHandles[hwChannel],
					PWM_ActionQual_Toggle);
		}
		/* Special handling for 0% and 100% duty to prevent two conflicting events
		 * from firing in the HW, so just set the output to constant high or low
		 */
		if (configDuty == 0 || configDuty == 0x8000) {
			PWM_setActionQual_CntUp_CmpA_PwmA(PwmHandles[hwChannel],
								PWM_ActionQual_Disabled);
			if (configDuty == 0) {
				PWM_setActionQualContSWForce_PwmA(PwmHandles[hwChannel], ConfigPtr->Channels[i].settings.polarity == STD_HIGH ? PWM_ActionQualContSWForce_Clear : PWM_ActionQualContSWForce_Set);
			} else {
				PWM_setActionQualContSWForce_PwmA(PwmHandles[hwChannel], ConfigPtr->Channels[i].settings.polarity == STD_HIGH ? PWM_ActionQualContSWForce_Set : PWM_ActionQualContSWForce_Clear);
			}
		}
		/* Dead-band setup. This is not part of Autosar but an extra feature
		 * that can be desirable for TMS570 users
		 */
		if (ConfigPtr->Channels[i].settings.deadband != 0)
		{
			uint32_t dbPeriod = pwmClockHz / ConfigPtr->Channels[i].settings.deadband;
			PWM_setDeadBandInputMode(PwmHandles[hwChannel], PWM_DeadBandInputMode_EPWMxA_Rising_and_Falling);
			PWM_setDeadBandOutputMode(PwmHandles[hwChannel], PWM_DeadBandOutputMode_EPWMxA_Rising_EPWMxB_Falling);
			PWM_setDeadBandPolarity(PwmHandles[hwChannel], PWM_DeadBandPolarity_EPWMxB_Inverted);
			PWM_setDeadBandRisingEdgeDelay(PwmHandles[hwChannel], dbPeriod);
			PWM_setDeadBandFallingEdgeDelay(PwmHandles[hwChannel], dbPeriod);
		}

		/*
		 * One more custom feature, an interrupt that always triggers in the middle of a period,
		 * regardless of whether there is a rising/falling edge at that time.
		 * Cannot be used together with the regular ASR-standard edge notification!
		 */
		if (ConfigPtr->Channels[i].settings.midPeriodInterrupt == 1) {
			pwm->ETCLR |= (1 << 0);
			EnableInterrupts(hwChannel);
			PWM_setActionQual_Period_PwmA(PwmHandles[hwChannel], PWM_ActionQual_Disabled);
			PWM_setIntMode(PwmHandles[hwChannel], PWM_IntMode_CounterEqualZeroOrPeriod);
		}

		/*
		 * For updates at the end of period, leave default shadowing of CMPA on,
		 * and write to the real register on period clock event.
		 * For unbuffered updates, disable shadowing of the CMPA register.
		*/
		#if (PWM_DUTYCYCLE_UPDATED_ENDPERIOD==STD_ON)
			PWM_setShadowMode_CmpA(PwmHandles[hwChannel], PWM_ShadowMode_Shadow);
			PWM_setLoadMode_CmpA(PwmHandles[hwChannel], PWM_LoadMode_Zero);
		#else
			PWM_setShadowMode_CmpA(PwmHandles[hwChannel], PWM_ShadowMode_Immediate);
		#endif
	}

	 /*set TBLCKLSYNC to 1, HW init complete */
	*((uint32*)PWM_PINMMR37_BASE_ADDR) |= (1 << 1);

#if PWM_NOTIFICATION_SUPPORTED==STD_ON
	 ISR_INSTALL_ISR1( "Pwm1", Pwm1_Isr, PWM_CH1_INT, PWM_ISR_PRIORITY,  0 );
	 ISR_INSTALL_ISR1( "Pwm2", Pwm2_Isr, PWM_CH2_INT, PWM_ISR_PRIORITY,  0 );
	 ISR_INSTALL_ISR1( "Pwm3", Pwm3_Isr, PWM_CH3_INT, PWM_ISR_PRIORITY,  0 );
	 ISR_INSTALL_ISR1( "Pwm4", Pwm4_Isr, PWM_CH4_INT, PWM_ISR_PRIORITY,  0 );
	 ISR_INSTALL_ISR1( "Pwm5", Pwm5_Isr, PWM_CH5_INT, PWM_ISR_PRIORITY,  0 );
	 ISR_INSTALL_ISR1( "Pwm6", Pwm6_Isr, PWM_CH6_INT, PWM_ISR_PRIORITY,  0 );
	 ISR_INSTALL_ISR1( "Pwm7", Pwm7_Isr, PWM_CH7_INT, PWM_ISR_PRIORITY,  0 );
#endif

	 /* Lock IOMM registers */
	 *((uint32*)PWM_IOMM_KICK0) = 0x00000000;

  Pwm_ModuleState = PWM_STATE_INITIALIZED;
}


/* @req SWS_Pwm_20080 */
#if (PWM_DE_INIT_API == STD_ON )
void Pwm_DeInit( void )
{
  Pwm_VALIDATE_INITIALIZED( PWM_DEINIT_SERVICE_ID );

  uint8 i;

  /*
   * @req SWS_Pwm_00010
   * @req SWS_Pwm_00011
   * @req SWS_Pwm_00012
   */
  for (i = 0; i < PWM_NUMBER_OF_CHANNELS; i++) {
	  Pwm_ChannelType hwChannel = PwmConfigPtr->Channels[i].channel;
#if PWM_NOTIFICATION_SUPPORTED==STD_ON
      Pwm_DisableNotification(hwChannel);
#endif
  		Pwm_OutputStateType idleState = PwmConfigPtr->Channels[i].settings.idleState;
  		if (idleState == PWM_HIGH) {
  			PWM_setActionQualContSWForce_PwmA(PwmHandles[hwChannel], PWM_ActionQualContSWForce_Set);
  		} else if (idleState == PWM_LOW) {
  			PWM_setActionQualContSWForce_PwmA(PwmHandles[hwChannel], PWM_ActionQualContSWForce_Clear);
  		}
  		DisablePwmChannel(hwChannel);
  }

  Pwm_ModuleState = PWM_STATE_UNINITIALIZED;
}
#endif

/* @req SWS_Pwm_20082 */
#if (PWM_SET_DUTY_CYCLE_API == STD_ON)
void Pwm_SetDutyCycle( Pwm_ChannelType Channel, uint16 DutyCycle )
{
  Pwm_VALIDATE_INITIALIZED( PWM_SETDUTYCYCLE_SERVICE_ID );
  Pwm_VALIDATE_CHANNEL( PWM_SETDUTYCYCLE_SERVICE_ID, Channel );
  
  Internal_SetDutyCycle(Channel, DutyCycle);
}
#endif

/* @req SWS_Pwm_20083 */
#if PWM_SET_PERIOD_AND_DUTY_API==STD_ON
void Pwm_SetPeriodAndDuty(Pwm_ChannelType Channel, Pwm_PeriodType Period,
		uint16 DutyCycle) {

	Pwm_VALIDATE_INITIALIZED( PWM_SETPERIODANDDUTY_SERVICE_ID );
	Pwm_VALIDATE_CHANNEL( PWM_SETPERIODANDDUTY_SERVICE_ID, Channel );

	int idx = channelConfigMap[Channel];

	/* @req SWS_PWM_0041 */
	if (PwmConfigPtr->Channels[idx].settings.class != PWM_VARIABLE_PERIOD) {
		Pwm_Det_ReportError(PWM_SETPERIODANDDUTY_SERVICE_ID, PWM_E_PERIOD_UNCHANGEABLE);
		return;
	}

	uint32_t pwmClockHz = Mcu_Arc_GetPeripheralClock(PERIPHERAL_CLOCK_PWM);
	pwmClockHz /= 2; /* This is the default PWM prescaler on reset */
	uint32_t hwPeriod = pwmClockHz / PwmConfigPtr->Channels[idx].settings.frequency;
	/* Validate that this period can be done in hardware, same as in Pwm_Init */
	PWM_VALIDATE((hwPeriod <= 65535 * 2), PWM_SETPERIODANDDUTY_SERVICE_ID, PWM_E_PARAM_CONFIG);

	PWM_Obj *pwm = (PWM_Obj*)PwmHandles[Channel];
	pwm->TBPRD = hwPeriod / 2;

	EnableInterrupts(Channel);

    /* @req SWS_Pwm_00150 */
	if (Period == 0) {
		Internal_SetDutyCycle(Channel, 0);
    } else {
    	Internal_SetDutyCycle(Channel, DutyCycle);
    }
}
#endif

#if (PWM_SET_PERIOD_AND_DUTY_API==STD_ON) || (PWM_SET_DUTY_CYCLE_API == STD_ON)
static void Internal_SetDutyCycle( Pwm_ChannelType Channel, uint16 DutyCycle )
{
	  /* @req SWS_Pwm_00013
	   * @req SWS_Pwm_00014
	   * @req SWS_Pwm_00016
	   */
	  PWM_Obj *pwm = (PWM_Obj*)PwmHandles[Channel];
	  uint32_t period = pwm->TBPRD * 2;
	  uint32_t absoluteDuty = ((uint32_t)period * DutyCycle) >> 15;
	  pwm->CMPA = calculateUpDownDuty(absoluteDuty, period);
	  int idx = channelConfigMap[Channel];
	  /*Set the compare action again: duty cycle may now be on
	   the other side of TBPRD (middle of period) */
	  if (DutyCycle == 0 || DutyCycle == 0x8000) {
		  PWM_setActionQual_CntUp_CmpA_PwmA(PwmHandles[Channel], PWM_ActionQual_Disabled);
		  PWM_setActionQual_CntDown_CmpA_PwmA(PwmHandles[Channel], PWM_ActionQual_Disabled);
		  Pwm_OutputStateType polarity = PwmConfigPtr->Channels[idx].settings.polarity;
		  if (DutyCycle == 0) {
			  PWM_setActionQualContSWForce_PwmA(PwmHandles[Channel], polarity == STD_HIGH ? PWM_ActionQualContSWForce_Clear : PWM_ActionQualContSWForce_Set);
		  } else {
			  PWM_setActionQualContSWForce_PwmA(PwmHandles[Channel], polarity == STD_HIGH ? PWM_ActionQualContSWForce_Set : PWM_ActionQualContSWForce_Clear);
		  }
	  }
	  else
	  {
		if (absoluteDuty <= pwm->TBPRD) {
			PWM_setActionQual_CntUp_CmpA_PwmA(PwmHandles[Channel], PWM_ActionQual_Toggle);
			PWM_setActionQual_CntDown_CmpA_PwmA(PwmHandles[Channel], PWM_ActionQual_Disabled);

		} else {
			PWM_setActionQual_CntDown_CmpA_PwmA(PwmHandles[Channel], PWM_ActionQual_Toggle);
			PWM_setActionQual_CntUp_CmpA_PwmA(PwmHandles[Channel], PWM_ActionQual_Disabled);
		}
		PWM_setActionQualContSWForce_PwmA(PwmHandles[Channel], PWM_ActionQualContSWForce_Disabled);
	  }

	  /* @req SWS_Pwm_00017
	   * This requirement is to update duty cycle at the end of the period. That is the
	   * default behaviour of the hardware, and this setting is changed in Pwm_Init if
	   * PwmDutycycleUpdatedEndperiod was turned off
	   */

	  /* Reactivate channel if output was forced to idle */
	  /* @req SWS_Pwm_20086 */
	  if (channelForcedIdle[Channel]) {
		  PWM_setActionQualContSWForce_PwmA(PwmHandles[Channel], PWM_ActionQualContSWForce_Disabled);
		  channelForcedIdle[Channel] = false;
	  }
}
#endif

/* @req SWS_Pwm_20112 */
/* @req SWS_Pwm_20113 */
#if PWM_NOTIFICATION_SUPPORTED==STD_ON
void Pwm_DisableNotification(Pwm_ChannelType Channel) 
{
	Pwm_VALIDATE_INITIALIZED( PWM_DISABLENOTIFICATION_SERVICE_ID );
	Pwm_VALIDATE_CHANNEL( PWM_DISABLENOTIFICATION_SERVICE_ID, Channel );
	/* @req SWS_Pwm_00023 */
    PWM_disableInt(PwmHandles[Channel]);
}

void Pwm_EnableNotification(Pwm_ChannelType Channel,
        Pwm_EdgeNotificationType Notification) 
{
    Pwm_VALIDATE_CHANNEL( PWM_ENABLENOTIFICATION_SERVICE_ID, Channel );
    Pwm_VALIDATE_INITIALIZED( PWM_ENABLENOTIFICATION_SERVICE_ID );

    /*@req SWS_PWM_00081 */
    /* clear any pending interrupts before enable */
    PWM_Obj *pwm = (PWM_Obj*)PwmHandles[Channel];
    pwm->ETCLR |= (1 << 0);

    /* @req SWS_Pwm_00024 */
    EnableInterrupts(Channel);

    Pwm_OutputStateType polarity = PwmConfigPtr->Channels[Channel].settings.polarity;
    uint16_t configDuty = PwmConfigPtr->Channels[Channel].settings.duty;
    uint8_t cmpADirection = 0;
    if (configDuty <= pwm->TBPRD / 2) {
    	cmpADirection = 1;
    }

    /*
     * 	When should an interrupt be generated?
     * 	New PWM period begins on TBCTR = 0
     * 	Duty cycle is at TBPRD = CMPA in one of the directions
     * 	cmpADirection is 0 for down-count and 1 for up-count
     *	 |=================================|
     *   |            |  HIGH   |  LOW     |
     *   |FALLING     |  Duty   |  Zero    |
     *   |RISING      |  Zero   |  Duty    |
     *   |=================================|
     */

    PWM_IntMode_e mode = (cmpADirection == 1) ?
    							PWM_IntMode_CounterEqualCmpAIncr :
    							PWM_IntMode_CounterEqualCmpADecr;
    switch(Notification) {
    case PWM_FALLING_EDGE:
    	if (polarity == PWM_HIGH) {
    		PWM_setIntMode(PwmHandles[Channel], mode);
    	} else if (polarity == PWM_LOW) {
    		PWM_setIntMode(PwmHandles[Channel], PWM_IntMode_CounterEqualZero);
    	}
    	break;
    case PWM_RISING_EDGE:
    	if (polarity == PWM_HIGH) {
    		PWM_setIntMode(PwmHandles[Channel], PWM_IntMode_CounterEqualZero);
    	} else if (polarity == PWM_LOW) {
    		PWM_setIntMode(PwmHandles[Channel], mode);
    	}
    	break;
    case PWM_BOTH_EDGES:
    	/* IMPROVEMENT: Add support for both-edge notification */
    	break;
    }
}

ISR(Pwm1_Isr) {
	Pwm_Isr(PWM_CHANNEL_1);
}

ISR(Pwm2_Isr) {
	Pwm_Isr(PWM_CHANNEL_2);
}

ISR(Pwm3_Isr) {
	Pwm_Isr(PWM_CHANNEL_3);
}

ISR(Pwm4_Isr) {
	Pwm_Isr(PWM_CHANNEL_4);
}

ISR(Pwm5_Isr) {
	Pwm_Isr(PWM_CHANNEL_5);
}

ISR(Pwm6_Isr) {
	Pwm_Isr(PWM_CHANNEL_6);
}

ISR(Pwm7_Isr) {
	Pwm_Isr(PWM_CHANNEL_7);
}

static void Pwm_Isr(uint8 channel) {
	int idx = channelConfigMap[channel];
	PWM_Obj *pwm = (PWM_Obj*)PwmHandles[channel];
	uint16_t intsel = pwm->ETSEL & (0x7);
	if (intsel == 3) { /* TBCTR = ZERO interrupt */
		if (PwmConfigPtr->ArcNotificationHandlers[idx] != NULL) {
			PwmConfigPtr->ArcNotificationHandlers[idx]();
		}
	} else {
		if (PwmConfigPtr->NotificationHandlers[idx] != NULL) {
			PwmConfigPtr->NotificationHandlers[idx]();
		}
	}
	pwm->ETCLR |= (1 << 0); /*clear pending interrupt flag */
}
#endif /* PWM_NOTIFICATION_SUPPORTED==STD_ON */

/* @req SWS_Pwm_20084 */
#if (PWM_SET_OUTPUT_TO_IDLE_API == STD_ON)
void Pwm_SetOutputToIdle(Pwm_ChannelType Channel) {
	Pwm_VALIDATE_CHANNEL( PWM_SETOUTPUTTOIDLE_SERVICE_ID, Channel );
	Pwm_VALIDATE_INITIALIZED( PWM_SETOUTPUTTOIDLE_SERVICE_ID );

	/* @req SWS_Pwm_00021 */
	Pwm_OutputStateType idleState = PwmConfigPtr->Channels[Channel].settings.idleState;
	if (idleState == PWM_HIGH) {
		PWM_setActionQualContSWForce_PwmA(PwmHandles[Channel],
				PWM_ActionQualContSWForce_Set);
	} else if (idleState == PWM_LOW) {
		PWM_setActionQualContSWForce_PwmA(PwmHandles[Channel],
				PWM_ActionQualContSWForce_Clear);
	}
	channelForcedIdle[Channel] = true;
}
#endif

/* @req SWS_Pwm_20085 */
#if PWM_GET_OUTPUT_STATE_API==STD_ON
Pwm_OutputStateType Pwm_GetOutputState(Pwm_ChannelType Channel) {
	Pwm_VALIDATE_CHANNEL(PWM_GETOUTPUTSTATE_SERVICE_ID, Channel)
	return PWM_LOW; /* IMPROVEMENT: implement Pwm_GetOutputState */
}
#endif

#if ( PWM_VERSION_INFO_API == STD_ON)
void Pwm_GetVersionInfo( Std_VersionInfoType* versioninfo) {

    /* @req SWS_Pwm_00151 */
    PWM_VALIDATE( ( NULL != versioninfo ), PWM_GETVERSIONINFO_SERVICE_ID, PWM_E_PARAM_POINTER);

    versioninfo->vendorID = PWM_VENDOR_ID;
    versioninfo->moduleID = PWM_MODULE_ID;
    versioninfo->sw_major_version = PWM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = PWM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = PWM_SW_PATCH_VERSION;
}
#endif


static void EnablePwmChannel(uint8 channel) {
	/* The register and bit to enable clock to each PWM are from
	   the TI reference SPNS192A, sec. 5.1.1, table 5-1 */

	if (channel == 0) { /* PINMMR37[8] */
		*((uint32*)PWM_PINMMR37_BASE_ADDR) |= (1 << 8);
	} else if (channel == 1) { /* PINMMR37[16] */
		*((uint32*)PWM_PINMMR37_BASE_ADDR) |= (1 << 16);
	} else if (channel == 2) { /* PINMMR37[24] */
		*((uint32*)PWM_PINMMR37_BASE_ADDR) |= (1 << 24);
	} else if (channel == 3) { /* PINMMR38[0] */
		*((uint32*)PWM_PINMMR38_BASE_ADDR) |= (1 << 0);
	} else if (channel == 4) { /* PINMMR38[8] */
		*((uint32*)PWM_PINMMR38_BASE_ADDR) |= (1 << 8);
	} else if (channel == 5) { /* PINMMR38[16] */
		*((uint32*)PWM_PINMMR38_BASE_ADDR) |= (1 << 16);
	} else if (channel == 6) { /* PINMMR38[24] */
		*((uint32*)PWM_PINMMR38_BASE_ADDR) |= (1 << 24);
	}
}

#if (PWM_DE_INIT_API == STD_ON )
static void DisablePwmChannel(uint8 channel) {
	if (channel == 0) { /* PINMMR37[8] */
		*((uint32*) PWM_PINMMR37_BASE_ADDR) &= ~(1 << 8);
	} else if (channel == 1) { /* PINMMR37[16] */
		*((uint32*) PWM_PINMMR37_BASE_ADDR) &= ~(1 << 16);
	} else if (channel == 2) { /* PINMMR37[24] */
		*((uint32*) PWM_PINMMR37_BASE_ADDR) &= ~(1 << 24);
	} else if (channel == 3) { /* PINMMR38[0] */
		*((uint32*) PWM_PINMMR38_BASE_ADDR) &= ~(1 << 0);
	} else if (channel == 4) { /* PINMMR38[8] */
		*((uint32*) PWM_PINMMR38_BASE_ADDR) &= ~(1 << 8);
	} else if (channel == 5) { /* PINMMR38[16] */
		*((uint32*) PWM_PINMMR38_BASE_ADDR) &= ~(1 << 16);
	} else if (channel == 6) { /* PINMMR38[24] */
		*((uint32*) PWM_PINMMR38_BASE_ADDR) &= ~(1 << 24);
	}
}
#endif

static void EnableInterrupts(uint8 channel) {
	PWM_enableInt(PwmHandles[channel]);
	PWM_setIntPeriod(PwmHandles[channel], PWM_IntPeriod_FirstEvent);
}

static uint16 calculateUpDownDuty(uint32 absoluteDuty, uint32 period) {
	if (absoluteDuty <= period / 2) {
		return absoluteDuty;
	} else {
		return (period / 2) - (absoluteDuty - (period / 2));
	}
}
