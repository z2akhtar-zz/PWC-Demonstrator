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


/** @tagSettings DEFAULT_ARCHITECTURE=PPC|MPC5607B|MPC5645S */
/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */

#include <assert.h>
#include <string.h>

#include "Pwm.h"
#include "MemMap.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#include "mpc55xx.h"

#include "Os.h"
#include "Mcu.h"
#if PWM_NOTIFICATION_SUPPORTED==STD_ON
#include "isr.h"
#include "irq.h"
#include "arc.h"
#endif

/* General requirement tagging */

/* @req  SWS_Pwm_00062 Pwm_Init shall only initialize the configured resources */
/* @req  SWS_Pwm_10051 PWM function shall report errors to the DET*/
/* @req  SWS_Pwm_20051 If DET error the PWM function shall skip the desired functionality. */
/* @req  SWS_Pwm_10086 After Pwm_SetOutputToIdle() variable period type channels reactivate through Pwm_SetPeriodAndDuty() */
/* @req  SWS_Pwm_20086 After Pwm_SetOutputToIdle() channels reactivate through Pwm_SetDutyCycle() */
/* @req  SWS_Pwm_00105 Pwm_Notification service specification*/
/* @req  SWS_Pwm_00104 Optional Interfaces. Partly implemented, we don't use DEM. */
/* @req  SWS_Pwm_00088 Functions should be re-entrant for different PWM channel numbers*/
/* @req  SWS_Pwm_00070 All time units should be ticks*/
/* @req  SWS_Pwm_20002 PWM should report DET error PWM_E_UNINIT when used without prior module init. */
/* @req  SWS_Pwm_30002 PWM should report DET error PWM_E_PARAM_CHANNEL when used with invalid channel id.*/

#if ( PWM_DEV_ERROR_DETECT == STD_ON )
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(PWM_MODULE_ID,0,_api,_err); \
          return; \
        }

#define VALIDATE_W_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(PWM_MODULE_ID,0,_api,_err); \
          return (_rv); \
        }
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_W_RV(_exp,_api,_err,_rv )
#endif


#if defined(CFG_MPC5604B) || defined(CFG_MPC5606B) || defined(CFG_MPC5607B)
#   define PWM_RUNTIME_CHANNEL_COUNT    PWM_MAX_CHANNEL
#   define IS_VALID_CHANNEL(_x) ((_x) < PWM_MAX_CHANNEL)
#elif defined(CFG_MPC5606S)
#   define PWM_RUNTIME_CHANNEL_COUNT    PWM_MAX_CHANNEL
#   define IS_VALID_CHANNEL(_x) ((((_x) < PWM_MAX_CHANNEL) && ((_x) >= 40)) || (((_x) <= 23) && ((_x) >= 16)))
#elif defined(CFG_MPC5645S)
#   define PWM_RUNTIME_CHANNEL_COUNT    PWM_MAX_CHANNEL
#   define IS_VALID_CHANNEL(_x) (((_x) < PWM_MAX_CHANNEL) && ((((_x) <= 23) && ((_x) >= 16)) || (((_x) <= 47) && ((_x) >= 32))))
#elif defined(CFG_MPC5644A)
#   define PWM_RUNTIME_CHANNEL_COUNT    24
#   define IS_VALID_CHANNEL(_x) ((_x) < PWM_RUNTIME_CHANNEL_COUNT)
#elif defined(CFG_MPC563XM)
    #define PWM_RUNTIME_CHANNEL_COUNT   24
    #define IS_VALID_CHANNEL(_x) ( ((_x) < PWM_RUNTIME_CHANNEL_COUNT) && \
                                               ( ((_x) == 23) || \
                                                 (((_x) >= 8) && (_x <= 15) ) || \
                                                 ((_x) == 0) || \
                                                 ((_x) == 2) || \
                                                 ((_x) == 4) ))
#elif defined(CFG_MPC5516)
#   define PWM_RUNTIME_CHANNEL_COUNT    16
#   define IS_VALID_CHANNEL(_x) ((_x) < 16)
#elif defined(CFG_MPC5567)
#   define PWM_RUNTIME_CHANNEL_COUNT   24
#   define IS_VALID_CHANNEL(_x) ((_x) < 24)
#else
#   error Not supported MCU
#endif

const Pwm_ConfigType* Pwm_ConfigPtr = NULL;

typedef enum {
    PWM_STATE_UNINITIALIZED, PWM_STATE_INITIALIZED
} Pwm_ModuleStateType;

static Pwm_ModuleStateType Pwm_ModuleState = PWM_STATE_UNINITIALIZED;
#if PWM_NOTIFICATION_SUPPORTED==STD_ON
// Run-time variables
typedef struct {
    Pwm_EdgeNotificationType NotificationState;
} Pwm_ChannelStructType;

// We use Pwm_ChannelType as index here
Pwm_ChannelStructType ChannelRuntimeStruct[PWM_RUNTIME_CHANNEL_COUNT];
#endif

#if PWM_DE_INIT_API==STD_ON
static void Pwm_DeInitChannel(Pwm_ChannelType Channel);
#endif

#if PWM_NOTIFICATION_SUPPORTED==STD_ON
ISR(Pwm_Isr);
#endif
/**
 * Calculates period ticks and prescaler
 *
 * @param channelConfig Pointer to configuration struct representing a PWM channel.
 * @param ticks Calculated nr of ticks.
 * @param prescaler Calculated prescaler.
 */
