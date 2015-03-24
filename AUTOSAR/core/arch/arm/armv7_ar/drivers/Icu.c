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


/** @tagSettings DEFAULT_ARCHITECTURE=ZYNQ */
/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */

/* ----------------------------[information]----------------------------------*/
/*
 * Author: lejo
 *
 * Description:
 *   Implements the Icu Driver module for the Xilinx Zynq.
 *
 * Support:
 *   General                  Have Support
 *   -------------------------------------------
 *   ICU_EDGE_DETECT_API            Y
 *   ICU_DE_INIT_API				Y
 *   ICU_TIMESTAMP_API              N
 *   ICU_EDGE_COUNT_API				N
 *   ICU_GET_INPUT_STATE_API		N
 *   ICU_SET_MODE_API				N
 *   ICU_DISABLE_WAKEUP_API 		N
 *   ICU_ENABLE_WAKEUP_API 			N
 *   ICU_GET_DUTY_CYCLE_VALUES_API	N
 *   ICU_GET_TIME_ELAPSED_API		N
 *   ICU_GET_VERSION_INFO_API		N
 *   ICU_SIGNAL_MEASUREMENT_API		N
 *   ICU_WAKEUP_FUNCTIONALITY_API	N
 *
*/

#include "Icu.h"
#include "Icu_Internal.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#include "zynq.h"
#include "Mcu.h"
#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
#include "isr.h"
#include "irq_zynq.h"
#endif
#include "Os.h"

// General requirements implemented/inspected
/** @req SWS_Icu_00100 */
/** @req SWS_Icu_00283 */
/** @req SWS_Icu_00285 */
/** @req SWS_Icu_00286 */
/** @req SWS_Icu_00286 */
/** @req SWS_Icu_00297 */
/** @req SWS_Icu_00299 */
/** @req SWS_Icu_00300 */
/** @req SWS_Icu_00309 */
/** @req SWS_Icu_00310 */
/** @req SWS_Icu_00311 */
/** @req SWS_Icu_00312 */
/** @req SWS_Icu_00316 */
/** @req SWS_Icu_00322 */
/** @req SWS_Icu_00324 */
/** @req SWS_Icu_00326 */
/** @req SWS_Icu_00328 */
/** @req SWS_Icu_00330 */
/** @req SWS_Icu_00332 */
/** @req SWS_Icu_00330 */
/** @req SWS_Icu_00346 */
/** @req SWS_Icu_00368 */
/** @req SWS_Icu_00373 */
/** @req SWS_Icu_00378 */
/** @req SWS_Icu_00044 */
/** @req SWS_Icu_00050 */
/** @req SWS_Icu_00051 */
/** @req SWS_Icu_00091 */

#if ( ICU_DEV_ERROR_DETECT == STD_ON )
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void) Det_ReportError(ICU_MODULE_ID,0,_api,_err); \
          return; \
        }

#define VALIDATE_W_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          (void) Det_ReportError(ICU_MODULE_ID,0,_api,_err); \
          return (_rv); \
        }
#else
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_W_RV(_exp,_api,_err,_rv )
#endif

const Icu_ConfigType* IcuConfigPtr = NULL;


// The MIO pins are mapped to GPIO bank 0 and 1, the EMIO pins are mapped to GPIO bank 2 and 3
#define IS_MIO_BANK(_x) ((_x) <= 1)
#define ZYNQ_GPIO_MAX_BANKS 4

/* Start pin number of the banks */
const uint8_t ZYNQ_GPIO_BANK_PINS[ZYNQ_GPIO_MAX_BANKS + 1] = { 0, 32, 54, 86, 118 };

/* Pointers to the GPIO registers in a bank */
typedef struct {
	vuint32_t *DIRM;                /* Direction Mode */
	vuint32_t *OEN;                 /* Output Enable */
	vuint32_t *INT_MASK;            /* Interrupt Mask status */
	vuint32_t *INT_EN;              /* Interrupt Enable/Unmask */
	vuint32_t *INT_DIS;             /* Interrupt Disable/Mask */
	vuint32_t *INT_STAT;            /* Interrupt Status */
	vuint32_t *INT_TYPE;            /* Interrupt Type */
	vuint32_t *INT_POLARITY;        /* Interrupt Polarity */
	vuint32_t *INT_ANY;             /* Interrupt Any edge sensitive */
} GPIO_Bank_Reg;

/* GPIO registers for all banks */
static GPIO_Bank_Reg Bank_Regs[ZYNQ_GPIO_MAX_BANKS];

#define BANK_SIZE 	0x40
#define DIRM_OFFSET 0x00000204

/* Initialize all GPIO register pointers.
 * This must be called before any access to the registers are done in this module.*/
