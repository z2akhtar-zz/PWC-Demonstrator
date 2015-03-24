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
/** @tagSettings DEFAULT_ARCHITECTURE=ZYNQ */

/* ----------------------------[information]----------------------------------*/
/*
 * Author: tose
 *
 * Description:
 *   Implements the Adc Driver module for the Xilinx Zynq.
 *   This version of the MCAL supports software triggered single shot conversion.
 *
 * Support:
 *   General                  Have Support
 *   -------------------------------------------
 *   ADC_DEINIT_API                    Y
 *   ADC_DEV_ERROR_DETECT              Y
 *   ADC_ENABLE_QUEUING                N
 *   ADC_ENABLE_START_STOP_GROUP_API   Y
 *   ADC_GRP_NOTIF_CAPABILITY          Y
 *   ADC_HW_TRIGGER_API                Y
 *   ADC_PRIORITY_IMPLEMENTATION       N
 *   ADC_READ_GROUP_API                Y
 *   ADC_VERSION_INFO_API              Y
*/


#include "Adc.h"
#include "zynq.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
#include "isr.h"
#include "irq_zynq.h"
#endif
#include "Adc_Internal.h"
#include "Os.h"

// General requirements implemented/inspected
/* @req SWS_Adc_00056  */
/* @req SWS_Adc_00078  */
/* @req SWS_Adc_00090  */
/* @req SWS_Adc_00091  */
/* @req SWS_Adc_00092  */
/* @req SWS_Adc_00098  */
/* @req SWS_Adc_00099  */
/* @req SWS_Adc_100  */
/* @req SWS_Adc_101  */
/* @req SWS_Adc_110  */
/* @req SWS_Adc_111  */
/* @req SWS_Adc_138  */
/* @req SWS_Adc_140  */
/* @req SWS_Adc_155  */
/* @req SWS_Adc_156  */
/* @req SWS_Adc_214  */
/* @req SWS_Adc_219  */
/* @req SWS_Adc_224  */
/* @req SWS_Adc_226  */
/* @req SWS_Adc_228  */
/* @req SWS_Adc_246  */
/* @req SWS_Adc_247  */
/* @req SWS_Adc_248  */
/* @req SWS_Adc_249  */
/* @req SWS_Adc_250  */
/* @req SWS_Adc_259  */
/* @req SWS_Adc_260  */
/* @req SWS_Adc_267  */
/* @req SWS_Adc_277  */
/* @req SWS_Adc_280  */
/* @req SWS_Adc_307  */
/* @req SWS_Adc_318  */
/* @req SWS_Adc_319  */
/* @req SWS_Adc_320  */
/* @req SWS_Adc_326  */
/* @req SWS_Adc_327  */
/* @req SWS_Adc_329  */
/* @req SWS_Adc_330  */
/* @req SWS_Adc_359  */
/* @req SWS_Adc_360  */
/* @req SWS_Adc_363  */
/* @req SWS_Adc_364  */
/* @req SWS_Adc_365  */
/* @req SWS_Adc_366  */
/* @req SWS_Adc_367  */
/* @req SWS_Adc_368  */
/* @req SWS_Adc_369  */
/* @req SWS_Adc_372  */
/* @req SWS_Adc_373  */
/* @req SWS_Adc_374  */
/* @req SWS_Adc_375  */
/* @req SWS_Adc_376  */
/* @req SWS_Adc_380  */
/* @req SWS_Adc_381  */
/* @req SWS_Adc_382  */
/* @req SWS_Adc_383  */
/* @req SWS_Adc_385  */
/* @req SWS_Adc_386  */
/* @req SWS_Adc_387  */
/* @req SWS_Adc_413  */
/* @req SWS_Adc_415  */
/* @req SWS_Adc_419  */
/* @req SWS_Adc_420  */
/* @req SWS_Adc_431  */
/* @req SWS_Adc_433  */
/* @req SWS_Adc_458  */
/* @req SWS_Adc_503  */
/* @req SWS_Adc_505  */
/* @req SWS_Adc_506  */
/* @req SWS_Adc_507  */
/* @req SWS_Adc_508  */
/* @req SWS_Adc_509  */
/* @req SWS_Adc_510  */
/* @req SWS_Adc_512  */
/* @req SWS_Adc_513  */
/* @req SWS_Adc_514  */
/* @req SWS_Adc_515  */
/* @req SWS_Adc_517  */
/* @req SWS_Adc_518  */
/* @req SWS_Adc_519  */
/* @req SWS_Adc_528  */