static void calcPeriodTicksAndPrescaler(
		const Pwm_ChannelConfigurationType* channelConfig,
		uint16* ticks, Pwm_ChannelPrescalerType* prescaler) {

	uint32 pre_global = 0;
	uint32 f_in = 0;

#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
	Pwm_ChannelType channel = channelConfig->channel;

	if(channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
		f_in = Mcu_Arc_GetPeripheralClock( PERIPHERAL_CLOCK_EMIOS_0 );
		pre_global = EMIOS_0.MCR.B.GPRE; /*lint !e923 Ok, cast in Freescale header file */
	} else {
		f_in = Mcu_Arc_GetPeripheralClock( PERIPHERAL_CLOCK_EMIOS_1 );
		pre_global = EMIOS_1.MCR.B.GPRE; /*lint !e923 Ok, cast in Freescale header file */
	}
#else
	f_in = Mcu_Arc_GetPeripheralClock( PERIPHERAL_CLOCK_EMIOS );
	pre_global = EMIOS.MCR.B.GPRE;
#endif

	uint32 f_target = channelConfig->frequency;
	uint32 pre;
	uint32 ticks_temp;

	if(f_target > 0){
		if (channelConfig->prescaler == PWM_CHANNEL_PRESCALER_AUTO) {
			// Go from lowest to highest prescaler
			for (pre = PWM_CHANNEL_PRESCALER_1; pre <= PWM_CHANNEL_PRESCALER_4; pre++) {
				/*lint -e{632, 633} Ok, skip dimension analysis */
				ticks_temp = f_in / (f_target * (pre_global + 1uL) * (pre + 1uL)); // Calc ticks
				if (ticks_temp > 0xffff) {
					ticks_temp = 0xffff;  // Prescaler too low
				} else {
					break;                // Prescaler ok
				}
			}
		} else {
			pre = channelConfig->prescaler; // Use config setting
			/*lint -e{632, 633} Ok, skip dimension analysis */
			ticks_temp = f_in / (f_target * (pre_global + 1uL) * (pre+1uL)); // Calc ticks
			if (ticks_temp > 0xffff) {
				ticks_temp = 0xffff;  // Prescaler too low
			}
		}

		(*ticks) = ticks_temp; /*lint !e734 ticks_temp is never greater than 0xFFFF */
		(*prescaler) = (Pwm_ChannelPrescalerType) pre;
	}
	else{	/* Divisor check, f_target == 0 */
		(*ticks) = 0xffff;
		(*prescaler) = PWM_CHANNEL_PRESCALER_4;
	}

}

/**
 * Sets the duty cycle
 *
 * @param Channel Numeric identifier of a PWM channel
 * @param DutyCycle Wanted duty cycle
 */
static void setDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle)
{
    uint16 actualDutyCyle = DutyCycle; /* Variable needed for MISRA compliance */

    if (actualDutyCyle > PWM_100_PERCENT) {
        actualDutyCyle = PWM_100_PERCENT;
#if PWM_DEV_ERROR_DETECT==STD_ON
        (void)Det_ReportError(PWM_MODULE_ID,0,PWM_GLOBAL_SERVICE_ID, PWM_E_EXECUTION_ERROR);
#endif
    }

    Pwm_ChannelType Chnl = Channel;;

    volatile struct EMIOS_tag *emiosHw;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        Chnl = Channel - (Pwm_ChannelType) PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file  */
#endif

    /* @req  SWS_Pwm_00058 */
    /* @req  SWS_Pwm_00059 */
    /* leading_edge_position variable is 16 bit used with interpretation 0% = 0 and 100%= 0x8000 */

    /* @req  SWS_Pwm_00016 */
    /* @req  SWS_Pwm_00018 */

    uint16 leading_edge_position = (uint16) ((uint32)(emiosHw->CH[Chnl].CBDR.R
            * (uint32) actualDutyCyle) >> 15u);
#if defined(CFG_MPC5567)
    /* Special treatment of 0% and 100% duty cycles for MPC5567 */
    if(PWM_0_PERCENT == actualDutyCyle) {
        /*
         * From "MPC5567 Microcontroller Reference Manual, Rev. 1":
         * In order to achieve 0% duty cycle, both registers A1 and B1 must be set to the same value.
         */
        leading_edge_position = (uint16) emiosHw->CH[Chnl].CBDR.R;
    }
    else if(PWM_100_PERCENT == actualDutyCyle) {
        /*
         * From "MPC5567 Microcontroller Reference Manual, Rev. 1":
         * 100% duty cycle is possible by writing 0x000000 to register A.
         */
        leading_edge_position = 0;
    }
#endif

    /* Timer instant for leading edge */


    /* @req  SWS_Pwm_00017
     * The function Pwm_SetDutyCycle shall update the duty cycle at
     * the end of the period if supported by the implementation and configured
     * with PwmDutycycleUpdatedEndperiod. [ This is achieved in hardware since
     * the A and B registers are double buffered ] and A1 and B1 are reloaded
     * with A2 and B2 at cycle boundary i.e when counter becomes 1.
     */

	/* @req  SWS_Pwm_00014
     * The function Pwm_SetDutyCycle shall set the output state according
     * to the configured polarity parameter [which is already set from
     * Pwm_Init], when the duty parameter is 0% [=0] or 100% [=0x8000].
     */

    /* @req  SWS_Pwm_00013 */
    emiosHw->CH[Chnl].CADR.R = (vuint32_t) leading_edge_position;
}

/**
 * Configures a PWM channel.
 *
 * @param channelConfig Pointer to configuration struct representing a PWM channel.
 */