void Init_Bank_Regs ()
{
	for (int i=0; i<ZYNQ_GPIO_MAX_BANKS;i++)
	{
		vuint32_t startoffset 		= i*BANK_SIZE + DIRM_OFFSET;
		Bank_Regs[i].DIRM 			= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset);
		Bank_Regs[i].OEN 			= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 4);
		Bank_Regs[i].INT_MASK 		= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 8);
		Bank_Regs[i].INT_EN 		= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 12);
		Bank_Regs[i].INT_DIS 		= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 16);
		Bank_Regs[i].INT_STAT 		= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 20);
		Bank_Regs[i].INT_TYPE 		= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 24);
		Bank_Regs[i].INT_POLARITY 	= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 28);
		Bank_Regs[i].INT_ANY 		= (vuint32_t*) (ZYNQ_GPIO_BASE + startoffset + 32);
	}
}

/* Retrieves the physical Channel ID from the logical Channel ID
 * The physical channels can be 0-117 (0-53: the MIO banks, 54-117: the EMIO banks in the GPIO) */
Icu_ChannelType GetPhysicalChannel (Icu_ChannelType ChannelID)
{
        return (IcuConfigPtr->Channels[ChannelID].physical_channel);
}

/* Converts the physical channel to the correct bank and local channel number.
 * physical_channel 0-31   = bank 0 (MIO)
 * physical_channel 32-53  = bank 1 (MIO)
 * physical_channel 54-85  = bank 2 (EMIO)
 * physical_channel 86-117 = bank 3 (EMIO)
 */
void GetBankAndChannel (Icu_ChannelType physical_channel, vuint32_t* bank, vuint32_t* channel)
{
	*bank = 0;
	*channel = physical_channel;

	for (vuint32_t b=0; b < ZYNQ_GPIO_MAX_BANKS; b++)
	{
		if ((*channel >= ZYNQ_GPIO_BANK_PINS[b]) &&
			(*channel < ZYNQ_GPIO_BANK_PINS[b+1]))
		{
			*bank = b;
			*channel -= ZYNQ_GPIO_BANK_PINS[b];
			break;
		}
	}
}

/* Set GPIO registers according to Zynq-7000 TRM chapter 14.2.4:
* TYPE-EDGE_RISING,  INT_TYPE - 1, INT_POLARITY - 1,  INT_ANY - 0;
* TYPE-EDGE_FALLING, INT_TYPE - 1, INT_POLARITY - 0,  INT_ANY - 0;
* TYPE-EDGE_BOTH,    INT_TYPE - 1, INT_POLARITY - NA, INT_ANY - 1;
*/
void DefineActivationEdge (Icu_ChannelType channel, Icu_ActivationType default_edge)
{
	vuint32_t bank, chan;
    vuint32_t int_type, int_pol, int_any;

	GetBankAndChannel(channel, &bank, &chan);

	if( !IS_MIO_BANK(bank) ) {
	    *(Bank_Regs[bank].DIRM) &= ~((vuint32_t) 1 << chan);/* Sets pin to input */
	}
	int_type = *(Bank_Regs[bank].INT_TYPE);
	int_pol  = *(Bank_Regs[bank].INT_POLARITY);
	int_any  = *(Bank_Regs[bank].INT_ANY);

    /** @req SWS_Icu_00011 */
    /* Default start activation edge*/

	if( default_edge == ICU_BOTH_EDGES ) {
		/* Both edges triggering */
		int_type |= (1 << chan);
		int_any |= (1 << chan);
	}
	else {
		/* Single edge triggering */
		if( default_edge == ICU_RISING_EDGE ) {
			/* Trigger on a rising edge */
			int_type |= (1 << chan);
			int_pol |= (1 << chan);
			int_any &= ~((vuint32_t)1 << chan);
		}
		else if( default_edge == ICU_FALLING_EDGE ) {
			/* Trigger on a falling edge */
			int_type |= (1 << chan);
			int_pol &= ~((vuint32_t)1 << chan);
			int_any &= ~((vuint32_t)1 << chan);
		}
	}

	*(Bank_Regs[bank].INT_TYPE) 	= int_type;
	*(Bank_Regs[bank].INT_POLARITY) = int_pol;
	*(Bank_Regs[bank].INT_ANY) 		= int_any;

}

/* Check if channel number is within range, and not GPIO pin [8:7] which are only outputs */
#define IS_VALID_CHANNEL(_x) (  ((_x) < ICU_NUMBER_OF_CHANNELS) && ((GetPhysicalChannel(_x)) <= ICU_MAX_CHANNEL-1) && ((((GetPhysicalChannel(_x)) < 7) || ((GetPhysicalChannel(_x)) > 8))))

typedef enum {
    ICU_STATE_UNINITIALIZED,
    ICU_STATE_INITIALIZED
} Icu_ModuleStateType;


static Icu_ModuleStateType Icu_ModuleState = ICU_STATE_UNINITIALIZED;
/* static Icu_ModeType Icu_ModuleMode; */ /* Not used for Zynq. */