#define _SHIFT_PCFG_DONE_INT(_x)    ((_x)>>2)
#define _SHIFT_XADC_PRESCALE(_x)    ((_x)<<8)
#define _SHIFT_XADC_IPISR_EOS(_x)   ((_x)>>4)
#define _SHIFT_SAMPLED_DATA(_x)     ((_x)>>4)
#define _SHIFT_XADC_SR_BUSY(_x)     ((_x)>>7)
#define MASK_EVENT_DRIVEN_MODE      0x0200
#define MASK_SEQ_BITS               0xF000
#define MASK_SEQ_SINGLE_CHANNEL     0x3000
#define MASK_SEQ_SINGLE_PASS        0x1000
#define MASK_SEQ_CONTINUOUS         0x2000
#define MASK_IPISR_EOS              0x00000010
#define MASK_TWELVE_BIT             0x0FFF
#define MASK_PCFG_DONE              0x04
#define MASK_SR_BUSY                0x80
#define DISABLE_ALL_IPIER_ISR       0
#define ENABLE_IPIER_EOS_ISR        0x00000010
#define ENABLE_IPIER_EOC_ISR        0x00000020
#define ENABLE_GIER_ISR             0x80000000
#define CONFIG_0_DEFAULT_VALUE      0
#define CONFIG_1_DEFAULT_VALUE      0
#define CONFIG_2_DEFAULT_VALUE      0x00001E00
#define SEQ_REG_DEFAULT_VALUE       0
#define WAIT_FOR_BUSY_TO_CLEAR_CYCLES   100000
/* Function prototypes. */
ISR(Adc_GroupConversionComplete);
static void forceSetAdcToIdle(void);

/* Local source variables */
static Adc_StateType adcState = ADC_STATE_UNINIT;
static const Adc_ConfigType *AdcConfigPtr;
static Adc_GroupDefType *Adc_CurrentConvGroupPtr = NULL;

//List of channel addresses for conversion result when in Sequence Mode. Adc_ChannelType will be used to index this array.
static const uint32_t *channelAddressListSequenceMode[ADC_NBR_OF_CHANNELS] =
{
  NULL, //INTERNAL_XADC_CALIB
  NULL, //INVALID_1
  NULL, //INVALID_2
  NULL, //INVALID_3
  NULL, //INVALID_4
  &AXI_XADC.VCCPINT, //INTERNAL_VCCPINT
  &AXI_XADC.VCCPAUX, //INTERNAL_VCCPAUX
  &AXI_XADC.VCC_DRR, //INTERNAL_VCC_DDR
  &AXI_XADC.TEMP, //INTERNAL_TEMP
  &AXI_XADC.VCCINT, //INTERNAL_VCCINT
  &AXI_XADC.VCCAUX, //INTERNAL_VCCAUX
  &AXI_XADC.VP_VN, //VP_VN
  &AXI_XADC.VREFP, //INTERNAL_VREFP
  &AXI_XADC.VREFN, //INTERNAL_VREFN
  &AXI_XADC.VBRAM, //INTERNAL_VCCBRAM
  NULL, //INVALID_5
  &AXI_XADC.VAUX0_PN, //VAUX_0
  &AXI_XADC.VAUX1_PN, //VAUX_1
  &AXI_XADC.VAUX2_PN, //VAUX_2
  &AXI_XADC.VAUX3_PN, //VAUX_3
  &AXI_XADC.VAUX4_PN, //VAUX_4
  &AXI_XADC.VAUX5_PN, //VAUX_5
  &AXI_XADC.VAUX6_PN, //VAUX_6
  &AXI_XADC.VAUX7_PN, //VAUX_7
  &AXI_XADC.VAUX8_PN, //VAUX_8
  &AXI_XADC.VAUX9_PN, //VAUX_9
  &AXI_XADC.VAUX10_PN, //VAUX_10
  &AXI_XADC.VAUX11_PN, //VAUX_11
  &AXI_XADC.VAUX12_PN, //VAUX_12
  &AXI_XADC.VAUX13_PN, //VAUX_13
  &AXI_XADC.VAUX14_PN, //VAUX_14
  &AXI_XADC.VAUX15_PN //VAUX_15
};
//List of channel bitmasks for conversion result when in Single Channel Mode. Adc_ChannelType will be used to index this array.
static const uint16_t singleChannelModeTranslationTable[ADC_NBR_OF_CHANNELS] =
{
        0x08, // INTERNAL_XADC_CALIB
        0x09, // INVALID_1
        0x0A, // INVALID_2
        0x0B, // INVALID_3
        0x0C, // INVALID_4
        0x0D, // INTERNAL_VCCPINT
        0x0E, // INTERNAL_VCCPAUX
        0x0F, // INTERNAL_VCC_DDR
        0x00, // INTERNAL_TEMP
        0x01, // INTERNAL_VCCINT
        0x02, // INTERNAL_VCCAUX
        0x03, // VP_VN
        0x04, // INTERNAL_VREFP
        0x05, // INTERNAL_VREFN
        0x06, // INTERNAL_VCCBRAM
        0x07, // INVALID_5
        0x10, // VAUX_0
        0x11, // VAUX_1
        0x12, // VAUX_2
        0x13, // VAUX_3
        0x14, // VAUX_4
        0x15, // VAUX_5
        0x16, // VAUX_6
        0x17, // VAUX_7
        0x18, // VAUX_8
        0x19, // VAUX_9
        0x1A, // VAUX_10
        0x1B, // VAUX_11
        0x1C, // VAUX_12
        0x1D, // VAUX_13
        0x1E, // VAUX_14
        0x1F  // VAUX_15
};