static void configureChannel(const Pwm_ChannelConfigurationType* channelConfig){

    Pwm_ChannelPrescalerType prescaler;
    uint16 period_ticks;
	Pwm_ChannelType globalChannel = channelConfig->channel;
	Pwm_ChannelType localChannel = globalChannel;
    volatile struct EMIOS_tag *emiosHw;

#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(globalChannel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        localChannel = globalChannel - (Pwm_ChannelType)PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif

	emiosHw->CH[localChannel].CCR.B.MODE = 0;

#if defined(CFG_MPC5567)
	emiosHw->CH[localChannel].CCR.B.DMA = 0; /* FLAG used for interrupts, not DMA */
	emiosHw->CH[localChannel].CCR.B.BSL = 3; /* Bus select: Internal counter.*/
	emiosHw->CH[localChannel].CCR.B.ODIS = 0; /* Output disable: Off */
#else
	emiosHw->CH[localChannel].CCR.B.DMA = 0; /* FLAG used for interrupts, not DMA */
	emiosHw->CH[localChannel].CCR.B.BSL = (vuint32_t)channelConfig->clksrc; /* Bus select */
	emiosHw->CH[localChannel].CCR.B.ODIS = 0; /* Output disable: Off */
#endif

	calcPeriodTicksAndPrescaler( channelConfig, &period_ticks, &prescaler ); /*lint !e934 Helper func responsible for treating args as temporary pointers */

	emiosHw->CH[localChannel].CBDR.R = (vuint32_t)period_ticks;
	emiosHw->CH[localChannel].CCR.B.UCPRE = (vuint32_t)prescaler;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
	emiosHw->CH[localChannel].CCR.B.UCPEN = 1; /*Prescaler enable */
#else
	emiosHw->CH[localChannel].CCR.B.UCPREN = 1; /*Prescaler enable */
#endif
    /* @req SWS_Pwm_10009  */
    /* @req SWS_Pwm_20009  */
    /* @req SWS_Pwm_30009  */
	setDutyCycle(globalChannel, channelConfig->duty);

    // 0 A match on comparator A clears the output flip-flop, while a match on comparator B sets it
    // 1 A match on comparator A sets the output flip-flop, while a match on comparator B clears it
    // A duty cycle of X % should give a signal with state 'channelConfig->polarity' during
    // X % of the period time.
	emiosHw->CH[localChannel].CCR.B.EDPOL = (channelConfig->polarity == PWM_LOW) ? 1 : 0;

#if defined (CFG_MPC5567)
	emiosHw->CH[localChannel].CCR.B.MODE = (vuint32_t)PWM_EMIOS_OPWM; /* Select the mode of operation of the Unified Channel */
#else

	// Handle counter bus provider mode: A-register shall contain the counter period
	if (channelConfig->mode == PWM_MODE_COUNTER_BUS_PROVIDER_MCB_UP) {
		emiosHw->CH[localChannel].CADR.R = (vuint32_t)period_ticks;
	}

	emiosHw->CH[localChannel].CCR.B.MODE = (vuint32_t) channelConfig->mode; /* Select the mode of operation of the Unified Channel */
#endif

    /* @req SWS_Pwm_00052  */
    /* On hw reset the CCR.B.FEN is set to zero by default */

}

#if (PWM_NOTIFICATION_SUPPORTED == STD_ON)
/**
 * Helper function for installing interrupts.
 *
 * @param hwUnit Hardware unit
 * @param channelId	Channel id
 */
static void installChannelInterrupt(uint8 hwUnit, Pwm_ChannelType channelId)
{
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)

    Pwm_ChannelType channel = channelId;

    if (hwUnit == 1) {
        channel += PWM_NUMBER_OF_EACH_EMIOS;
    }

#else
    Pwm_ChannelType channel = channelId;
#endif

    /*lint -save -e641 Conversion ok in ISR_INSTALL_ISR2 */
    // Install ISR
#if defined(CFG_MPC560XB) || defined(CFG_MPC5604P) || defined(CFG_MPC5606S)
    switch(channel)
    {
#if !defined(CFG_MPC5606S)
    case 0:
    case 1:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F0_F1),PWM_ISR_PRIORITY, 0);
        break;
    case 2:
    case 3:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F2_F3),PWM_ISR_PRIORITY, 0);
        break;
    case 4:
    case 5:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F4_F5),PWM_ISR_PRIORITY, 0);
        break;
    case 6:
    case 7:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F6_F7),PWM_ISR_PRIORITY, 0);
        break;
    case 8:
    case 9:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F8_F9),PWM_ISR_PRIORITY, 0);
        break;
    case 10:
    case 11:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F10_F11),PWM_ISR_PRIORITY, 0);
        break;
    case 12:
    case 13:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F12_F13),PWM_ISR_PRIORITY, 0);
        break;
    case 14:
    case 15:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F14_F15),PWM_ISR_PRIORITY, 0);
        break;
#endif
    case 16:
    case 17:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F16_F17),PWM_ISR_PRIORITY, 0);
        break;
    case 18:
    case 19:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F18_F19),PWM_ISR_PRIORITY, 0);
        break;
    case 20:
    case 21:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F20_F21),PWM_ISR_PRIORITY, 0);
        break;
    case 22:
    case 23:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F22_F23),PWM_ISR_PRIORITY, 0);
        break;
