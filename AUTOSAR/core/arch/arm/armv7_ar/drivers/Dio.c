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

/** @req SWS_Dio_00012 */
/** @req SWS_Dio_00063 MCU architecture specific driver */
/** @req SWS_Dio_00051 Read and Write services are acting directly on HW registers */
/** @req SWS_Dio_00118 */
/** @req SWS_Dio_00083 */
/** @req SWS_Dio_00131 */
/** @req SWS_Dio_00129 */
/** @req SWS_Dio_00026 Implemented in generator */
/** @req SWS_Dio_00113 Implemented in generator */
/** @req SWS_Dio_00017 Implemented in generator */
/** @req SWS_Dio_00020 Implemented in generator */
/** @req SWS_Dio_00022 Implemented in generator */


#include "Dio.h"
#if defined(USE_DET)
#include "Det.h" /** @req SWS_Dio_00194 */
#endif
#include <string.h>
#include "zynq.h"

#if (DIO_VERSION_INFO_API == STD_ON)
static Std_VersionInfoType _Dio_VersionInfo =
{ .vendorID = (uint16)DIO_VENDOR_ID, .moduleID = (uint16) DIO_MODULE_ID,
        .sw_major_version = (uint8)DIO_SW_MAJOR_VERSION,
        .sw_minor_version = (uint8)DIO_SW_MINOR_VERSION,
        .sw_patch_version = (uint8)DIO_SW_PATCH_VERSION
};
#endif

#define OUTPUT_ONLY_PIN_8 8
#define OUTPUT_ONLY_PIN_7 7
#define OUTPUT_ONLY_PIN_MASK ((1<<OUTPUT_ONLY_PIN_8)&(1<<OUTPUT_ONLY_PIN_7))
#define GPIO_BANK_1_REG_WIDTH ((2 << 22)-1)

typedef struct {
    const Dio_ConfigType* Config;
    boolean InitRun;
}Dio_GlobalType;

static Dio_GlobalType DioGlobal = {.InitRun = FALSE, .Config = NULL};

#if ( DIO_DEV_ERROR_DETECT == STD_ON )
static int Port_Config_Contains(Dio_PortType portId)
{
  const Dio_PortType* port_ptr=DioGlobal.Config->PortConfig;
  int rv=0;
  while (DIO_END_OF_LIST!=*port_ptr)
  {
    if (*port_ptr==portId)
    { rv=1; break;}
    port_ptr++;
  }
  return rv;
}

static int Channel_Group_Config_Contains(const Dio_ChannelGroupType* _channelGroupIdPtr)
{
  const Dio_ChannelGroupType* chGrp_ptr=DioGlobal.Config->GroupConfig;
  int rv=0;
  if(_channelGroupIdPtr != 0) {
    while (DIO_END_OF_LIST!=chGrp_ptr->port)
    {
      if (chGrp_ptr->port==_channelGroupIdPtr->port&&
          chGrp_ptr->mask==_channelGroupIdPtr->mask)
      { rv=1; break;}
      chGrp_ptr++;
    }
  }
  return rv;
}
/** @req SWS_Dio_00074 */
#define VALIDATE_CHANNEL_RV(_channelId, _api, _rv) \
    if(0==(DIO_AVAILABLE_CHANNELS & ((uint64)1<<_channelId))) { \
        (void)Det_ReportError(DIO_MODULE_ID,0,_api,DIO_E_PARAM_INVALID_CHANNEL_ID ); \
        return _rv;   \
        }

#define VALIDATE_CHANNEL(_channelId, _api) \
    if(0==(DIO_AVAILABLE_CHANNELS & ((uint64)1<<_channelId))) { \
        (void)Det_ReportError(DIO_MODULE_ID,0,_api,DIO_E_PARAM_INVALID_CHANNEL_ID ); \
        return;   \
        }
/** @req SWS_Dio_00075 */
#define VALIDATE_PORT_RV(_portId, _api, _rv)\
    if(0==Port_Config_Contains(_portId)) {\
        (void)Det_ReportError(DIO_MODULE_ID,0,_api,DIO_E_PARAM_INVALID_PORT_ID ); \
        return _rv;\
    }