/* Struct containing runtime settings for each channels */
typedef struct{
#if (ICU_TIMESTAMP_API == STD_ON)
    Icu_ValueType* NextTimeStampIndexPtr;
    uint16 NextTimeStampIndex;
    uint16 TimeStampBufferSize;
    uint16 NotifyInterval;
    uint16 NotificationCounter;
    boolean IsActive; /* IsActive is set to TRUE when Timestamping is executing */
#endif

#if ((ICU_DISABLE_WAKEUP_API == STD_ON) || (ICU_ENABLE_WAKEUP_API == STD_ON))
boolean IsWakeupEnabled;
#endif

#if (ICU_EDGE_DETECT_API == STD_ON)
Icu_InputStateType InputState;
#endif

#if (ICU_EDGE_DETECT_API == STD_ON)
Icu_EdgeNumberType EdgeCounter;
#endif

/* IsRunning is set to TRUE when a running operation (e.g Edge Counting or Timestamping) is executing*/
boolean IsRunning;

boolean NotificationEnabled;

}Icu_ChannelStructType;

static Icu_ChannelStructType IcuChannelRuntimeStruct[ICU_NUMBER_OF_CHANNELS];

ISR(Icu_Isr);


static void configureChannel(Icu_ChannelType channelId, const Icu_ChannelConfigurationType* channelConfig){

    Icu_ChannelType channel = channelConfig->physical_channel;
    /** @req SWS_Icu_00088 */
    Icu_ActivationType default_edge = channelConfig->default_start_edge;

    /** @req SWS_Icu_00128 */
    /** @req SWS_Icu_00129 */
    /** @req SWS_Icu_00006 */
	/* Set the direction of the specified GPIO pin as input */
	DefineActivationEdge(channel, default_edge);


     /** @req SWS_Icu_00040 */
#if (ICU_EDGE_DETECT_API == STD_ON)
    IcuChannelRuntimeStruct[channelId].InputState = ICU_IDLE;
#endif
#if (ICU_TIMESTAMP_API == STD_ON)
    IcuChannelRuntimeStruct[channelId].IsActive = FALSE;
#endif
    IcuChannelRuntimeStruct[channelId].IsRunning = FALSE;
}

#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
static void installChannelInterrupt()
{

    /* Install ISR. All channels interrupts is combined to IRQ ID#52 */
    ISR_INSTALL_ISR2("IcuIsr", Icu_Isr, (IrqType)(IRQ_GPIO),ICU_ISR_PRIORITY, 0);

}
#endif

/** @req SWS_Icu_00191 */
/** @req SWS_Icu_00138 */
/** @req SWS_Icu_00298 */
void Icu_Init(const Icu_ConfigType* ConfigPtr) {

    /** @req SWS_Icu_00001 */
    /** @req SWS_Icu_00023 */
    /** @req SWS_Icu_00148 */
    VALIDATE( ( NULL != ConfigPtr ), ICU_INIT_ID, ICU_E_PARAM_CONFIG );

    /** @req SWS_Icu_00271 */
    /** @req SWS_Icu_00220 */
	/** @req SWS_Icu_00221 */
    if( Icu_ModuleState == ICU_STATE_INITIALIZED ) {
#if ICU_DEV_ERROR_DETECT==STD_ON
        (void) Det_ReportError(ICU_MODULE_ID,0,ICU_INIT_ID,ICU_E_ALREADY_INITIALIZED);
#endif
        return;
    }

    Icu_ModuleState = ICU_STATE_INITIALIZED;

    IcuConfigPtr = ConfigPtr;
    Init_Bank_Regs();

    Icu_ChannelType channel_iterator;

    for (channel_iterator = 0; channel_iterator < ICU_NUMBER_OF_CHANNELS; channel_iterator++) {
        const Icu_ChannelConfigurationType* channelConfig = &ConfigPtr->Channels[channel_iterator];

        configureChannel( channel_iterator, channelConfig );

        /** @req SWS_Icu_00061 */
        Icu_DisableNotification(channel_iterator);

#if (ICU_DISABLE_WAKEUP_API == STD_ON)
        /** @req SWS_Icu_00121 */
        Icu_DisableWakeup(channel_iterator);
#endif

#if (ICU_SET_MODE_API == STD_ON)
        /** @req SWS_Icu_00060 */
        Icu_SetMode( ICU_MODE_NORMAL );
#endif
 /*        Icu_ModuleMode = ICU_MODE_NORMAL; */ /* Not used for Zynq */

#if (ICU_TIMESTAMP_API == STD_ON)
        Icu_ChannelType active_iterator;

        for (active_iterator = 0; active_iterator < ICU_NUMBER_OF_CHANNELS; active_iterator++) {
            IcuChannelRuntimeStruct[active_iterator].IsActive = FALSE;
        }
#endif

    }
#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
    installChannelInterrupt();
#endif

}