#if !defined(CFG_MPC5606S)
    case 24:
    case 25:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F24_F25),PWM_ISR_PRIORITY, 0);
        break;
    case 26:
    case 27:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F26_F27),PWM_ISR_PRIORITY, 0);
        break;
    case 28:
    case 29:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F0_F1),PWM_ISR_PRIORITY, 0);
        break;
    case 30:
    case 31:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F2_F3),PWM_ISR_PRIORITY, 0);
        break;
    case 32:
    case 33:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F4_F5),PWM_ISR_PRIORITY, 0);
        break;
    case 34:
    case 35:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F6_F7),PWM_ISR_PRIORITY, 0);
        break;
    case 36:
    case 37:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F8_F9),PWM_ISR_PRIORITY, 0);
        break;
    case 38:
    case 39:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F10_F11),PWM_ISR_PRIORITY, 0);
        break;
    case 40:
    case 41:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F12_F13),PWM_ISR_PRIORITY, 0);
        break;
    case 42:
    case 43:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F14_F15),PWM_ISR_PRIORITY, 0);
        break;
#endif
    case 44:
    case 45:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F16_F17),PWM_ISR_PRIORITY, 0);
        break;
    case 46:
    case 47:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F18_F19),PWM_ISR_PRIORITY, 0);
        break;
    case 48:
    case 49:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F20_F21),PWM_ISR_PRIORITY, 0);
        break;
    case 50:
    case 51:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F22_F23),PWM_ISR_PRIORITY, 0);
        break;
#if !defined(CFG_MPC5606S)
    case 52:
    case 53:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F24_F25),PWM_ISR_PRIORITY, 0);
        break;
    case 54:
    case 55:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F26_F27),PWM_ISR_PRIORITY, 0);
        break;
#endif
    default:
        break;
    }
#elif defined(CFG_MPC5645S)
    switch(hwUnit) {
        case 0:
            switch(channelId) {
                case 8:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F8),PWM_ISR_PRIORITY, 0);
                    break;
                case 9:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F9),PWM_ISR_PRIORITY, 0);
                    break;
                case 10:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F10),PWM_ISR_PRIORITY, 0);
                    break;
                case 11:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F11),PWM_ISR_PRIORITY, 0);
                    break;
                case 12:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F12),PWM_ISR_PRIORITY, 0);
                    break;
                case 13:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F13),PWM_ISR_PRIORITY, 0);
                    break;
                case 14:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F14),PWM_ISR_PRIORITY, 0);
                    break;
                case 15:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F15),PWM_ISR_PRIORITY, 0);
                    break;
                case 16:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F16),PWM_ISR_PRIORITY, 0);
                    break;
                case 17:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F17),PWM_ISR_PRIORITY, 0);
                    break;
                case 18:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F18),PWM_ISR_PRIORITY, 0);
                    break;
                case 19:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F19),PWM_ISR_PRIORITY, 0);
                    break;
                case 20:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F20),PWM_ISR_PRIORITY, 0);
                    break;
                case 21:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F21),PWM_ISR_PRIORITY, 0);
                    break;
                case 22:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F22),PWM_ISR_PRIORITY, 0);
                    break;
                case 23:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_0_GFR_F23),PWM_ISR_PRIORITY, 0);
                    break;
                default:
                    break;
            }
            break;
        case 1:
            switch(channelId) {
                case 8:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F8),PWM_ISR_PRIORITY, 0);
                    break;
                case 9:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F9),PWM_ISR_PRIORITY, 0);
                    break;
                case 10:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F10),PWM_ISR_PRIORITY, 0);
                    break;
                case 11:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F11),PWM_ISR_PRIORITY, 0);
                    break;
                case 12:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F12),PWM_ISR_PRIORITY, 0);
                    break;
                case 13:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F13),PWM_ISR_PRIORITY, 0);
                    break;
                case 14:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F14),PWM_ISR_PRIORITY, 0);
                    break;
                case 15:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F15),PWM_ISR_PRIORITY, 0);
                    break;
                case 16:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F16),PWM_ISR_PRIORITY, 0);
                    break;
                case 17:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F17),PWM_ISR_PRIORITY, 0);
                    break;
                case 18:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F18),PWM_ISR_PRIORITY, 0);
                    break;
                case 19:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F19),PWM_ISR_PRIORITY, 0);
                    break;
                case 20:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F20),PWM_ISR_PRIORITY, 0);
                    break;
                case 21:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F21),PWM_ISR_PRIORITY, 0);
                    break;
                case 22:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F22),PWM_ISR_PRIORITY, 0);
                    break;
                case 23:
                    ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMIOS_1_GFR_F23),PWM_ISR_PRIORITY, 0);
                    break;
                default:
                    break;
            }
            break;
        default:
            (void)channel;
            break;
    }
#else
    switch(channel)
    {
    case 0:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F0),PWM_ISR_PRIORITY, 0);
        break;
    case 1:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F1),PWM_ISR_PRIORITY, 0);
        break;
    case 2:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F2),PWM_ISR_PRIORITY, 0);
        break;
    case 3:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F3),PWM_ISR_PRIORITY, 0);
        break;
    case 4:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F4),PWM_ISR_PRIORITY, 0);
        break;
    case 5:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F5),PWM_ISR_PRIORITY, 0);
        break;
    case 6:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F6),PWM_ISR_PRIORITY, 0);
        break;
    case 7:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F7),PWM_ISR_PRIORITY, 0);
        break;
    case 8:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F8),PWM_ISR_PRIORITY, 0);
        break;
    case 9:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F9),PWM_ISR_PRIORITY, 0);
        break;
    case 10:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F10),PWM_ISR_PRIORITY, 0);
        break;
    case 11:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F11),PWM_ISR_PRIORITY, 0);
        break;
    case 12:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F12),PWM_ISR_PRIORITY, 0);
        break;
    case 13:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F13),PWM_ISR_PRIORITY, 0);
        break;
    case 14:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F14),PWM_ISR_PRIORITY, 0);
        break;
    case 15:
        ISR_INSTALL_ISR2("PwmIsr", Pwm_Isr, (IrqType)(EMISOS200_FLAG_F15),PWM_ISR_PRIORITY, 0);
        break;
    default:
        break;
    }
