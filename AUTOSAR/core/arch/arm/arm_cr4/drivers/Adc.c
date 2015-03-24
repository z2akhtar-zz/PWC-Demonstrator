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


#include "Adc.h"
#include "Adc_TMS570.h" /* Use Texas Instrument header-file*/
#include "Adc_Internal.h"

#if defined(USE_DET)
#include "Det.h"
#endif
#include "isr.h"

static Adc_StateType adcState = ADC_STATE_UNINIT;

static const Adc_ConfigType *AdcConfigPtr;
#if (STD_ON == ADC_GRP_NOTIF_CAPABILITY)
static void ADCConversionComplete_Isr(unsigned int adc);
ISR(ADC1Grp1ConversionComplete_Isr);
ISR(ADC2Grp2ConversionComplete_Isr);
#endif

static void populateGroupMaps(void);
static unsigned int getHw(unsigned int group);
static unsigned int getGroupId(unsigned int group);

static int gm[ADC_ARC_NR_OF_GROUPS];
static int groupIdMap[ADC_ARC_NR_OF_GROUPS];
typedef struct {
	Adc_StatusType status;
	const Adc_GroupDefType* activeGroup;
} CoreStateType;
CoreStateType CoreStates[2];


void Adc_Init (const Adc_ConfigType *ConfigPtr)
{
	//validate config pointer

	//Store ADC ConfigPtr
	AdcConfigPtr = ConfigPtr;

	//Setup group->HW MAP
	populateGroupMaps();

	for (int i = 0; i < ADC_ARC_CTRL_CONFIG_CNT; i++) {
		adcBASE_t* adcReg = (ConfigPtr[i].hwConfigPtr->hwUnitId == 0) ? adcREG1 : adcREG2;
		//Reset Control Register (ADRSTCR) to release the module from the reset state
		adcReg->RSTCR = 0x0;

		//	//Enable the ADC state machine
		adcReg->OPMODECR = adcReg->OPMODECR | 0x1;

		//Configure the ADCLK frequency  (Max 30Mhz)
		adcReg->CLOCKCR = 0x4;

		if (adcReg == adcREG1) {
			adcReg->G1SAMP = ConfigPtr->hwConfigPtr[i].convTime;
		} else if (adcReg == adcREG2) {
			adcReg->G2SAMP = ConfigPtr->hwConfigPtr[i].convTime;
		}

		for (int j = 0; j < ConfigPtr[i].nbrOfGroups; j++) {
#if (ADC_GRP_NOTIF_CAPABILITY==STD_ON)
			Adc_DisableGroupNotification(ConfigPtr[i].groupConfigPtr[j].groupId);
#endif
			ConfigPtr[i].groupConfigPtr[j].status->groupStatus = ADC_IDLE;
		}
	}

	#if (ADC_GRP_NOTIF_CAPABILITY==STD_ON)
	ISR_INSTALL_ISR1( "ADC1Grp1ConvComplete", ADC1Grp1ConversionComplete_Isr, 15, ADC_ISR_PRIORITY, 0 );
	ISR_INSTALL_ISR1( "ADC2Grp2ConvComplete", ADC2Grp2ConversionComplete_Isr, 57, ADC_ISR_PRIORITY, 0 );
	#endif

	CoreStates[0].status = CoreStates[1].status = ADC_BUSY;
	CoreStates[0].activeGroup = CoreStates[1].activeGroup = NULL;

	//	//Init done, change state to ADC_STATE_INIT
	adcState = ADC_STATE_INIT;
}

#if (ADC_DEINIT_API == STD_ON)
void Adc_DeInit (void) {
	/* Set AD Reset Control Register(ADRSTCR) to reset all ADC internal state machines and control
	   and status registers */
	adcREG1->RSTCR = 0x0;
	adcREG2->RSTCR = 0x0;

	//TODO: reset driver-internal states, structs and variables if needed
	adcState = ADC_STATE_UNINIT;
}
#endif