/** @req SWS_Icu_00092 */
/** @req SWS_Icu_00301 */
#if (ICU_DE_INIT_API == STD_ON)
void DeInitChannel(Icu_ChannelType Channel) {

    vuint32_t bank, chan;

    Init_Bank_Regs();

    /** @req SWS_Icu_00036 */
    /* Set registers to Power on reset values */

	Icu_ChannelType physical_channel = GetPhysicalChannel(Channel);
	GetBankAndChannel(physical_channel, &bank, &chan);

	/* Disable interrupt for the given Channel */
    /* Set activation registers to 0 */
	*(Bank_Regs[bank].INT_DIS) 		|= (1 << chan);
	*(Bank_Regs[bank].INT_EN) 		&= ~((vuint32_t)1 << chan);
	*(Bank_Regs[bank].INT_TYPE) 	&= (1 << chan);
	*(Bank_Regs[bank].INT_POLARITY) &= (1 << chan);
	*(Bank_Regs[bank].INT_ANY) 		&= (1 << chan);


    /* @req SWS_Icu_0037 */
    Icu_DisableNotification(Channel);

}

/* De-initialize the ICU module */
/** @req SWS_Icu_00193 */
void Icu_DeInit( void ) {

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_DEINIT_ID, ICU_E_UNINIT);

    Icu_ChannelType channel_iterator;

    /** @req SWS_Icu_00152 */
    /* Check if any channel is currently active.*/
    for (channel_iterator = 0; channel_iterator < ICU_NUMBER_OF_CHANNELS; channel_iterator++) {
    	if (IcuChannelRuntimeStruct[channel_iterator].IsRunning)
    		return;
    }


    for (channel_iterator = 0; channel_iterator < ICU_NUMBER_OF_CHANNELS; channel_iterator++) {
        DeInitChannel(channel_iterator);
    }

    /* Disable module */
    Icu_ModuleState = ICU_STATE_UNINITIALIZED;

}
#endif


/** @req SWS_Icu_00095 */
/** @req SWS_Icu_00303 */
#if (ICU_SET_MODE_API == STD_ON)
/* Sets the ICU mode */
/** @req SWS_Icu_00194 */
void Icu_SetMode( Icu_ModeType Mode ){

    /** @req SWS_Icu_00125 */
    VALIDATE((Mode == ICU_MODE_NORMAL || Mode == ICU_MODE_SLEEP), ICU_SETMODE_ID, ICU_E_PARAM_MODE);

    /* NOT IMPLEMENTED: Decide what registers are affected. Not supported for Zynq. */
}
#endif


/** @req SWS_Icu_00096 */
/** @req SWS_Icu_00306 */
#if (ICU_DISABLE_WAKEUP_API == STD_ON)
/* Disables the wakeup capability of a single ICU channel */
/** @req SWS_Icu_00195 */
void Icu_DisableWakeup( Icu_ChannelType Channel ){

	/** @req ICU268 */
    /** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_DISABLEWAKEUP_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00024 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_DISABLEWAKEUP_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00059 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].wakeup_capability), ICU_DISABLEWAKEUP_ID, ICU_E_PARAM_CHANNEL);

    /* NOT IMPLEMENTED: Disable wakeup in channel hardware */

    IcuChannelRuntimeStruct[Channel].IsWakeupEnabled = FALSE;

}
#endif


/** @req SWS_Icu_00097 */
/** @req SWS_Icu_00308 */
#if (ICU_ENABLE_WAKEUP_API == STD_ON)
/* (Re-)enables the wakeup capability of the given ICU channel */
/** @req SWS_Icu_00196 */
void Icu_EnableWakeup( Icu_ChannelType Channel ){
	/** @req ICU268 */
    /** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_ENABLEWAKEUP_ID, ICU_E_UNINIT);

	/** @req SWS_Icu_00155 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_ENABLEWAKEUP_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00156 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].wakeup_capability), ICU_ENABLEWAKEUP_ID, ICU_E_PARAM_CHANNEL);

    /* NOT IMPLEMENTED: Enable wakeup in channel hardware */

    IcuChannelRuntimeStruct[Channel].IsWakeupEnabled = TRUE;

}
#endif


/** @req SWS_Icu_00360 */
/** @req SWS_Icu_00362 */
#if (ICU_WAKEUP_FUNCTIONALITY_API == STD_ON && ICU_REPORT_WAKEUP_SOURCE == STD_ON)
/* Checks if a wakeup capable ICU channel is the source for a wakeup event
 * and calls the ECU state manager service EcuM_SetWakeupEvent
 * in case of a valid ICU channel wakeup event. */
/** @req SWS_Icu_00358 */
void Icu_CheckWakeup( EcuM_WakeupSourceType WakeupSource ){

    /** @req SWS_Icu_00363 */
	/** @req ICU268 */
    /** @req SWS_Icu_00022 */

    VALIDATE((Icu_ModuleState == ICU_STATE_INITIALIZED), ICU_CHECKWAKEUP_ID, ICU_E_UNINIT);

    /* NOT IMPLEMENTED:  Not supported for Zynq. */
}
#endif