#endif

    /*lint -restore */
}
#endif

/* @req SWS_Pwm_00095 */
void Pwm_Init(const Pwm_ConfigType* ConfigPtr) {

    /* @req SWS_Pwm_00046 */
    /* @req SWS_Pwm_10002 */
    VALIDATE( ( NULL != ConfigPtr ), PWM_INIT_SERVICE_ID, PWM_E_PARAM_CONFIG );

    Pwm_ChannelType channel_iterator;

    /** @req SWS_PWM_00118 */
    /** @req SWS_PWM_00121 */
    /** @req SWS_Pwm_50002 */

    if( Pwm_ModuleState == PWM_STATE_INITIALIZED ) {
#if PWM_DEV_ERROR_DETECT==STD_ON
        (void)Det_ReportError(PWM_MODULE_ID,0,PWM_INIT_SERVICE_ID,PWM_E_ALREADY_INITIALIZED);
#endif
        return;
    }

#if defined(CFG_MPC5606S) && !defined(CFG_MPC560XB)
    CGM.AC1_SC.R = 0x03000000; /*lint !e923 Ok, cast in Freescale header file */ /* MPC56xxS: Select aux. set 1 clock to be FMPLL0 */
    CGM.AC2_SC.R = 0x03000000; /*lint !e923 Ok, cast in Freescale header file */ /* MPC56xxS: Select aux. set 2 clock to be FMPLL0 */
#endif

    Pwm_ConfigPtr = ConfigPtr;

#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    /* Clock scaler uses system clock (~64MHz) as source, so prescaler 64 => 1MHz. */
    EMIOS_0.MCR.B.GPRE = PWM_PRESCALER_EMIOS_0 - 1; /*lint !e923 Ok, cast in Freescale header file */
    EMIOS_1.MCR.B.GPRE = PWM_PRESCALER_EMIOS_1 - 1; /*lint !e923 Ok, cast in Freescale header file */

    /* Enable eMIOS clock */
    EMIOS_0.MCR.B.GPREN = 1; /*lint !e923 Ok, cast in Freescale header file */
    EMIOS_1.MCR.B.GPREN = 1; /*lint !e923 Ok, cast in Freescale header file */

    /* Stop channels when in debug mode */
    EMIOS_0.MCR.B.FRZ = PWM_FREEZE_ENABLE; /*lint !e923 Ok, cast in Freescale header file */
    EMIOS_1.MCR.B.FRZ = PWM_FREEZE_ENABLE; /*lint !e923 Ok, cast in Freescale header file */

    /* Use global time base */
    EMIOS_0.MCR.B.GTBE = 1; /*lint !e923 Ok, cast in Freescale header file */
    EMIOS_1.MCR.B.GTBE = 1; /*lint !e923 Ok, cast in Freescale header file */
#else
    /* Clock scaler uses system clock (~64MHz) as source, so prescaler 64 => 1MHz. */
    EMIOS.MCR.B.GPRE = PWM_PRESCALER - 1; /*lint !e923 Ok, cast in Freescale header file */

    /* Enable eMIOS clock */
    EMIOS.MCR.B.GPREN = 1; /*lint !e923 Ok, cast in Freescale header file */

    /* Stop channels when in debug mode */
    EMIOS.MCR.B.FRZ = PWM_FREEZE_ENABLE; /*lint !e923 Ok, cast in Freescale header file */

    /* Use global time base */
    EMIOS.MCR.B.GTBE = 1; /*lint !e923 Ok, cast in Freescale header file */
#endif

    Pwm_ModuleState = PWM_STATE_INITIALIZED;

    /* Enable EMIOS clock (for channel register access in case module has been disabled MDIS==1, which is done by Pwm_DeInit) */
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    EMIOS_0.MCR.B.MDIS = 0; /*lint !e923 Ok, cast in Freescale header file */
    EMIOS_1.MCR.B.MDIS = 0; /*lint !e923 Ok, cast in Freescale header file */
#else
    EMIOS.MCR.B.MDIS = 0; /*lint !e923 Ok, cast in Freescale header file */
#endif

    for (channel_iterator = 0; channel_iterator < PWM_NUMBER_OF_CHANNELS; channel_iterator++) {
        const Pwm_ChannelConfigurationType* channelConfig = &ConfigPtr->Channels[channel_iterator];
        Pwm_ChannelType channel = channelConfig->channel;
        volatile struct EMIOS_tag *emiosHw;
        uint8 hwUnit = 0;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
        if(channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
            emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
        } else {
            emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
            channel -= PWM_NUMBER_OF_EACH_EMIOS;
            hwUnit = 1;
        }
#else
        emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif

#if !defined(CFG_MPC5567)
        // Clear the disable bit for this channel
        emiosHw->UCDIS.R =(vuint32_t)((uint32)emiosHw->UCDIS.R & ~(1UL << (uint8) channel));
#endif
        /* @req SWS_Pwm_00007 */
        configureChannel( channelConfig );

        (void)hwUnit; // Dummy access for dont-care cases
#if (PWM_NOTIFICATION_SUPPORTED == STD_ON)
        installChannelInterrupt(hwUnit, channel);
#endif
    }
}

/* @req SWS_Pwm_10080*/
#if PWM_DE_INIT_API==STD_ON
/**
 * Deinit pwm channel
 *
 * @param Channel Numeric identifier of a PWM channel
 */