#define VALIDATE_PORT(_portId, _api)\
    if(0==Port_Config_Contains(_portId)) {\
        (void)Det_ReportError(DIO_MODULE_ID,0,_api,DIO_E_PARAM_INVALID_PORT_ID ); \
        return;\
    }
/** @req SWS_Dio_00114 */
#define VALIDATE_CHANNELGROUP_RV(_channelGroupIdPtr, _api, _rv)\
    if(0==Channel_Group_Config_Contains(_channelGroupIdPtr)) {\
        (void)Det_ReportError(DIO_MODULE_ID,0,_api,DIO_E_PARAM_INVALID_GROUP_ID ); \
        return _rv;\
    }
#define VALIDATE_CHANNELGROUP(_channelGroupIdPtr, _api)\
    if(0==Channel_Group_Config_Contains(_channelGroupIdPtr)) {\
        (void)Det_ReportError(DIO_MODULE_ID,0,_api,DIO_E_PARAM_INVALID_GROUP_ID ); \
        return;\
    }
#define VALIDATE(_exp,_api,_err ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(DIO_MODULE_ID,0,_api,_err); \
          return; \
        }
#define VALIDATE_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          (void)Det_ReportError(DIO_MODULE_ID,0,_api,_err); \
          return _rv; \
        }
#else
#define VALIDATE_CHANNEL_RV(_channelId, _api, _rv)
#define VALIDATE_CHANNEL(_channelId, _api)
#define VALIDATE_PORT_RV(_portId, _api, _rv)
#define VALIDATE_PORT(_portId, _api)
#define VALIDATE_CHANNELGROUP_RV(_channelGroupIdPtr, _api, _rv)
#define VALIDATE_CHANNELGROUP(_channelGroupIdPtr, _api)
#define VALIDATE(_exp,_api,_err )
#define VALIDATE_RV(_exp,_api,_err,_rv )
#endif

#define BANK0_WIDTH 32
#define BANK1_WIDTH 22
#define BANK2_WIDTH 32
#define BANK3_WIDTH 32

/** @req SWS_Dio_00027 */
/** @req SWS_Dio_00089 */
/** @req SWS_Dio_00128 */
/** @req SWS_Dio_00011 */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType channelId)
{
  Dio_LevelType level = STD_LOW;
  Dio_ChannelType chnlId;
  VALIDATE_RV( DioGlobal.InitRun, DIO_READCHANNEL_ID, DIO_E_UNINIT, (Dio_LevelType)0 );
  VALIDATE_CHANNEL_RV(channelId, DIO_READCHANNEL_ID, level);

  if (channelId < BANK0_WIDTH) {
	  /* Read from Bank 0, MIO */
	  if (OUTPUT_ONLY_PIN_7 != channelId && OUTPUT_ONLY_PIN_8 != channelId) { //Reading ch 7 & 8 through DATA0_RO always returns zero
          level = (GPIO_REG.DATA_0_RO & (1 << channelId)) >> channelId;
      } else { //Read output register
          level = (GPIO_REG.DATA_0 & (1 << channelId)) >> channelId;
      }
  }
  else {
      chnlId = channelId - BANK0_WIDTH;
      if (chnlId < BANK1_WIDTH) {
    	  /* Read from Bank 1, MIO */
    	  level = (GPIO_REG.DATA_1_RO & (1 << chnlId)) >> chnlId;
      }
      else {
           chnlId = chnlId - BANK1_WIDTH;
           if (chnlId < BANK2_WIDTH) {
         	  /* Read from Bank 2, EMIO */
         	  level = (GPIO_REG.DATA_2_RO & (1 << chnlId)) >> chnlId;
           }
           else {
              chnlId = channelId - BANK2_WIDTH;
          	  /* Read from Bank 3, EMIO */
          	  level = (GPIO_REG.DATA_3_RO & (1 << chnlId)) >> chnlId;
           }
      }
  }
  return (level);
}