/* @req SWS_Adc_00228 */
#if (ADC_DEINIT_API == STD_ON)
void Adc_DeInit (void)
{
    Adc_GroupType group;

    if (E_OK == Adc_CheckDeInit(adcState, AdcConfigPtr))
    {
        /* @req SWS_Adc_00110 */
        /* @req SWS_Adc_00111 */

        //Reset XADC hard macro + XADC IP core
        AXI_XADC.SYSMONRR = 0;
        AXI_XADC.SYSMONRR = 1;
        AXI_XADC.SYSMONRR = 0;
        AXI_XADC.SSR = 0x0000000A;

        //Disable all interrupts.
        AXI_XADC.GIER = 0;
        AXI_XADC.IPIER = DISABLE_ALL_IPIER_ISR;

        //Set config & seq registers to default
        AXI_XADC.CONFG0 = CONFIG_0_DEFAULT_VALUE;
        AXI_XADC.CONFG1 = CONFIG_1_DEFAULT_VALUE;
        AXI_XADC.CONFG2 = CONFIG_2_DEFAULT_VALUE;
        AXI_XADC.SEQ0 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ1 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ2 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ3 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ4 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ5 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ6 = SEQ_REG_DEFAULT_VALUE;
        AXI_XADC.SEQ7 = SEQ_REG_DEFAULT_VALUE;

#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
        for (group = 0; group < ADC_NBR_OF_GROUPS; group++)
        {
          AdcConfigPtr->groupConfigPtr[group].status->notifictionEnable = 0;
        }
#endif

        adcState = ADC_STATE_UNINIT;
    }
}
#endif

/* @req SWS_Adc_00365 */
/* @req SWS_Adc_00056 */
void Adc_Init (const Adc_ConfigType *ConfigPtr)
{
  Adc_GroupType group;
  uint8_t plInitDone, n, shift;
  uint32_t analogInputMode = 0, channelSettlingTime = 0;
  uint32_t lvlShiftEn = SLCR.LVL_SHFTR_EN;
  uint32_t pLProg = _SHIFT_PCFG_DONE_INT(DEVCFG_REG.INT_STS & MASK_PCFG_DONE);

  //Check if the PL unit is up and running. If not, accessing the XADC will hang the MCU.
  plInitDone = (!(lvlShiftEn == 0) || pLProg) ? 1 : 0;

  /* @req SWS_Adc_00342 */
  if (E_OK == Adc_CheckInit(adcState, ConfigPtr) && plInitDone)
  {
    /* First of all, store the location of the configuration data. */
    /* @req SWS_Adc_00342 */
    AdcConfigPtr = Adc_GlobalConfig;

#if !defined(CFG_DRIVERS_USE_CONFIG_ISRS)
    //Install interrupt
    ISR_INSTALL_ISR2( "AdcIsr", Adc_GroupConversionComplete, (IrqType)(ADC_ASSIGNED_IRQ_ID), 4, 0 );
#endif
    forceSetAdcToIdle();
    AXI_XADC.CONFG2 = _SHIFT_XADC_PRESCALE(AdcConfigPtr->hwConfigPtr->adcPrescale);
    // Enable global interrupt register.
    AXI_XADC.GIER = ENABLE_GIER_ISR;
    //Set ADC Channel Analog-Input mode and  ADC Channel Settling Time mode
    for (n = 0; n < AdcConfigPtr->nbrOfChannels; n++) {
        shift = AdcConfigPtr->channelConfigPtr[n].adcChannel;

        analogInputMode      |=  AdcConfigPtr->channelConfigPtr[n].adcChannelInputMode << shift;
        channelSettlingTime  |=  AdcConfigPtr->channelConfigPtr[n].adcChannelSettlingTime << shift;
    }
    AXI_XADC.SEQ4 = channelSettlingTime & 0x0000FFFF;
    AXI_XADC.SEQ5 = channelSettlingTime >> 16;
    AXI_XADC.SEQ6 = analogInputMode & 0x0000FFFF;
    AXI_XADC.SEQ7 = analogInputMode >> 16;

    for (group = 0; group < ADC_NBR_OF_GROUPS; group++)
    {
      /* @req SWS_Adc_00307 */
      AdcConfigPtr->groupConfigPtr[group].status->groupStatus = ADC_IDLE;
      /* @req SWS_Adc_00077 */
#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
        AdcConfigPtr->groupConfigPtr[group].status->notifictionEnable = 0;
#endif
    }

    /* Move on to INIT state. */
    adcState = ADC_STATE_INIT;
  }
}