/* Sets the activation-edge for the given channel */
/** @req SWS_Icu_00197 */
void Icu_SetActivationCondition( Icu_ChannelType Channel, Icu_ActivationType Activation ){


	/** @req ICU268 */
    /** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_SETACTIVATIONCONDITION_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00159 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_SETACTIVATIONCONDITION_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00090 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode != ICU_MODE_SIGNAL_MEASUREMENT), ICU_SETACTIVATIONCONDITION_ID, ICU_E_PARAM_CHANNEL);
    /** @req SWS_Icu_00043 */
    VALIDATE((( Activation == ICU_RISING_EDGE ) || ( Activation == ICU_FALLING_EDGE ) || ( Activation == ICU_BOTH_EDGES )), ICU_SETACTIVATIONCONDITION_ID, ICU_E_PARAM_ACTIVATION);

    Icu_ChannelType physical_channel = GetPhysicalChannel(Channel);

    /** @req SWS_Icu_00011 */
    /** @req SWS_Icu_00366 */
    DefineActivationEdge(physical_channel, Activation);

    /** @req SWS_Icu_00139 */
#if (ICU_EDGE_DETECT_API == STD_ON)
    IcuChannelRuntimeStruct[Channel].InputState = ICU_IDLE;
#endif

}


/* Disables the notification of a channel */
/** @req SWS_Icu_00198 */
void Icu_DisableNotification( Icu_ChannelType Channel ){

	/** @req ICU268 */
    /** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_DISABLENOTIFICATION_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00160 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_DISABLENOTIFICATION_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00009 */
    /** @req SWS_Icu_00042 */
    IcuChannelRuntimeStruct[Channel].NotificationEnabled = FALSE;

}


/* Enables the notification on the given channel */
/** @req SWS_Icu_00199 */
void Icu_EnableNotification( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_ENABLENOTIFICATION_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00161 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_ENABLENOTIFICATION_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00010 */
    IcuChannelRuntimeStruct[Channel].NotificationEnabled = TRUE;
}



/** @req SWS_Icu_00122 */
/** @req SWS_Icu_00315 */
#if (ICU_GET_INPUT_STATE_API == STD_ON)
/* Returns the status of the ICU input */
/** @req SWS_Icu_00200 */
Icu_InputStateType Icu_GetInputState( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE_W_RV(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_GETINPUTSTATE_ID, ICU_E_UNINIT, ICU_IDLE);

    /** @req SWS_Icu_00162 */
    /** @req SWS_Icu_00049 */
    VALIDATE_W_RV(IS_VALID_CHANNEL(Channel), ICU_GETINPUTSTATE_ID, ICU_E_PARAM_CHANNEL, ICU_IDLE);

    /** @req SWS_Icu_00030 */
    /** @req SWS_Icu_00162 */
    VALIDATE_W_RV((( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_SIGNAL_EDGE_DETECT) ||
                   ( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_SIGNAL_MEASUREMENT)) ,
                     ICU_GETINPUTSTATE_ID, ICU_E_PARAM_CHANNEL, ICU_IDLE);

    /* Return the current input status for the channel */
     Icu_InputStateType CurrentState = IcuChannelRuntimeStruct[Channel].InputState;

    /** @req SWS_Icu_00032 */
    if (CurrentState == ICU_ACTIVE)
        IcuChannelRuntimeStruct[Channel].InputState = ICU_IDLE;

    if (IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_SIGNAL_MEASUREMENT)
    /** @req SWS_Icu_00314*/
    /* NOT IMPLEMENTED: Check if measurement is completed before returning ICU_ACTIVE */

    else
        /** @req SWS_Icu_00313 */
        /** @req SWS_Icu_00031 */
        return (CurrentState);
}
#endif



#if (ICU_EDGE_COUNT_API == STD_ON)
/* Icu_EdgeCountingISR is called by the Icu_Isr when an edgege count interupt is raised.
 * It increases the edge counter value for the channel by one.
 */
void Icu_EdgeCountingISR(Icu_ChannelType Channel)
{
     /* Increase Edge Counter */
    IcuChannelRuntimeStruct[Channel].EdgeCounter++;
}

#endif



/** @req SWS_Icu_00098 */
/** @req SWS_Icu_00321 */
/** @req SWS_Icu_00099 */
/** @req SWS_Icu_00323 */
/** @req SWS_Icu_00100 */
/** @req SWS_Icu_00325 */
#if (ICU_TIMESTAMP_API == STD_ON)

/* Icu_TimestampISR is called by the Icu_Isr when a timestamp interupt is raised.
 * It captures the timestamp value into the buffer provided in the call to Icu_StartTimestamp().
 */
void Icu_TimestampISR(Icu_ChannelType Channel)
{
	   /* NOT IMPLEMENTED:  Not supported for Zynq. */

}


/* Initialize Timestamping runtime properties */
void Init_Timestamping(Icu_ChannelType Channel, Icu_ValueType* StartPtr, uint16 Size, uint16 NotifyInterval)
{
    IcuChannelRuntimeStruct[Channel].NextTimeStampIndexPtr = StartPtr;
    IcuChannelRuntimeStruct[Channel].TimeStampBufferSize = Size;
    IcuChannelRuntimeStruct[Channel].NextTimeStampIndex = 0;
    IcuChannelRuntimeStruct[Channel].NotifyInterval = NotifyInterval;
    IcuChannelRuntimeStruct[Channel].NotificationCounter = 0;
}


