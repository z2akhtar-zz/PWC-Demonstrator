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

/** @tagSettings DEFAULT_ARCHITECTURE=RH850 */
/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */

#ifndef GPT_INTERNAL_H_
#define GPT_INTERNAL_H_

#define GPT_MAXIMUM_CHANNELS        4u
#define IS_TARGET_VALID(_c)      /*lint -save -e685 */  ((_c != 0) && (_c <= 65535)) /*lint -restore */

#define GET_CONFIG(ch)			(*Gpt_Global.config)[ch]

typedef enum {
    GPT_STATE_STOPPED = 0,
    GPT_STATE_RUNNING,
    GPT_STATE_EXPIRED,
    GPT_STATE_INITIALIZED
} Gpt_ChannelStateType;

/**
 * Type that holds all global data for Gpt
 */
typedef struct {
    /* Set if Gpt_Init() have been called */
    boolean initRun;

    /* Gpt driver mode */
    Gpt_ModeType mode;
    /* Our config */
    const Gpt_ConfigType (*config)[GPT_CHANNEL_CNT];

#if ( GPT_WAKEUP_FUNCTIONALITY_API == STD_ON )
    uint8 wakeupEnabled;
#endif

    /* Maps the a physical channel id to configured channel index */
    uint8 channelMap[GPT_MAXIMUM_CHANNELS];
    Gpt_ChannelStateType Gpt_ChannelState[GPT_CHANNEL_CNT];
    Gpt_ValueType Gpt_ChannelTargetValue[GPT_CHANNEL_CNT];
    boolean Gpt_NotifEnable[GPT_CHANNEL_CNT];
    boolean Gpt_WakUpEnable[GPT_CHANNEL_CNT];

} Gpt_GlobalType;

/*lint -esym(9003,Gpt_Global) */
extern Gpt_GlobalType Gpt_Global;

typedef uint8 Gpt_Hw_ChannelType;

static inline Gpt_Hw_ChannelType HwChannelToChannel(Gpt_ChannelType ch) {
    return Gpt_Global.channelMap[ch];
}
static inline Gpt_Hw_ChannelType ChannelToHwChannel(Gpt_ChannelType ch) {
    return GET_CONFIG(ch).GptChannelId;
}

void Gpt_Hw_Init(const Gpt_ConfigType* ConfigPtr);
void Gpt_Hw_DeInit(void);
Gpt_ValueType Gpt_Hw_GetTimeElapsed(Gpt_Hw_ChannelType Channel);
Gpt_ValueType Gpt_Hw_GetTimeRemaining(Gpt_Hw_ChannelType Channel);
void Gpt_Hw_StartTimer(Gpt_Hw_ChannelType Channel, Gpt_ValueType Value);
void Gpt_Hw_StopTimer(Gpt_Hw_ChannelType Channel);
void Gpt_Hw_EnableNotification(Gpt_Hw_ChannelType Channel);
void Gpt_Hw_DisableNotification(Gpt_Hw_ChannelType Channel);
void Gpt_Hw_SetMode(Gpt_ModeType Mode);
void Gpt_Hw_DisableWakeup(Gpt_Hw_ChannelType Channel);
void Gpt_Hw_EnableWakeup(Gpt_Hw_ChannelType Channel);

#endif /* GPT_INTERNAL_H_ */