Std_ReturnType Adc_SetupResultBuffer (Adc_GroupType group, Adc_ValueGroupType *bufferPtr)
{
  Std_ReturnType returnValue = E_NOT_OK;

  /* Check for development errors. */
  if (E_OK == Adc_CheckSetupResultBuffer (adcState, AdcConfigPtr, group, bufferPtr))
  {
      /* @req SWS_Adc_00420 */
      AdcConfigPtr->groupConfigPtr[group].status->resultBufferPtr = bufferPtr;

      returnValue = E_OK;
  }

  return returnValue;
}

/* @req SWS_Adc_00359. */
#if (ADC_READ_GROUP_API == STD_ON)
Std_ReturnType Adc_ReadGroup (Adc_GroupType group, Adc_ValueGroupType *dataBufferPtr)
{
  Std_ReturnType returnValue;
  uint16_t channel;
  Adc_ValueGroupType *tmpPointer;

  if (E_OK == Adc_CheckReadGroup (adcState, AdcConfigPtr, group))
  {
    Adc_GroupDefType *groupPtr = (Adc_GroupDefType *)&AdcConfigPtr->groupConfigPtr[group];

    returnValue = E_OK;

    if ((ADC_CONV_MODE_ONESHOT == groupPtr->conversionMode) &&
        (ADC_STREAM_COMPLETED  == groupPtr->status->groupStatus))
    {
        /* @req SWS_Adc_00330. */
        groupPtr->status->groupStatus = ADC_IDLE;
    }
    else if ((ADC_CONV_MODE_CONTINUOUS == groupPtr->conversionMode) &&
             (ADC_STREAM_BUFFER_LINEAR == groupPtr->streamBufferMode) &&
             (ADC_ACCESS_MODE_STREAMING == groupPtr->accessMode) &&
             (ADC_STREAM_COMPLETED    == groupPtr->status->groupStatus))
    {
        /* @req SWS_Adc_00330. */
        groupPtr->status->groupStatus = ADC_IDLE;
    }
    else if (ADC_CONV_MODE_CONTINUOUS == groupPtr->conversionMode &&
             ADC_STREAM_COMPLETED    == groupPtr->status->groupStatus )
    {
        /* @req SWS_Adc_00329. */
        groupPtr->status->groupStatus = ADC_BUSY;
    }
    else if (ADC_COMPLETED    == groupPtr->status->groupStatus)
    {
        /* @req SWS_Adc_00331. */
        groupPtr->status->groupStatus = ADC_BUSY;
    }

    /* Copy the result to application buffer. */
    for (channel = 0; channel < groupPtr->numberOfChannels; channel++)
    {
        if(groupPtr->status->currSampleCount > 0){
            /* Remeber that the result buffer has the following layout: CH1_SMPL1, CH1_SMPL2, CH2_SMPL1, CH2_SMPL2 */
            /* @req SWS_Adc_00075 */
            tmpPointer = groupPtr->status->currResultBufPtr;
            if( ADC_ACCESS_MODE_STREAMING == groupPtr->accessMode )
            {
                if( ADC_STREAM_BUFFER_LINEAR == groupPtr->streamBufferMode ){
                    tmpPointer--;
                }
                else if( ADC_STREAM_BUFFER_CIRCULAR == groupPtr->streamBufferMode ){
                    /* Handle buffer wrapping */
                    if( tmpPointer > groupPtr->status->resultBufferPtr ){
                        tmpPointer--;
                    }
                    else{
                        tmpPointer = groupPtr->status->resultBufferPtr + (groupPtr->streamNumSamples-1);
                    }
                }
            }
            dataBufferPtr[channel] = groupPtr->status->currResultBufPtr[channel*groupPtr->streamNumSamples];
        }else{
            /* @req SWS_Adc_00075 */
            dataBufferPtr[channel] = groupPtr->status->resultBufferPtr[channel];
        }
    }
  }
  else
  {
    /* An error have been raised from Adc_CheckReadGroup(). */
    returnValue = E_NOT_OK;
  }

  return (returnValue);
}
#endif