/* Reset Timestamping runtime properties */
void Clear_TimeStamping(Icu_ChannelType Channel)
{
    IcuChannelRuntimeStruct[Channel].NextTimeStampIndexPtr = NULL;
    IcuChannelRuntimeStruct[Channel].TimeStampBufferSize = 0;
    IcuChannelRuntimeStruct[Channel].NextTimeStampIndex = 0;
    IcuChannelRuntimeStruct[Channel].NotifyInterval = 0;
    IcuChannelRuntimeStruct[Channel].NotificationCounter = 0;
}


/* Starts the capturing of timer values on the edges*/
/** @req SWS_Icu_00201 */
void Icu_StartTimestamp( Icu_ChannelType Channel, Icu_ValueType* BufferPtr, uint16 BufferSize, uint16 NotifyInterval ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_STARTTIMESTAMP_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00163 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_STARTTIMESTAMP_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00120 */
    VALIDATE((BufferPtr != NULL), ICU_STARTTIMESTAMP_ID, ICU_E_PARAM_BUFFER_PTR);

    /** @req SWS_Icu_00108 */
     VALIDATE((BufferSize > 0), ICU_STARTTIMESTAMP_ID, ICU_E_PARAM_BUFFER_SIZE);

    /** @req SWS_Icu_00354 */
    VALIDATE(((IcuConfigPtr->Channels[Channel].notification != NULL) && (NotifyInterval > 0)), ICU_STARTTIMESTAMP_ID, ICU_E_PARAM_NOTIFY_INTERVAL);

    /** @req SWS_Icu_00066 */
    /** @req SWS_Icu_00163 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_TIMESTAMP), ICU_STARTTIMESTAMP_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00317 */
    Init_Timestamping(Channel, BufferPtr, BufferSize, NotifyInterval);

    /* Enable Interrupt for the channels */

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

    IcuChannelRuntimeStruct[Channel].IsRunning = TRUE;
    IcuChannelRuntimeStruct[Channel].IsActive = TRUE;

}


/* Stops the timestamp measurement of the given channel */
/** @req SWS_Icu_00202 */
void Icu_StopTimestamp( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_STOPTIMESTAMP_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00164 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_STOPTIMESTAMP_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00165 */
    /** @req SWS_Icu_00164 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_TIMESTAMP), ICU_STOPTIMESTAMP_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00166 */
    VALIDATE((IcuChannelRuntimeStruct[Channel].IsActive), ICU_STOPTIMESTAMP_ID, ICU_E_NOT_STARTED);

    /* Disable the interrupt.  */

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

    Clear_TimeStamping(Channel);

    IcuChannelRuntimeStruct[Channel].IsRunning = FALSE;
    IcuChannelRuntimeStruct[Channel].IsActive = FALSE;

}


/* Reads the timestamp index of the given channel */
/** @req SWS_Icu_00203 */
Icu_IndexType Icu_GetTimestampIndex( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE_W_RV(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_GETTIMESTAMPINDEX_ID, ICU_E_UNINIT, 0);

    /** @req SWS_Icu_00169 */
    /** @req SWS_Icu_00107 */
    VALIDATE_W_RV(IS_VALID_CHANNEL(Channel), ICU_GETTIMESTAMPINDEX_ID, ICU_E_PARAM_CHANNEL, 0);

    /** @req SWS_Icu_00170 */
    VALIDATE_W_RV(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_TIMESTAMP), ICU_GETTIMESTAMPINDEX_ID, ICU_E_PARAM_CHANNEL,0);

    /** @req SWS_Icu_00135 */
    if (!(IcuChannelRuntimeStruct[Channel].IsActive)) return (0);

    // return next timestamp index of channel_iterator to be written
    return ((IcuChannelRuntimeStruct[Channel].NextTimeStampIndex));
}

#endif /* #if(ICU_TIMESTAMP_API == STD_ON) */


/** @req SWS_Icu_00101 */
/** @req SWS_Icu_00327 */
/** @req SWS_Icu_00102 */
/** @req SWS_Icu_00329 */
/** @req SWS_Icu_00103 */
/** @req SWS_Icu_00331 */
/** @req SWS_Icu_00104 */
/** @req SWS_Icu_00333 */
#if (ICU_EDGE_COUNT_API == STD_ON)
/* Resets the value of the counted edges to zero */
/** @req SWS_Icu_00204 */
void Icu_ResetEdgeCount( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_RESETEDGECOUNT_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00171 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_RESETEDGECOUNT_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00171 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_EDGE_COUNTER), ICU_RESETEDGECOUNT_ID, ICU_E_PARAM_CHANNEL);

    IcuChannelRuntimeStruct[Channel].EdgeCounter = 0;

}



