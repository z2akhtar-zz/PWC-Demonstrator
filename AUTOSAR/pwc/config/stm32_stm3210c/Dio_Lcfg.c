
/*
 * Generator version: 5.0.0
 * AUTOSAR version:   4.1.2
 */

#include "Dio.h"
#include "Dio_Cfg.h"

const Dio_ChannelType DioChannelConfigData[] = { 
	DioConf_DioChannel_I2C_SCL,
	DioConf_DioChannel_I2C_SDA,
	DioConf_DioChannel_Key,
	DioConf_DioChannel_LED_1,
	DioConf_DioChannel_LED_2,
	DioConf_DioChannel_LED_3,
	DioConf_DioChannel_LED_4,
	DioConf_DioChannel_Tamper,
	DioConf_DioChannel_WakeUp,
	DIO_END_OF_LIST
};

const Dio_PortType DioPortConfigData[] = { 
	DioConf_DioPort_LED_PORT,
	DioConf_DioPort_PortA,
	DioConf_DioPort_PortB,
	DioConf_DioPort_PortC,
	DIO_END_OF_LIST
};

const Dio_ChannelGroupType DioGroupConfigData[] = {
	{ 
	  .port = DIO_END_OF_LIST, 
	  .mask = 0,
	  .offset = 0
	}
};

const Dio_ConfigType DioConfigData = {
	.ChannelConfig = DioChannelConfigData,
	.GroupConfig = DioGroupConfigData,
	.PortConfig = DioPortConfigData
};