Adc_StreamNumSampleType Adc_GetStreamLastPointer(Adc_GroupType group, Adc_ValueGroupType** PtrToSamplePtr)
{
    Adc_StreamNumSampleType nofSample = 0;

    /* Check for development errors. */
    if (E_OK == Adc_CheckGetStreamLastPointer (adcState, AdcConfigPtr, group))
    {
        Adc_GroupDefType *groupPtr = (Adc_GroupDefType *)&AdcConfigPtr->groupConfigPtr[group];

        /* Set resultPtr to application buffer. */
        if(groupPtr->status->currSampleCount > 0){
            /* @req SWS_Adc_00214 */
            /* @req SWS_Adc_00219 */
            /* @req SWS_Adc_00387 */
            *PtrToSamplePtr = &groupPtr->status->currResultBufPtr[0];
            nofSample = groupPtr->status->currSampleCount;
        }

        if( (ADC_COMPLETED  == groupPtr->status->groupStatus) ){
            /* @req SWS_Adc_00328 */
            groupPtr->status->groupStatus = ADC_BUSY;
        }
        else if( ADC_STREAM_COMPLETED  == groupPtr->status->groupStatus )
        {
            if( (ADC_CONV_MODE_ONESHOT == groupPtr->conversionMode) &&
                    (ADC_TRIGG_SRC_SW == groupPtr->triggerSrc) &&
                    (ADC_ACCESS_MODE_SINGLE == groupPtr->accessMode)) {
                /* @req SWS_Adc_00327 */
                groupPtr->status->groupStatus = ADC_IDLE;
            }
            else if( (ADC_ACCESS_MODE_STREAMING == groupPtr->accessMode) &&
                    (ADC_STREAM_BUFFER_LINEAR == groupPtr->streamBufferMode) ){
                /* @req SWS_Adc_00327 */
                groupPtr->status->groupStatus = ADC_IDLE;
            }
            else if( ADC_CONV_MODE_CONTINUOUS == groupPtr->conversionMode ){
                /* @req SWS_Adc_00326 */
                groupPtr->status->groupStatus = ADC_BUSY;
            }
            else if( (ADC_TRIGG_SRC_HW == groupPtr->triggerSrc) &&
                     ( (ADC_ACCESS_MODE_SINGLE == groupPtr->accessMode)  ||
                       ( (ADC_ACCESS_MODE_STREAMING == groupPtr->accessMode) &&
                         (ADC_STREAM_BUFFER_CIRCULAR == groupPtr->streamBufferMode)
                       )
                     )
                   ){
                /* @req SWS_Adc_00326 */
                groupPtr->status->groupStatus = ADC_BUSY;
            }
        }
    }
    else
    {
        /* Some condition not met */
        *PtrToSamplePtr = NULL;
    }

    return nofSample;
}