Std_ReturnType Adc_SetupResultBuffer (Adc_GroupType group, Adc_ValueGroupType *bufferPtr)
{
  Std_ReturnType returnValue = E_NOT_OK;

 /* Check for development errors. */
  if (E_OK == Adc_CheckSetupResultBuffer (adcState, AdcConfigPtr, group, bufferPtr))
  {

	  /* @req SWS_Adc_00420 */
	  AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].status->resultBufferPtr = bufferPtr;

	  returnValue = E_OK;
  }
  else
  {
    /* An error have been raised from Adc_CheckSetupResultBuffer(). */
    returnValue = E_NOT_OK;
  }

  return (returnValue);
}


#if (ADC_READ_GROUP_API == STD_ON)
Std_ReturnType Adc_ReadGroup (Adc_GroupType group, Adc_ValueGroupType *dataBufferPtr)
{
  Std_ReturnType returnValue;
  Adc_ChannelType channel;

  if (E_OK == Adc_CheckReadGroup (adcState, AdcConfigPtr, group))
  {

      /* Copy the result to application buffer. */
      for (channel = 0; channel < AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].numberOfChannels; channel++)
      {
          /* @req SWS_Adc_00075 */
          dataBufferPtr[channel] = AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].status->resultBufferPtr[channel];
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

Adc_StatusType Adc_GetGroupStatus (Adc_GroupType Group)
{
	return AdcConfigPtr[getHw(Group)].groupConfigPtr[Group % 100].status->groupStatus;
}

#if (ADC_ENABLE_START_STOP_GROUP_API == STD_ON)
void Adc_StartGroupConversion (Adc_GroupType group)
{
	/* @req SWS_Adc_00156 */
  /* Run development error check. */
  if (E_OK == Adc_CheckStartGroupConversion (adcState, AdcConfigPtr, group))
  {
#if ( ADC_DEV_ERROR_DETECT == STD_ON )
	  if (CoreStates[getHw(group)].status == ADC_BUSY) {
		  Det_ReportError(ADC_MODULE_ID,0,ADC_STARTGROUPCONVERSION_ID,ADC_E_BUSY );
	  }
#endif
	  unsigned int ci = 0;

	  unsigned channels = 0x0;

	  for(ci = 0; ci < AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].numberOfChannels; ci++) {
			  channels |= (1 << AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].channelList[ci]);
	  }

	  if(getHw(group) == 0) {
	  	  adcREG1->GxSEL[1] = channels;
	  } else if(getHw(group) == 1){
		  adcREG2->GxSEL[2] = channels;
	  }

    /* Set group state to BUSY. */
    AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].status->groupStatus = ADC_BUSY;

    CoreStates[getHw(group)].status = ADC_BUSY;
    CoreStates[getHw(group)].activeGroup = &AdcConfigPtr[getHw(group)].groupConfigPtr[group%100];
  }
}


void Adc_StopGroupConversion (Adc_GroupType group)
{
  /* Run development error check. */
  if (E_OK == Adc_CheckStopGroupConversion (adcState, AdcConfigPtr, group))
  {

	  if(getHw(group) == 0) {
		  adcREG1->GxSEL[1] = 0x0;
	  } else if (group == 1){
		  adcREG2->GxSEL[2] = 0x0;
	  }
#if (ADC_GRP_NOTIF_CAPABILITY==STD_ON)
	  Adc_DisableGroupNotification(group);
#endif
	  AdcConfigPtr[getHw(group)].groupConfigPtr[group%100].status->groupStatus = ADC_IDLE;
  }
}
#endif

#if (STD_ON == ADC_VERSION_INFO_API)
void Adc_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    /* @req ADC458 */
     VALIDATE( !(versioninfo == NULL), ADC_GETVERSIONINFO_ID, ADC_E_PARAM_POINTER );


    versioninfo->vendorID           = ADC_VENDOR_ID;
    versioninfo->moduleID           = ADC_MODULE_ID;
    versioninfo->sw_major_version   = ADC_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version   = ADC_SW_MINOR_VERSION;
    versioninfo->sw_patch_version   = ADC_SW_PATCH_VERSION;

    return;
}
#endif