/** @req SWS_Dio_00089 */
/** @req SWS_Dio_00128 */
/** @req SWS_Dio_00119 */
/** @!req SWS_Dio_00006 Write depends on value of level, instead on setting STD_HIGH/LOW */
/** @req SWS_Dio_00028 */
/** @req SWS_Dio_00029 */
/** @req SWS_Dio_00079 */
void Dio_WriteChannel(Dio_ChannelType channelId, Dio_LevelType level)
{
    Dio_ChannelType chnlId;
    VALIDATE( DioGlobal.InitRun, DIO_WRITECHANNEL_ID, DIO_E_UNINIT );
    VALIDATE_CHANNEL(channelId, DIO_WRITECHANNEL_ID);
    if (channelId < BANK0_WIDTH) {
        GPIO_REG.DATA_0 = (level << channelId) | (GPIO_REG.DATA_0 & ~((uint32)1 << channelId));
    }
    else {
        chnlId = channelId - BANK0_WIDTH;
        if (chnlId < BANK1_WIDTH) {
        	GPIO_REG.DATA_1 = (level << chnlId) | (GPIO_REG.DATA_1 & ~((uint32)1 << chnlId));
        }
        else {
            chnlId = chnlId - BANK1_WIDTH;
            if (chnlId < BANK2_WIDTH) {
            	GPIO_REG.DATA_2 = (level << chnlId) | (GPIO_REG.DATA_2 & ~((uint32)1 << chnlId));
            }
            else {
                chnlId = channelId - BANK2_WIDTH;
            	GPIO_REG.DATA_3 = (level << chnlId) | (GPIO_REG.DATA_3 & ~((uint32)1 << chnlId));
            }

        }
    }
  return;
}

/** @req SWS_Dio_00053 */
/** @req SWS_Dio_00031 */
/** @req SWS_Dio_00013 */
/** @!req SWS_Dio_00104 Undefined port pins are not set to 0 (for arch < 16bit) */
Dio_PortLevelType Dio_ReadPort(Dio_PortType portId) {
	Dio_PortLevelType level = 0;
	VALIDATE_RV( DioGlobal.InitRun, DIO_READPORT_ID, DIO_E_UNINIT, (Dio_PortLevelType)0 );

	VALIDATE_PORT_RV(portId, DIO_READPORT_ID, level);
	switch (portId) {
	case DIO_BANK_0: /* MIO */
		level = GPIO_REG.DATA_0_RO;
		level |= (GPIO_REG.DATA_0 & OUTPUT_ONLY_PIN_MASK); //Get the state of pins 8 and 7 (reading bit pos 7,8 from DATA_0_RO results always in zero)
		break;
	case DIO_BANK_1: /* MIO */
		level = GPIO_REG.DATA_1_RO & GPIO_BANK_1_REG_WIDTH;
		break;
	case DIO_BANK_2: /* EMIO */
		level = GPIO_REG.DATA_2_RO;
		break;
	case DIO_BANK_3: /* EMIO */
		level = GPIO_REG.DATA_3_RO;
		break;
	}
	return level;
}

/** @req SWS_Dio_00034 */
/** @req SWS_Dio_00053 */
/** @req SWS_Dio_00007 */
/** @!req SWS_Dio_00004 No check if channels of port is set as input */
/** @!req SWS_Dio_00035 No check if channels of port is set as input */
/** @req SWS_Dio_00105 Note: No check for endianess */
void Dio_WritePort(Dio_PortType portId, Dio_PortLevelType level)
{
    VALIDATE( DioGlobal.InitRun, DIO_READCHANNELGROUP_ID, DIO_E_UNINIT );
    VALIDATE_PORT(portId, DIO_WRITEPORT_ID);
    switch (portId) {
    case DIO_BANK_0: /* MIO */
        GPIO_REG.DATA_0 = level;
        break;
    case DIO_BANK_1: /* MIO */
        GPIO_REG.DATA_1 = level & GPIO_BANK_1_REG_WIDTH;
        break;
    case DIO_BANK_2: /* EMIO */
        GPIO_REG.DATA_2 = level;
        break;
    case DIO_BANK_3: /* EMIO */
        GPIO_REG.DATA_3 = level;
        break;
    }
    return;
}