ISR(Adc_GroupConversionComplete)
{
    Adc_GroupDefType *groupPtr = NULL;
    uint16_t index;
    Adc_ChannelType selectedChannel;
    vuint32_t towClearFlags;
    Adc_ValueGroupType rawValue, maskedRawValue;

    /* @req SWS_Adc_00078 */
    /* Disable interrupts */
    AXI_XADC.IPIER = DISABLE_ALL_IPIER_ISR;

    //Method of clearing interrupt flags, toggle on write.
    towClearFlags = AXI_XADC.IPISR;
    AXI_XADC.IPISR = towClearFlags;

    groupPtr = Adc_CurrentConvGroupPtr;

    if(groupPtr != NULL)
    {
        /* Copy conversion data from XADC to result buffer for the group channels. */
        for (index = 0; index < groupPtr->numberOfChannels; index++)
        {
            selectedChannel = groupPtr->channelList[index];
            /* To get the layout of the buffer right, first all sampels from one channel,
            than all samples for the next etc.*/
            rawValue = (Adc_ValueGroupType)*channelAddressListSequenceMode[selectedChannel];
            maskedRawValue = _SHIFT_SAMPLED_DATA(rawValue) & MASK_TWELVE_BIT;
            groupPtr->status->currResultBufPtr[index*groupPtr->streamNumSamples] = maskedRawValue;
        }

        if(ADC_ACCESS_MODE_SINGLE == groupPtr->accessMode )
        {
            groupPtr->status->currSampleCount = 1;
            /* @req SWS_Adc_00325 */
            groupPtr->status->groupStatus = ADC_STREAM_COMPLETED;

            if( ( groupPtr->conversionMode == ADC_CONV_MODE_CONTINUOUS) ||
                ((groupPtr->conversionMode == ADC_CONV_MODE_ONESHOT) && (ADC_TRIGG_SRC_HW == groupPtr->triggerSrc)) )
            {
                /* Turn on the end of stream/conversion interrupt again */
                if(ADC_TRIGG_SRC_HW == groupPtr->triggerSrc){
                    AXI_XADC.IPIER = ENABLE_IPIER_EOC_ISR;
                } else {
                    AXI_XADC.IPIER = ENABLE_IPIER_EOS_ISR;
                }
            }
            else
            {
                // ADC_ACCESS_MODE_SINGLE && ADC_CONV_MODE_ONESHOT && ADC_TRIGG_SRC_SW
                // Clear the sequence mode and leave the interrupts off.
                AXI_XADC.CONFG1 &= (uint32_t)~MASK_SEQ_BITS;
                forceSetAdcToIdle();
            }
        }
        else
        {
            if( ADC_STREAM_BUFFER_LINEAR == groupPtr->streamBufferMode )
            {
                groupPtr->status->currSampleCount++;
                if(groupPtr->status->currSampleCount < groupPtr->streamNumSamples)
                {
                    groupPtr->status->currResultBufPtr++;
                    /* @req SWS_Adc_00224 */
                    groupPtr->status->groupStatus = ADC_COMPLETED;

                    /* Turn on the end of stream/conversion interrupt again */
                    if(ADC_TRIGG_SRC_HW == groupPtr->triggerSrc){
                        AXI_XADC.IPIER = ENABLE_IPIER_EOC_ISR;
                    }else{
                        AXI_XADC.IPIER = ENABLE_IPIER_EOS_ISR;
                    }
                }
                else
                {
                    /* All samples completed */
                    /* @req SWS_Adc_00325 */
                    groupPtr->status->groupStatus = ADC_STREAM_COMPLETED;
                    forceSetAdcToIdle();
                }
            }
            else if(ADC_STREAM_BUFFER_CIRCULAR == groupPtr->streamBufferMode)
            {

                groupPtr->status->currResultBufPtr++;
                /* Check for buffer wrapping */              
                if( groupPtr->status->currResultBufPtr >= (groupPtr->status->resultBufferPtr + groupPtr->streamNumSamples) ){
                  groupPtr->status->currResultBufPtr = groupPtr->status->resultBufferPtr;    
                }                

                groupPtr->status->currSampleCount++;
                if(groupPtr->status->currSampleCount < groupPtr->streamNumSamples)
                {
                    /* @req SWS_Adc_00224 */
                    groupPtr->status->groupStatus = ADC_COMPLETED;
                }
                else{
                    groupPtr->status->currSampleCount = groupPtr->streamNumSamples;
                    /* All samples completed */
                    /* @req SWS_Adc_00325 */
                    groupPtr->status->groupStatus = ADC_STREAM_COMPLETED;
                }

                /* Turn on the end of stream/conversion interrupt again */
                if(ADC_TRIGG_SRC_HW == groupPtr->triggerSrc) {
                    AXI_XADC.IPIER = ENABLE_IPIER_EOC_ISR;
                } else {
                    AXI_XADC.IPIER = ENABLE_IPIER_EOS_ISR;
                }

            }
        }
        /* Call notification if enabled. */
#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
        if (groupPtr->status->notifictionEnable && groupPtr->groupCallback != NULL)
        {
            groupPtr->groupCallback();
        }
#endif
    }
}

