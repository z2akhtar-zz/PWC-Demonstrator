
/*
 * Generator version: 5.0.0
 * AUTOSAR version:   4.1.2
 */

#ifndef PORT_CFG_H_
#define PORT_CFG_H_

#include "Std_Types.h"
#include "Port_ConfigTypes.h"
#include "Port.h"

#if !(((PORT_SW_MAJOR_VERSION == 5) && (PORT_SW_MINOR_VERSION == 0)) )
#error Port: Configuration file expected BSW module version to be 5.0.*
#endif

#if !(((PORT_AR_RELEASE_MAJOR_VERSION == 4) && (PORT_AR_RELEASE_MINOR_VERSION == 1)) )
#error Port: Configuration file expected AUTOSAR version to be 4.1.*
#endif


#define	PORT_VERSION_INFO_API			STD_ON
#define	PORT_DEV_ERROR_DETECT			STD_ON
#define PORT_SET_PIN_DIRECTION_API		STD_ON
/** Allow Pin mode changes during runtime (not avail on this CPU) */
#define PORT_SET_PIN_MODE_API			STD_OFF
#define PORT_POSTBUILD_VARIANT 			STD_OFF


#define PortConf_PortPin_I2C_SCL		((Port_PinType)21)
#define Port_I2C_SCL	PortConf_PortPin_I2C_SCL
#define PortConf_PortPin_I2C_SDA		((Port_PinType)22)
#define Port_I2C_SDA	PortConf_PortPin_I2C_SDA
#define PortConf_PortPin_Key		((Port_PinType)25)
#define Port_Key	PortConf_PortPin_Key
#define PortConf_PortPin_PD13		((Port_PinType)61)
#define Port_PD13	PortConf_PortPin_PD13
#define PortConf_PortPin_PD3		((Port_PinType)51)
#define Port_PD3	PortConf_PortPin_PD3
#define PortConf_PortPin_PD4		((Port_PinType)52)
#define Port_PD4	PortConf_PortPin_PD4
#define PortConf_PortPin_PD7		((Port_PinType)55)
#define Port_PD7	PortConf_PortPin_PD7
#define PortConf_PortPin_Tamper		((Port_PinType)45)
#define Port_Tamper	PortConf_PortPin_Tamper
#define PortConf_PortPin_WakeUp		((Port_PinType)0)
#define Port_WakeUp	PortConf_PortPin_WakeUp

/** Instance of the top level configuration container */
extern const Port_ConfigType PortConfigData;

/* Configuration Set Handles */
#define PortConfigSet (&PortConfigData)
#define Can_PortConfigSet (&PortConfigData)


#endif /* PORT_CFG_H_ */