#if (STD_ON == ADC_GRP_NOTIF_CAPABILITY)
void Adc_EnableGroupNotification(Adc_GroupType Group) {
	//validate group here
#if ( ADC_DEV_ERROR_DETECT == STD_ON )
	if (E_NOT_OK == ValidateGroup(AdcConfigPtr, Group, ADC_ENABLEGROUPNOTIFICATION_ID))
	{
		return;
	}
#endif
	if (getHw(Group) == 0) {
		adcREG1->GxINTENA[1] |= (1<<3); //Enable Conversion End Interrupt for Group 1 in ADC1
		adcREG1->GxINTCR[1] = 1U;
	} else if (getHw(Group) == 1) {
		adcREG2->GxINTENA[2] |= (1<<3); //Enable Conversion End Interrupt for Group 2 in ADC2
		adcREG2->GxINTCR[2] = 1U;
	}
}

void Adc_DisableGroupNotification(Adc_GroupType Group) {
#if ( ADC_DEV_ERROR_DETECT == STD_ON )
	if (E_NOT_OK == ValidateGroup(AdcConfigPtr, Group, ADC_ENABLEGROUPNOTIFICATION_ID))
	{
		return;
	}
#endif
	if (getHw(Group) == 0) {
		adcREG1->GxINTENA[1] &= ~(1 << 3); //Enable Conversion End Interrupt for Group 1 in ADC1
		adcREG1->GxINTCR[1] = 0U;
	} else if (getHw(Group) == 1) {
		adcREG2->GxINTENA[2] &= ~(1 << 3); //Enable Conversion End Interrupt for Group 2 in ADC2
		adcREG2->GxINTCR[2] = 0U;
	}
}
#endif
/* Functions below, specific to driver */

#if (STD_ON == ADC_GRP_NOTIF_CAPABILITY)
ISR(ADC1Grp1ConversionComplete_Isr) {
	ADCConversionComplete_Isr(0);
}

ISR(ADC2Grp2ConversionComplete_Isr) {
	ADCConversionComplete_Isr(1);
}

static void ADCConversionComplete_Isr(unsigned int adc) {
	int channels = AdcConfigPtr[adc].groupConfigPtr[0].numberOfChannels;
	const Adc_GroupDefType* groupPtr = CoreStates[adc].activeGroup;

	for (int i = 0; i < channels; i++) {
		if (adc == 0) {
			groupPtr->status->resultBufferPtr[i] = *((uint32_t*)(&adcREG1->GxBUF[1]) + i);
		} else if (adc == 1) {
			groupPtr->status->resultBufferPtr[i] = *((uint32_t*)(&adcREG2->GxBUF[2]) + i);
		}
	}
	groupPtr->status->groupStatus = ADC_STREAM_COMPLETED;
	CoreStates[adc].status = ADC_IDLE;
	if (groupPtr->groupCallback != NULL) {
		groupPtr->groupCallback();
	}
	if (adc == 0) {
		adcREG1->GxINTFLG[1] = 9U;
		adcREG1->G1SR &= ~(1 << 0);

	} else if (adc == 1) {
		adcREG2->GxINTFLG[2] = 9U;
		adcREG2->G2SR &= ~(1 << 0);
	}
}
#endif

static void populateGroupMaps(void) {
	int grp = 0;
	for (int i = 0; i < ADC_ARC_CTRL_CONFIG_CNT; i++) {
		for (int j = 0; j < AdcConfigPtr[i].nbrOfGroups; j++) {
			gm[grp] = AdcConfigPtr[i].groupConfigPtr[j].hwUnit;
			groupIdMap[grp] = AdcConfigPtr[i].groupConfigPtr[j].groupId;
			grp++;
		}
	}
}

static unsigned int getHw(unsigned int group) {
	int seqId = getGroupId(group);
	return gm[seqId];
}

static unsigned int getGroupId(unsigned int group) {
	for (int i = 0; i < ADC_ARC_NR_OF_GROUPS; i++) {
		if (groupIdMap[i] == group) return i;
	}
	return -1; //Error
}