/* @req SWS_Adc_00259 */
/* @req SWS_Adc_00260 */
#if (ADC_ENABLE_START_STOP_GROUP_API == STD_ON)
void Adc_StartGroupConversion (Adc_GroupType group)
{
  /* Run development error check. */
  if (E_OK == Adc_CheckStartGroupConversion (adcState, AdcConfigPtr, group))
  {
      uint8_t busyStatus = 1;
      uint8_t i, bitShifts;
      uint32_t configData;
      uint32_t channelSelectionReg = 0;
      uint32_t towClearFlags;

      //Check to ensure that the peripheral is not in a busy state
      for (i = 0; i < WAIT_FOR_BUSY_TO_CLEAR_CYCLES; i++) {
          busyStatus = _SHIFT_XADC_SR_BUSY(AXI_XADC.SR & MASK_SR_BUSY);
          if (busyStatus == 0) {
              break;
          }
      }

      if (busyStatus == 1) {
#if ( ADC_DEV_ERROR_DETECT == STD_ON )
          (void)Det_ReportError(ADC_MODULE_ID,0,ADC_STARTGROUPCONVERSION_ID, ADC_E_BUSY );
#endif
          return;
      }

      Adc_CurrentConvGroupPtr = (Adc_GroupDefType *)&AdcConfigPtr->groupConfigPtr[group];
      Adc_CurrentConvGroupPtr->status->currSampleCount = 0;
      /* @req SWS_Adc_00431 */
      Adc_CurrentConvGroupPtr->status->currResultBufPtr = Adc_CurrentConvGroupPtr->status->resultBufferPtr;

      //Method of clearing interrupt flags, toggle on write.
      towClearFlags = AXI_XADC.IPISR;
      AXI_XADC.IPISR = towClearFlags;

      /* Loop through all channels and setup the peripheral channel selection registers. */
      for (i = 0; i < AdcConfigPtr->groupConfigPtr[group].numberOfChannels; i++)
      {
        bitShifts = AdcConfigPtr->groupConfigPtr[group].channelList[i];
        /* Select physical channel. */
        channelSelectionReg |= (1 << bitShifts);//AdcConfigPtr->groupConfigPtr[group].channelList[i]);
      }

      AXI_XADC.SEQ0 = channelSelectionReg & 0x0000FFFF;
      AXI_XADC.SEQ1 = channelSelectionReg >> 16;

      //Set the conversion mode.
      configData = AXI_XADC.CONFG1;
      configData &= (uint32_t)~MASK_SEQ_BITS;
      if( AdcConfigPtr->groupConfigPtr[group].conversionMode == ADC_CONV_MODE_ONESHOT ){
          configData |= (uint32_t)MASK_SEQ_SINGLE_PASS;
      }
      else if( AdcConfigPtr->groupConfigPtr[group].conversionMode == ADC_CONV_MODE_CONTINUOUS ){
          configData |= (uint32_t)MASK_SEQ_CONTINUOUS;
      }
      else{
          // Default
          configData |= (uint32_t)MASK_SEQ_SINGLE_PASS;
      }

      AXI_XADC.CONFG0 &= (uint32_t)~MASK_EVENT_DRIVEN_MODE;

      //Start the conversion
      AXI_XADC.CONFG1 = configData;

      //Enable EOS interrupt.
      AXI_XADC.IPIER = (ENABLE_IPIER_EOS_ISR);

    /* Set group state to BUSY. */
    AdcConfigPtr->groupConfigPtr[group].status->groupStatus = ADC_BUSY;
  }

  /* @req SWS_Adc_00061 */
  /* @req SWS_Adc_00156 */
}

void Adc_StopGroupConversion (Adc_GroupType group)
{
  /* Run development error check. */
  if (E_OK == Adc_CheckStopGroupConversion (adcState, AdcConfigPtr, group))
  {
      /*@req SWS_Adc_00385 */
      /*@req SWS_Adc_00386 */
      forceSetAdcToIdle();

      /*@req SWS_Adc_00360 */
      AdcConfigPtr->groupConfigPtr[group].status->groupStatus = ADC_IDLE;

#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
        /* Disable group notification if enabled. */
        if(1 == AdcConfigPtr->groupConfigPtr[group].status->notifictionEnable){
            /* @req SWS_Adc_00155 */
            Adc_DisableGroupNotification (group);
        }
#endif
  }
  else
  {
    /* Error have been set within Adc_CheckStartGroupConversion(). */
  }
}
#endif