static void Pwm_DeInitChannel(Pwm_ChannelType Channel) {
    volatile struct EMIOS_tag *emiosHw;
    Pwm_ChannelType Chnl= Channel;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        Chnl = Channel - (Pwm_ChannelType) PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif

    /* Set 0% duty */
#if defined(CFG_MPC5567)
    emiosHw->CH[Chnl].CADR.R = emiosHw->CH[Chnl].CBDR.R;
#else
    emiosHw->CH[Chnl].CADR.R = 0;
#endif

#if !defined(CFG_MPC5567)
    // Set the disable bit for this channel
    emiosHw->UCDIS.R = (vuint32_t)(emiosHw->UCDIS.R | (1UL << (uint8)Chnl));
#endif

    /* @req SWS_Pwm_00012*/
    #if PWM_NOTIFICATION_SUPPORTED==STD_ON
        Pwm_DisableNotification(Chnl);
    #endif
}

/*@req SWS_Pwm_00096*/
void Pwm_DeInit(void) {
    Pwm_ChannelType channel_iterator;
    /* @req SWS_Pwm_00117*/
    VALIDATE(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_DEINIT_SERVICE_ID, PWM_E_UNINIT);

    for (channel_iterator = 0; channel_iterator < PWM_NUMBER_OF_CHANNELS; channel_iterator++) {
        Pwm_ChannelType channel = Pwm_ConfigPtr->Channels[channel_iterator].channel;
        Pwm_DeInitChannel(channel);
    }

    // Disable module
    /* @req SWS_Pwm_00011*/
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    EMIOS_0.MCR.B.MDIS = 1; /*lint !e923 Ok, cast in Freescale header file */
    EMIOS_1.MCR.B.MDIS = 1; /*lint !e923 Ok, cast in Freescale header file */
#else
    EMIOS.MCR.B.MDIS = 1; /*lint !e923 Ok, cast in Freescale header file */
#endif
    /* @req SWS_Pwm_00010*/
    Pwm_ModuleState = PWM_STATE_UNINITIALIZED;
}
#endif

/* @req SWS_Pwm_20083 */
#if PWM_SET_PERIOD_AND_DUTY_API==STD_ON
/* @req SWS_Pwm_00098 */
/* @req SWS_Pwm_00020 */
/* @req SWS_Pwm_00076 */
void Pwm_SetPeriodAndDuty(Pwm_ChannelType Channel, Pwm_PeriodType Period, uint16 DutyCycle) {

    Pwm_ChannelType localChannel = Channel;

    /* @req SWS_Pwm_00117*/
    VALIDATE(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_SETPERIODANDDUTY_SERVICE_ID, PWM_E_UNINIT);
    /* @req SWS_Pwm_00047*/
    VALIDATE(IS_VALID_CHANNEL(Channel), PWM_SETPERIODANDDUTY_SERVICE_ID, PWM_E_PARAM_CHANNEL);

    Pwm_ChannelType channel_iterator;

    for (channel_iterator = 0; channel_iterator < PWM_NUMBER_OF_CHANNELS; channel_iterator++)
    {
        if(Channel == Pwm_ConfigPtr->Channels[channel_iterator].channel){
            /* @req SWS_Pwm_00041 */
            if(Pwm_ConfigPtr->ChannelClass[channel_iterator] != PWM_VARIABLE_PERIOD){
#if PWM_DEV_ERROR_DETECT==STD_ON
                /* @req SWS_Pwm_00045 */
                /* @req SWS_Pwm_40002 */
                (void)Det_ReportError(PWM_MODULE_ID,0, PWM_SETPERIODANDDUTY_SERVICE_ID, PWM_E_PERIOD_UNCHANGEABLE);
#endif
                return;
            }
            break;
        }
    }

    /* Check that we found a valid channel */
    /* @req SWS_Pwm_00047*/
    VALIDATE(channel_iterator < PWM_NUMBER_OF_CHANNELS, PWM_SETPERIODANDDUTY_SERVICE_ID, PWM_E_PARAM_CHANNEL);

    volatile struct EMIOS_tag *emiosHw;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        localChannel = Channel - PWM_NUMBER_OF_EACH_EMIOS;
    }
    /* @req SWS_Pwm_00150 */
    //HW limitation CBDR requires value greater than 1, so Period=1 is also treated as zero
    if ((Period == 0) || (Period == 1)) {
        emiosHw->CH[localChannel].CADR.R = 0;//0% duty cycle
        return;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */

    /* @req SWS_Pwm_00150 */
    if (Period == 0) {
        emiosHw->CH[localChannel].CADR.R = emiosHw->CH[localChannel].CBDR.R;//0% duty cycle
        return;
    }

#endif

    /* @req SWS_Pwm_00019 */
    /* Timer instant for the period to restart */
    emiosHw->CH[localChannel].CBDR.R = (vuint32_t) Period;
    setDutyCycle(Channel, DutyCycle);
}
#endif


/* @req SWS_Pwm_20082 */
#if PWM_SET_DUTY_CYCLE_API==STD_ON
/* @req SWS_Pwm_00097 */
void Pwm_SetDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle)
{
    /* @req SWS_PWM_00117 */
    VALIDATE(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_SETDUTYCYCLE_SERVICE_ID, PWM_E_UNINIT);
    /* @req SWS_PWM_00047 */
    VALIDATE(IS_VALID_CHANNEL(Channel), PWM_SETDUTYCYCLE_SERVICE_ID, PWM_E_PARAM_CHANNEL);

    setDutyCycle(Channel, DutyCycle);
}
#endif