/* Enables the counting of edges of the given channel */
/** @req SWS_Icu_00205 */
void Icu_EnableEdgeCount( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_ENABLEEDGECOUNT_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00172 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_ENABLEEDGECOUNT_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00074 */
    /** @req SWS_Icu_00172 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_EDGE_COUNTER), ICU_ENABLEEDGECOUNT_ID, ICU_E_PARAM_CHANNEL);

    Icu_ChannelType emios_ch = GetPhysicalChannel(Channel);

    /* Enable Interrupt for the channels */

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

    IcuChannelRuntimeStruct[Channel].IsRunning = TRUE;
}



/* Disables the counting of edges of the given channel */
/** @req SWS_Icu_00206 */
void Icu_DisableEdgeCount( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_DISABLEEDGECOUNT_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00173 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_DISABLEEDGECOUNT_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00173 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_EDGE_COUNTER), ICU_DISABLEEDGECOUNT_ID, ICU_E_PARAM_CHANNEL);

    /* Disable Interrupt for the channels */

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

    IcuChannelRuntimeStruct[Channel].IsRunning = FALSE;

}


/* Reads the number of counted edges since last call of Icu_ResetEdgeCount */
/** @req SWS_Icu_00207 */
Icu_EdgeNumberType Icu_GetEdgeNumbers( Icu_ChannelType Channel ){

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE_W_RV(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_GETEDGENUMBERS_ID, ICU_E_UNINIT, 0);

    /** @req SWS_Icu_00174 */
    /** @req SWS_Icu_00175 */
    VALIDATE_W_RV(IS_VALID_CHANNEL(Channel), ICU_GETEDGENUMBERS_ID, ICU_E_PARAM_CHANNEL, 0);

    /** @req SWS_Icu_00174 */
    /** @req SWS_Icu_00175 */
    VALIDATE_W_RV(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_EDGE_COUNTER), ICU_GETEDGENUMBERS_ID, ICU_E_PARAM_CHANNEL,0);

    // return number of counted edges for Channel
    return (IcuChannelRuntimeStruct[Channel].EdgeCounter);
}
#endif


/** @req SWS_Icu_00369 */
/** @req SWS_Icu_00370 */
/** @req SWS_Icu_00374 */
/** @req SWS_Icu_00375 */
#if (ICU_EDGE_DETECT_API == STD_ON)
/* (Re-)Enables the detection of edges of the given channel */
/** @req SWS_Icu_00364 */
void Icu_EnableEdgeDetection( Icu_ChannelType Channel ){

	vuint32_t bank, chan;

    /** @req SWS_Icu_00371 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_ENABLEEDGEDETECTION_ID, ICU_E_PARAM_CHANNEL);


    /** @req SWS_Icu_00371 */
    /** @req SWS_Icu_00367 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_SIGNAL_EDGE_DETECT), ICU_ENABLEEDGEDETECTION_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00365 */
    /* Enable interrupt for the given Channel */
    Icu_ChannelType physical_channel = GetPhysicalChannel(Channel);


    GetBankAndChannel(physical_channel, &bank, &chan);

    // Clear interrupt
    *(Bank_Regs[bank].INT_STAT) |= (1 << chan);
    *(Bank_Regs[bank].INT_EN) |= (1 << chan);
    *(Bank_Regs[bank].INT_DIS) &= ~((vuint32_t)1 << chan);


}