/* @req SWS_Adc_00265*/
/* @req SWS_Adc_00266*/
#if (STD_ON == ADC_HW_TRIGGER_API)
/* @req SWS_Adc_00370 */
void Adc_EnableHardwareTrigger(Adc_GroupType Group){
    uint8_t i = 0, busyStatus = 1;
    uint32_t configData;
    uint32_t towClearFlags;

    if( E_OK == Adc_CheckEnableHardwareTrigger(adcState, AdcConfigPtr, Group) )
    {

        //Check to ensure that the peripheral is not in a busy state
        for (i = 0; i < WAIT_FOR_BUSY_TO_CLEAR_CYCLES; i++) {
            busyStatus = _SHIFT_XADC_SR_BUSY(AXI_XADC.SR & MASK_SR_BUSY);
            if (busyStatus == 0) {
                break;
            }
        }

        if (busyStatus == 1) {
  #if ( ADC_DEV_ERROR_DETECT == STD_ON )
            (void)Det_ReportError(ADC_MODULE_ID,0,ADC_ENABLEHARDWARETRIGGER_ID, ADC_E_BUSY );
  #endif
            return;
        }


        Adc_CurrentConvGroupPtr = (Adc_GroupDefType *)&AdcConfigPtr->groupConfigPtr[Group];
        Adc_CurrentConvGroupPtr->status->currSampleCount = 0;
        /* @req SWS_Adc_00432 */
        Adc_CurrentConvGroupPtr->status->currResultBufPtr = Adc_CurrentConvGroupPtr->status->resultBufferPtr;

        //Method of clearing interrupt flags, toggle on write.
        towClearFlags = AXI_XADC.IPISR;
        AXI_XADC.IPISR = towClearFlags;

        // Only one channel supported
        AXI_XADC.CONFG0 |= singleChannelModeTranslationTable[Adc_CurrentConvGroupPtr->channelList[0]];

        //Set the conversion mode.
        configData = AXI_XADC.CONFG1;
        configData &= (uint32_t)~MASK_SEQ_BITS;

        /* @req SWS_Adc_00114 */
        /* @req SWS_Adc_00144 */
        AXI_XADC.CONFG0 |= MASK_EVENT_DRIVEN_MODE;

        // Single channel is the only mode for HW triggered conversion.
        configData |= (uint32_t)MASK_SEQ_SINGLE_CHANNEL;

        // Setup start of  (although we�re still waiting for trigger event)
        AXI_XADC.CONFG1 = configData;

        //Enable EOC interrupt.
        AXI_XADC.IPIER = (ENABLE_IPIER_EOC_ISR);

        /* Set group state to BUSY. */
        AdcConfigPtr->groupConfigPtr[Group].status->groupStatus = ADC_BUSY;
    }
}

/* @req SWS_Adc_00371 */
void Adc_DisableHardwareTrigger(Adc_GroupType Group){
    if( E_OK == Adc_CheckDisableHardwareTrigger(adcState, AdcConfigPtr, Group) )
    {
        /*@req SWS_Adc_00116 */
        forceSetAdcToIdle();

        /*@req SWS_Adc_00361 */
        AdcConfigPtr->groupConfigPtr[Group].status->groupStatus = ADC_IDLE;

#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
        /* Disable group notification if enabled. */
        if( AdcConfigPtr->groupConfigPtr[Group].status->notifictionEnable ){
            /* @req SWS_Adc_00157 */
            Adc_DisableGroupNotification (Group);
        }
#endif
    }
}
#endif

/* @req SWS_Adc_00100 */
#if (ADC_GRP_NOTIF_CAPABILITY == STD_ON)
void Adc_EnableGroupNotification (Adc_GroupType group)
{
    /* @req SWS_Adc_00057 */
    Adc_EnableInternalGroupNotification(adcState, AdcConfigPtr, group);
}

void Adc_DisableGroupNotification (Adc_GroupType group)
{
    /* @req SWS_Adc_00058 */
    Adc_InternalDisableGroupNotification(adcState, AdcConfigPtr, group);
}
#endif

Adc_StatusType Adc_GetGroupStatus (Adc_GroupType group)
{
    return Adc_InternalGetGroupStatus(adcState, AdcConfigPtr, group);
}

#if (STD_ON == ADC_VERSION_INFO_API)
void Adc_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    Adc_InternalGetVersionInfo(versioninfo);
}
#endif

static void forceSetAdcToIdle (void)
{
    /* Disable the EOS interrupt */
    AXI_XADC.IPIER = DISABLE_ALL_IPIER_ISR;
    //Clear the sequence mode.
    AXI_XADC.CONFG1 &= (uint32_t)~MASK_SEQ_BITS;
    AXI_XADC.CONFG0 |= MASK_EVENT_DRIVEN_MODE;
}