/* @req SWS_Pwm_20084 */
#if  PWM_SET_OUTPUT_TO_IDLE_API == STD_ON
/* @req SWS_Pwm_00099 */
/* @req SWS_Pwm_00119 CBDR controls the period and it is preserved for calls to Pwm_SetOutputToIdle() */
void Pwm_SetOutputToIdle(Pwm_ChannelType Channel)
{
    Pwm_ChannelType Chnl = Channel;

    /* @req SWS_Pwm_00117 */
    VALIDATE(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_SETOUTPUTTOIDLE_SERVICE_ID, PWM_E_UNINIT);
    /* @req SWS_Pwm_00047 */
    VALIDATE(IS_VALID_CHANNEL(Channel), PWM_SETOUTPUTTOIDLE_SERVICE_ID, PWM_E_PARAM_CHANNEL);

    volatile struct EMIOS_tag *emiosHw;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        Chnl = Channel - PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif

/* @req    SWS_Pwm_00021 */
    /* Set 0% duty */
#if defined(CFG_MPC5567)
    emiosHw->CH[Chnl].CADR.R = emiosHw->CH[Chnl].CBDR.R;
#else
    emiosHw->CH[Chnl].CADR.R = 0;
#endif
}
#endif

/* @req SWS_Pwm_20085*/
#if (PWM_GET_OUTPUT_STATE_API==STD_ON)

/* @req SWS_Pwm_00100*/
Pwm_OutputStateType Pwm_GetOutputState(Pwm_ChannelType Channel)
{
    Pwm_ChannelType Chnl = Channel;

    /* @req SWS_Pwm_30051 */
    /* @req SWS_Pwm_00117 */
    VALIDATE_W_RV(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_GETOUTPUTSTATE_SERVICE_ID, PWM_E_UNINIT, PWM_LOW);
    /* @req SWS_Pwm_00047 */
    VALIDATE_W_RV(IS_VALID_CHANNEL(Channel), PWM_GETOUTPUTSTATE_SERVICE_ID, PWM_E_PARAM_CHANNEL, PWM_LOW);

    const volatile struct EMIOS_tag *emiosHw;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        Chnl = Channel - PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif
    /* @req SWS_Pwm_00022 */
    return (Pwm_OutputStateType)emiosHw->CH[Chnl].CSR.B.UCOUT;
}
#endif

#if PWM_NOTIFICATION_SUPPORTED==STD_ON
/* @req SWS_Pwm_00101 */
/* @req SWS_Pwm_20112 */
/* @req SWS_Pwm_20113 */
/* @req SWS_Pwm_10115 */
/* @req SWS_Pwm_20115 */
/* @req SWS_Pwm_30115 */
void Pwm_DisableNotification(Pwm_ChannelType Channel)
{
    Pwm_ChannelType Chnl = Channel;

    /* @req SWS_Pwm_00117 */
    VALIDATE(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_DISABLENOTIFICATION_SERVICE_ID, PWM_E_UNINIT);
    /* @req SWS_Pwm_00047 */
    VALIDATE(IS_VALID_CHANNEL(Channel), PWM_DISABLENOTIFICATION_SERVICE_ID, PWM_E_PARAM_CHANNEL);

    volatile struct EMIOS_tag *emiosHw;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        Chnl = Channel - PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif

    // Disable flags on this channel
    /* @req SWS_Pwm_00023 */
    emiosHw->CH[Chnl].CCR.B.FEN = 0;
}

/* @req SWS_Pwm_00102 */
void Pwm_EnableNotification(Pwm_ChannelType Channel,Pwm_EdgeNotificationType Notification)
{
    Pwm_ChannelType Chnl = Channel;

    /* @req SWS_Pwm_00117 */
    VALIDATE(Pwm_ModuleState == PWM_STATE_INITIALIZED, PWM_ENABLENOTIFICATION_SERVICE_ID, PWM_E_UNINIT);
    /* @req SWS_Pwm_00047 */
    VALIDATE(IS_VALID_CHANNEL(Channel), PWM_ENABLENOTIFICATION_SERVICE_ID, PWM_E_PARAM_CHANNEL);

    volatile struct EMIOS_tag *emiosHw;
#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    if(Channel <= (PWM_NUMBER_OF_EACH_EMIOS-1)) {
        emiosHw = &EMIOS_0; /*lint !e923 Ok, cast in Freescale header file */
    } else {
        emiosHw = &EMIOS_1; /*lint !e923 Ok, cast in Freescale header file */
        Chnl = Channel - PWM_NUMBER_OF_EACH_EMIOS;
    }
#else
    emiosHw = &EMIOS; /*lint !e923 Ok, cast in Freescale header file */
#endif


#if defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    ChannelRuntimeStruct[Channel].NotificationState = Notification;
#else
    ChannelRuntimeStruct[Channel].NotificationState = Notification;
#endif

    /* @req SWS_Pwm_00081 */
    /* Clear flag */
    emiosHw->CH[Chnl].CSR.B.FLAG = 1;
    /* @req SWS_Pwm_00024 */
    /* Flag enable. */
    emiosHw->CH[Chnl].CCR.B.FEN = 1;
}