/* Disables the detection of edges of the given channel */
/** @req SWS_Icu_00377 */
void Icu_DisableEdgeDetection( Icu_ChannelType Channel ){

	vuint32_t bank, chan;

	/** @req ICU268 */
	/** @req SWS_Icu_00022 */
    VALIDATE(Icu_ModuleState == ICU_STATE_INITIALIZED, ICU_DISABLEEDGEDETECTION_ID, ICU_E_UNINIT);

    /** @req SWS_Icu_00376 */
    VALIDATE(IS_VALID_CHANNEL(Channel), ICU_DISABLEEDGEDETECTION_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00376 */
    VALIDATE(( IcuConfigPtr->Channels[Channel].measurement_mode == ICU_MODE_SIGNAL_EDGE_DETECT), ICU_DISABLEEDGEDETECTION_ID, ICU_E_PARAM_CHANNEL);

    /** @req SWS_Icu_00372 */
    /* Disable interrupt for the given Channel */
    Icu_ChannelType physical_channel = GetPhysicalChannel(Channel);
    GetBankAndChannel(physical_channel, &bank, &chan);

    // Clear interrupt
    *(Bank_Regs[bank].INT_STAT) |= (1 << chan);
    *(Bank_Regs[bank].INT_EN) &= ~((vuint32_t)1 << chan);
    *(Bank_Regs[bank].INT_DIS) |= (1 << chan);

}
#endif


#if (ICU_SIGNAL_MEASUREMENT_API == STD_ON)
/* Starts the measurement of signals */
/** @req SWS_Icu_00208 */
void Icu_StartSignalMeasurement( Icu_ChannelType Channel ){

    /* NOT IMPLEMENTED:  Not supported for Zynq. */
}


/* Stops the measurement of signals of the given channel */
/** @req SWS_Icu_00209 */
void Icu_StopSignalMeasurement( Icu_ChannelType Channel ){

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

}
#endif

/** @req SWS_Icu_00105 */
/** @req SWS_Icu_00341 */
#if (ICU_GET_TIME_ELAPSED_API == STD_ON)
/* Reads the elapsed Signal Low Time for the given channel */
/** @req SWS_Icu_00210 */
Icu_ValueType Icu_GetTimeElapsed( Icu_ChannelType Channel ){

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

}
#endif

/** @req SWS_Icu_00106 */
/** @req SWS_Icu_00345 */
#if (ICU_GET_DUTY_CYCLE_VALUES_API == STD_ON)
/* Reads the coherent active time and period time for the given ICU Channel */
/** @req SWS_Icu_00211 */
void Icu_GetDutyCycleValues( Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues ){

    /* NOT IMPLEMENTED:  Not supported for Zynq. */

}
#endif


ISR(Icu_Isr)
{

    for (Icu_ChannelType channel_iterator = 0; channel_iterator < ICU_NUMBER_OF_CHANNELS; channel_iterator++)
    {
    	vuint32_t bank, chan;
    	vuint32_t intstatus, enabled;

        Icu_ChannelType gpio_ch = IcuConfigPtr->Channels[channel_iterator].physical_channel;
        GetBankAndChannel(gpio_ch, &bank, &chan);

    	intstatus = *(Bank_Regs[bank].INT_STAT);
    	enabled  = *(Bank_Regs[bank].INT_MASK);

        if ((intstatus & (1 << chan)) && !(enabled & (1 << chan)))
        {

#if (ICU_TIMESTAMP_API == STD_ON)
            /** @req SWS_Icu_00218 */
            if (IcuConfigPtr->Channels[channel_iterator].measurement_mode == ICU_MODE_TIMESTAMP)
            {
                // Call TimeStamp ISR to save timestamp value into buffer
               Icu_TimestampISR (channel_iterator);

               /** @req SWS_Icu_00318 */
               /** @req SWS_Icu_00319 */
               /** @req SWS_Icu_00320 */
               /** @req SWS_Icu_00134 */
               /** @req SWS_Icu_00216 */
               /** @req SWS_Icu_00217 */
               /** @req SWS_Icu_00042 */

               if (IcuChannelRuntimeStruct[channel_iterator].NotificationEnabled &&
                   (++IcuChannelRuntimeStruct[channel_iterator].NotificationCounter >= IcuChannelRuntimeStruct[channel_iterator].NotifyInterval))
               {
                   /** @req SWS_Icu_00119 */
                   /** @req SWS_Icu_00215 */
                   // Call configured notification function if defined
                   if (IcuConfigPtr->Channels[channel_iterator].notification != NULL)
                       IcuConfigPtr->Channels[channel_iterator].notification();
               }
            }
            else
#endif

#if (ICU_EDGE_DETECT_API == STD_ON)
            if (IcuConfigPtr->Channels[channel_iterator].measurement_mode == ICU_MODE_SIGNAL_EDGE_DETECT)
            {
                IcuChannelRuntimeStruct[channel_iterator].InputState = ICU_ACTIVE;
                /** @req SWS_Icu_00119 */
                /** @req SWS_Icu_00021 */
                /** @req SWS_Icu_00214 */
                /** @req SWS_Icu_00042 */
                if (IcuChannelRuntimeStruct[channel_iterator].NotificationEnabled &&
                   (IcuConfigPtr->Channels[channel_iterator].notification != NULL))
                {
                    IcuConfigPtr->Channels[channel_iterator].notification();
                }
            }
#endif

#if (ICU_EDGE_COUNT_API == STD_ON)
            if (IcuConfigPtr->Channels[channel_iterator].measurement_mode == ICU_MODE_EDGE_COUNTER)
            {
                IcuChannelRuntimeStruct[channel_iterator].InputState = ICU_ACTIVE;

                /* Increase the Edge Counter */
                IcuChannelRuntimeStruct[channel_iterator].EdgeCounter++;
            }
#endif

            /** @req SWS_Icu_00119 */
            // Clear interrupt
            *(Bank_Regs[bank].INT_STAT) |= (1 << chan);

        }
    }

}


#if (ICU_GET_VERSION_INFO_API)
/* returns the version information of this module */
/** @req SWS_Icu_00212 */
void Icu_GetVersionInfo( Std_VersionInfoType* versioninfo ){

    /* @req SWS_Icu_00356 */
    VALIDATE( ( NULL != versioninfo ), ICU_GETVERSIONINFO_ID, ICU_E_PARAM_VINFO);

    versioninfo->vendorID = ICU_VENDOR_ID;
    versioninfo->moduleID = ICU_MODULE_ID;
    versioninfo->sw_major_version = ICU_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ICU_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ICU_SW_PATCH_VERSION;

}
#endif