/** @req SWS_Dio_00056 */
/** @req SWS_Dio_00014 */
/** @req SWS_Dio_00037 Depends on the generator */
/** @req SWS_Dio_00092 */
/** @req SWS_Dio_00093 */
Dio_PortLevelType Dio_ReadChannelGroup(
    const Dio_ChannelGroupType *channelGroupIdPtr)
{
    Dio_PortLevelType level = 0;
    VALIDATE_RV( DioGlobal.InitRun, DIO_READCHANNELGROUP_ID, DIO_E_UNINIT, (Dio_PortLevelType)0 );
    VALIDATE_CHANNELGROUP_RV(channelGroupIdPtr,DIO_READCHANNELGROUP_ID,level);

    switch (channelGroupIdPtr->port) {
    case DIO_BANK_0: /* MIO */
        level = GPIO_REG.DATA_0_RO;
        level |= (GPIO_REG.DATA_0 & OUTPUT_ONLY_PIN_MASK); //Get the state of pins 8 and 7 (reading bit pos 7,8 from DATA_0_RO results always in zero)
        break;
    case DIO_BANK_1: /* MIO */
        level = GPIO_REG.DATA_1_RO & GPIO_BANK_1_REG_WIDTH;
        break;
    case DIO_BANK_2: /* EMIO */
		level = GPIO_REG.DATA_2_RO;
		break;
    case DIO_BANK_3: /* EMIO */
		level = GPIO_REG.DATA_3_RO;
		break;
    }

    level &= channelGroupIdPtr->mask;
 return level;
}

/** @req SWS_Dio_00056 */
/** @req SWS_Dio_00008 */
/** @req SWS_Dio_00039 Depends on the generator */
/** @req SWS_Dio_00040 */
/** @req SWS_Dio_00090 */
/** @req SWS_Dio_00091 */
void Dio_WriteChannelGroup(const Dio_ChannelGroupType *channelGroupIdPtr,
    Dio_PortLevelType level) {
    VALIDATE( DioGlobal.InitRun, DIO_WRITECHANNELGROUP_ID, DIO_E_UNINIT );
    VALIDATE_CHANNELGROUP(channelGroupIdPtr,DIO_WRITECHANNELGROUP_ID);

    switch (channelGroupIdPtr->port) {
    case DIO_BANK_0: /* MIO */
		GPIO_REG.MASK_DATA_0_LSW = ~((channelGroupIdPtr->mask) << 16) | (level & 0xFFFF);
		GPIO_REG.MASK_DATA_0_MSW = (channelGroupIdPtr->mask ^ 0xFFFF0000) | ((uint32)level >> 16);
		break;
    case DIO_BANK_1: /* MIO */
        GPIO_REG.MASK_DATA_1_LSW = ~((channelGroupIdPtr->mask) << 16) | (level & 0xFFFF);
        GPIO_REG.MASK_DATA_1_MSW = (channelGroupIdPtr->mask ^ 0xFFFF0000) | ((uint32)level >> 16);
        break;
    case DIO_BANK_2: /* EMIO */
        GPIO_REG.MASK_DATA_2_LSW = ~((channelGroupIdPtr->mask) << 16) | (level & 0xFFFF);
        GPIO_REG.MASK_DATA_2_MSW = (channelGroupIdPtr->mask ^ 0xFFFF0000) | ((uint32)level >> 16);
        break;
    case DIO_BANK_3: /* EMIO */
        GPIO_REG.MASK_DATA_3_LSW = ~((channelGroupIdPtr->mask) << 16) | (level & 0xFFFF);
        GPIO_REG.MASK_DATA_3_MSW = (channelGroupIdPtr->mask ^ 0xFFFF0000) | ((uint32)level >> 16);
        break;
    }

    return;
}

#if (DIO_VERSION_INFO_API == STD_ON)
void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    /** @req SWS_Dio_00189 **/
    VALIDATE( !(versioninfo == NULL), DIO_GETVERSIONINFO_ID, DIO_E_PARAM_POINTER );

    memcpy(versioninfo, &_Dio_VersionInfo, sizeof(Std_VersionInfoType));
    return;
}

#endif
/** @req SWS_Dio_00165 **/
void Dio_Init(const Dio_ConfigType* ConfigPtr)
{
    /** @req SWS_Dio_00167 **/
    /** @req SWS_Dio_00166 **/
    VALIDATE( (NULL != ConfigPtr), DIO_INIT_ID, DIO_E_PARAM_CONFIG );
    DioGlobal.Config = ConfigPtr;
    DioGlobal.InitRun = TRUE;
}

#if defined(HOST_TEST)
void Dio_Arc_Deinit(void)
{
    DioGlobal.Config = NULL;
    DioGlobal.InitRun = FALSE;
}
#endif