ISR(Pwm_Isr)
{
    // Find out which channel that triggered the interrupt by checking the global flag register
#if defined (CFG_MPC5516)
    uint32 flagmask = EMIOS.GFLAG.R;
#elif defined(CFG_MPC5567)
    uint32 flagmask = EMIOS.GFR.R;
#endif
    boolean intrptEn;
    uint8   chnlOutput;

#if defined(CFG_MPC5516) || defined(CFG_MPC5567)
    // There are 24 channels specified in the global flag register, but
    // we only listen to the first 16 as only these support OPWfM
    for (Pwm_ChannelType channel_iterator = 0; channel_iterator < PWM_NUMBER_OF_CHANNELS; channel_iterator++)
    {
        Pwm_ChannelType emios_ch = Pwm_ConfigPtr->Channels[channel_iterator].channel;

        if (flagmask & (1UL << (uint8)emios_ch))
        {
            intrptEn = EMIOS.CH[emios_ch].CCR.B.FEN != 0u ? TRUE : FALSE; /*lint !e923 Ok, cast in Freescale header file */
            if (Pwm_ConfigPtr->NotificationHandlers[channel_iterator] != NULL && intrptEn)
            {
            	/* Read Unified channel output pin */
                chnlOutput = EMIOS.CH[emios_ch].CSR.B.UCOUT; /*lint !e923 Ok, cast in Freescale header file */
                Pwm_EdgeNotificationType notification = ChannelRuntimeStruct[emios_ch].NotificationState;
                if ((notification == PWM_BOTH_EDGES) || (notification == chnlOutput))
                {
                    /* @req SWS_Pwm_00025 */
                    Pwm_ConfigPtr->NotificationHandlers[channel_iterator]();
                }
            }

            // Clear interrupt
            /* @req SWS_Pwm_00026 */
            EMIOS.CH[emios_ch].CSR.B.FLAG = 1; /*lint !e923 Ok, cast in Freescale header file  */
        }
    }
#elif defined(CFG_MPC560X) || defined(CFG_MPC5645S)
    /* Read global flag registers */
    uint32 flagmask_0 = EMIOS_0.GFR.R; /*lint !e923 Ok, cast in Freescale header file */
    uint32 flagmask_1 = EMIOS_1.GFR.R; /*lint !e923 Ok, cast in Freescale header file */

    for (Pwm_ChannelType channel_iterator = 0; channel_iterator < PWM_NUMBER_OF_CHANNELS; channel_iterator++)
    {

        Pwm_ChannelType emios_ch = Pwm_ConfigPtr->Channels[channel_iterator].channel;
        Pwm_ChannelType chnl = (emios_ch >= PWM_NUMBER_OF_EACH_EMIOS) ? (emios_ch - PWM_NUMBER_OF_EACH_EMIOS): emios_ch;

        if ( (emios_ch < PWM_NUMBER_OF_EACH_EMIOS) && ((flagmask_0 & (1UL << (uint8)emios_ch)) != 0))
        {
            intrptEn = (EMIOS_0.CH[emios_ch].CCR.B.FEN > 0u) ? TRUE : FALSE; /*lint !e923 Ok, cast in Freescale header file */
            if ((Pwm_ConfigPtr->NotificationHandlers[channel_iterator] != NULL) && intrptEn)
            {
            	/* Read Unified channel output pin */
                chnlOutput = EMIOS_0.CH[emios_ch].CSR.B.UCOUT; /*lint !e923 !e632 Ok, cast in Freescale header file */
                Pwm_EdgeNotificationType notification = ChannelRuntimeStruct[emios_ch].NotificationState;
                if ((notification == PWM_BOTH_EDGES) || (notification == chnlOutput)) /*lint !e634 !e641 Verified correctness  */
                {
                    /* @req SWS_Pwm_00025 */
                    Pwm_ConfigPtr->NotificationHandlers[channel_iterator]();
                }
            }

            // Clear interrupt
            /* @req SWS_Pwm_00026 */
            EMIOS_0.CH[emios_ch].CSR.B.FLAG = 1; /*lint !e923 Ok, cast in Freescale header file */
        }
        else if ((emios_ch >= PWM_NUMBER_OF_EACH_EMIOS) && ((flagmask_1 & (1UL << (uint8)(chnl))) != 0))
        {
            intrptEn = (EMIOS_1.CH[chnl].CCR.B.FEN > 0u) ? TRUE : FALSE; /*lint !e923 Ok, cast in Freescale header file */
            if ((Pwm_ConfigPtr->NotificationHandlers[channel_iterator] != NULL) &&
                    intrptEn) {
            	/* Read Unified channel output pin */
                chnlOutput = EMIOS_1.CH[chnl].CSR.B.UCOUT; /*lint !e923 !e632 Ok, cast in Freescale header file */
                Pwm_EdgeNotificationType notification = ChannelRuntimeStruct[emios_ch].NotificationState;
                if ((notification == PWM_BOTH_EDGES) || (notification == chnlOutput)) { /*lint !e634 !e641 Verified correctness  */
                    /* @req SWS_Pwm_00025 */
                    Pwm_ConfigPtr->NotificationHandlers[channel_iterator]();
                }
            }

            // Clear interrupt
            /* @req SWS_Pwm_00026 */
            EMIOS_1.CH[chnl].CSR.B.FLAG = 1; /*lint !e923 Ok, cast in Freescale header file */
        } else {
            /* Avoid Lint message */
        }
    }

#endif
}

#endif /* PWM_NOTIFICATION_SUPPORED == STD_ON */


/* @req SWS_Pwm_20069 */
#if ( PWM_VERSION_INFO_API == STD_ON)
/* @req SWS_Pwm_00103 */
void Pwm_GetVersionInfo( Std_VersionInfoType* versioninfo) {

    /* @req SWS_Pwm_00151 */
    VALIDATE( ( NULL != versioninfo ), PWM_GETVERSIONINFO_SERVICE_ID, PWM_E_PARAM_POINTER);

    versioninfo->vendorID = PWM_VENDOR_ID;
    versioninfo->moduleID = PWM_MODULE_ID;
    versioninfo->sw_major_version = PWM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = PWM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = PWM_SW_PATCH_VERSION;

}
#endif

