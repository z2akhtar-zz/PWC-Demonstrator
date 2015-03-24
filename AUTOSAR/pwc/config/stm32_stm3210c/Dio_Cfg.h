
/*
 * Generator version: 5.0.0
 * AUTOSAR version:   4.1.2
 */

#ifndef DIO_CFG_H_
#define DIO_CFG_H_

#include "Dio.h"

#if !(((DIO_SW_MAJOR_VERSION == 5) && (DIO_SW_MINOR_VERSION == 0)) )
#error Dio: Configuration file expected BSW module version to be 5.0.*
#endif

#if !(((DIO_AR_RELEASE_MAJOR_VERSION == 4) && (DIO_AR_RELEASE_MINOR_VERSION == 1)) )
#error Dio: Configuration file expected AUTOSAR version to be 4.1.*
#endif


#define DIO_VERSION_INFO_API    STD_ON
#define DIO_DEV_ERROR_DETECT    STD_ON

#define DIO_END_OF_LIST  (0xFFFFFFFFu)

typedef enum {
	DIO_PORT_A,
	DIO_PORT_B,
	DIO_PORT_C,
	DIO_PORT_D,
	DIO_PORT_E,
	DIO_PORT_F,
} Dio_Hw_PortType;

// Channels
#define DioConf_DioChannel_I2C_SCL 21
#define Dio_I2C_SCL DioConf_DioChannel_I2C_SCL
#define DioConf_DioChannel_I2C_SDA 22
#define Dio_I2C_SDA DioConf_DioChannel_I2C_SDA
#define DioConf_DioChannel_Key 25
#define Dio_Key DioConf_DioChannel_Key
#define DioConf_DioChannel_LED_1 55
#define Dio_LED_1 DioConf_DioChannel_LED_1
#define DioConf_DioChannel_LED_2 61
#define Dio_LED_2 DioConf_DioChannel_LED_2
#define DioConf_DioChannel_LED_3 51
#define Dio_LED_3 DioConf_DioChannel_LED_3
#define DioConf_DioChannel_LED_4 52
#define Dio_LED_4 DioConf_DioChannel_LED_4
#define DioConf_DioChannel_Tamper 45
#define Dio_Tamper DioConf_DioChannel_Tamper
#define DioConf_DioChannel_WakeUp 0
#define Dio_WakeUp DioConf_DioChannel_WakeUp

// Channel groups


// Ports
#define DioConf_DioPort_LED_PORT (DIO_PORT_D)
#define Dio_LED_PORT DioConf_DioPort_LED_PORT
#define DioConf_DioPort_PortA (DIO_PORT_A)
#define Dio_PortA DioConf_DioPort_PortA
#define DioConf_DioPort_PortB (DIO_PORT_B)
#define Dio_PortB DioConf_DioPort_PortB
#define DioConf_DioPort_PortC (DIO_PORT_C)
#define Dio_PortC DioConf_DioPort_PortC

/* Configuration Set Handles */
#define DioConfig (&DioConfigData)
#define Dio_DioConfig (&DioConfigData)

#endif /*DIO_CFG_H_*/
