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
#include "Port.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#include "Cpu.h"
#include <string.h>

#define GET_PIN_PORT(_pin) (_pin >> 8)
#define GET_PIN_PIN(_pin)  (_pin & 0x1F)
#define GET_PIN_MASK(_pin) (1 << (_pin & 0x1F))

typedef enum {
	PORT_UNINITIALIZED = 0, PORT_INITIALIZED,
} Port_StateType;

typedef volatile struct {
	uint32 FUN;
	uint32 DIR;
	uint32 DIN;
	uint32 DOUT;
	uint32 DSET;
	uint32 DCLR;
	uint32 PDR;
	uint32 PULDIS;
	uint32 PSL;
} Port_GioRegisterType;

typedef volatile struct {
	uint32 PC1; /* DIR    */
	uint32 PC2; /* DIN    */
	uint32 PC3; /* DOUT   */
	uint32 PC4; /* DSET   */
	uint32 PC5; /* DCLR   */
	uint32 PC6; /* PDR    */
	uint32 PC7; /* PULDIS */
	uint32 PC8; /* PSL    */
} Port_SpiControlRegisterType;

#define PORT_NOT_CONFIGURED 0x00000000

#define PORT_GIO_0_BASE ((Port_GioRegisterType *)0xFFF7BC30)
#define PORT_GIO_1_BASE ((Port_GioRegisterType *)0xFFF7BC50)
#define PORT_GIO_2_BASE ((Port_GioRegisterType *)0xFFF7B848) //Start at -4 bytes as it lacks FUN
#define PORT_GIO_3_BASE ((Port_GioRegisterType *)PORT_NOT_CONFIGURED)
#define PORT_GIO_4_BASE ((Port_GioRegisterType *)PORT_NOT_CONFIGURED)
#define PORT_GIO_5_BASE ((Port_GioRegisterType *)PORT_NOT_CONFIGURED)
#define PORT_GIO_6_BASE ((Port_GioRegisterType *)PORT_NOT_CONFIGURED)
#define PORT_GIO_7_BASE ((Port_GioRegisterType *)PORT_NOT_CONFIGURED)
#define PORT_GIO_8_BASE ((Port_GioRegisterType *)0xFFF7DDE0)
#define PORT_GIO_9_BASE ((Port_GioRegisterType *)0xFFF7DFE0)
#define PORT_GIO_10_BASE ((Port_GioRegisterType *)0xFFF7E1E0)
#define PORT_NUMBER_OF_PORTS 11

#define PORT_SPI5_GIO_BASE ((Port_SpiControlRegisterType*)0xFFF7FC18)


static Port_GioRegisterType * const Gio_Port_Base[] = {
	PORT_GIO_0_BASE,
	PORT_GIO_1_BASE,
	PORT_GIO_2_BASE,
	PORT_GIO_3_BASE,
	PORT_GIO_4_BASE,
	PORT_GIO_5_BASE,
	PORT_GIO_6_BASE,
	PORT_GIO_7_BASE,
	PORT_GIO_8_BASE,
	PORT_GIO_9_BASE,
	PORT_GIO_10_BASE
};


#define PINMMR0_BASE_ADDR ((uint32 *)0xFFFFEB10)
#define PINMMR1_BASE_ADDR ((uint32 *)0xFFFFEB14)
#define PINMMR2_BASE_ADDR ((uint32 *)0xFFFFEB18)
#define PINMMR3_BASE_ADDR ((uint32 *)0xFFFFEB1C)
#define PINMMR4_BASE_ADDR ((uint32 *)0xFFFFEB20)
#define PINMMR5_BASE_ADDR ((uint32 *)0xFFFFEB24)
#define PINMMR6_BASE_ADDR ((uint32 *)0xFFFFEB28)
#define PINMMR7_BASE_ADDR ((uint32 *)0xFFFFEB2C)
#define PINMMR8_BASE_ADDR ((uint32 *)0xFFFFEB30)
#define PINMMR9_BASE_ADDR ((uint32 *)0xFFFFEB34)
#define PINMMR10_BASE_ADDR ((uint32 *)0xFFFFEB38)
#define PINMMR11_BASE_ADDR ((uint32 *)0xFFFFEB3C)
#define PINMMR12_BASE_ADDR ((uint32 *)0xFFFFEB40)
#define PINMMR13_BASE_ADDR ((uint32 *)0xFFFFEB44)
#define PINMMR14_BASE_ADDR ((uint32 *)0xFFFFEB48)
#define PINMMR15_BASE_ADDR ((uint32 *)0xFFFFEB4C)
#define PINMMR16_BASE_ADDR ((uint32 *)0xFFFFEB50)
#define PINMMR17_BASE_ADDR ((uint32 *)0xFFFFEB54)
#define PINMMR18_BASE_ADDR ((uint32 *)0xFFFFEB58)
#define PINMMR19_BASE_ADDR ((uint32 *)0xFFFFEB5C)
#define PINMMR20_BASE_ADDR ((uint32 *)0xFFFFEB60)
#define PINMMR21_BASE_ADDR ((uint32 *)0xFFFFEB64)
#define PINMMR22_BASE_ADDR ((uint32 *)0xFFFFEB68)
#define PINMMR23_BASE_ADDR ((uint32 *)0xFFFFEB6C)
#define PINMMR24_BASE_ADDR ((uint32 *)0xFFFFEB70)
#define PINMMR25_BASE_ADDR ((uint32 *)0xFFFFEB74)
#define PINMMR26_BASE_ADDR ((uint32 *)0xFFFFEB78)
#define PINMMR27_BASE_ADDR ((uint32 *)0xFFFFEB7C)
#define PINMMR28_BASE_ADDR ((uint32 *)0xFFFFEB80)
#define PINMMR29_BASE_ADDR ((uint32 *)0xFFFFEB84)
#define PINMMR30_BASE_ADDR ((uint32 *)0xFFFFEB88)
#define PINMMR31_BASE_ADDR ((uint32 *)0xFFFFEB8C)
#define PINMMR32_BASE_ADDR ((uint32 *)0xFFFFEB90)
#define PINMMR33_BASE_ADDR ((uint32 *)0xFFFFEB94)
#define PINMMR34_BASE_ADDR ((uint32 *)0xFFFFEB98)
#define PINMMR35_BASE_ADDR ((uint32 *)0xFFFFEB9C)
#define PINMMR36_BASE_ADDR ((uint32 *)0xFFFFEBA0)
#define PINMMR37_BASE_ADDR ((uint32 *)0xFFFFEBA4)
#define PINMMR38_BASE_ADDR ((uint32 *)0xFFFFEBA8)
#define PINMMR39_BASE_ADDR ((uint32 *)0xFFFFEBAC)
#define PINMMR40_BASE_ADDR ((uint32 *)0xFFFFEBB0)
#define PINMMR41_BASE_ADDR ((uint32 *)0xFFFFEBB4)
#define PINMMR42_BASE_ADDR ((uint32 *)0xFFFFEBB8)
#define PINMMR43_BASE_ADDR ((uint32 *)0xFFFFEBBC)
#define PINMMR44_BASE_ADDR ((uint32 *)0xFFFFEBC0)
#define PINMMR45_BASE_ADDR ((uint32 *)0xFFFFEBC4)
#define PINMMR46_BASE_ADDR ((uint32 *)0xFFFFEBC8)
#define PINMMR47_BASE_ADDR ((uint32 *)0xFFFFEBCC)



static uint32 * PINMMR_Port_Base[] = {
	PINMMR0_BASE_ADDR,
	PINMMR1_BASE_ADDR,
	PINMMR2_BASE_ADDR,
	PINMMR3_BASE_ADDR,
	PINMMR4_BASE_ADDR,
	PINMMR5_BASE_ADDR,
	PINMMR6_BASE_ADDR,
	PINMMR7_BASE_ADDR,
	PINMMR8_BASE_ADDR,
	PINMMR9_BASE_ADDR,
	PINMMR10_BASE_ADDR,
	PINMMR11_BASE_ADDR,
	PINMMR12_BASE_ADDR,
	PINMMR13_BASE_ADDR,
	PINMMR14_BASE_ADDR,
	PINMMR15_BASE_ADDR,
	PINMMR16_BASE_ADDR,
	PINMMR17_BASE_ADDR,
	PINMMR18_BASE_ADDR,
	PINMMR19_BASE_ADDR,
	PINMMR20_BASE_ADDR,
	PINMMR21_BASE_ADDR,
	PINMMR22_BASE_ADDR,
	PINMMR23_BASE_ADDR,
	PINMMR24_BASE_ADDR,
	PINMMR25_BASE_ADDR,
	PINMMR26_BASE_ADDR,
	PINMMR27_BASE_ADDR,
	PINMMR28_BASE_ADDR,
	PINMMR29_BASE_ADDR,
	PINMMR30_BASE_ADDR,
	PINMMR31_BASE_ADDR,
	PINMMR32_BASE_ADDR,
	PINMMR33_BASE_ADDR,
	PINMMR34_BASE_ADDR,
	PINMMR35_BASE_ADDR,
	PINMMR36_BASE_ADDR,
	PINMMR37_BASE_ADDR,
	PINMMR38_BASE_ADDR,
	PINMMR39_BASE_ADDR,
	PINMMR40_BASE_ADDR,
	PINMMR41_BASE_ADDR,
	PINMMR42_BASE_ADDR,
	PINMMR43_BASE_ADDR,
	PINMMR44_BASE_ADDR,
	PINMMR45_BASE_ADDR,
	PINMMR46_BASE_ADDR,
	PINMMR47_BASE_ADDR
};

#define PORT_IOMM_KICK0 0xFFFFEA38
#define PORT_IOMM_KICK1 0xFFFFEA3C

#define PORT_IOMM_KICK0_MAGIC (0x83e70b13)
#define PORT_IOMM_KICK1_MAGIC (0x95a4f1e0)

#define SPI5GCR ((uint32_t*)0xFFF7FC00)

static Port_StateType _portState = PORT_UNINITIALIZED;
static const Port_ConfigType * _configPtr = &PortConfigData;

#if PORT_DEV_ERROR_DETECT == STD_ON
#define VALIDATE(_exp, _apiid, _errid) \
		if (!(_exp)) { \
			Det_ReportError(PORT_MODULE_ID, 0, _apiid, _errid); \
			return; \
		}

#define VALIDATE_PIN(_pin, _api) VALIDATE((_pin >= 0 && _pin < PORT_NR_PINS), _api, PORT_E_PARAM_PIN)

#else
#define VALIDATE(_exp, _apiid, _errid)
#define VALIDATE_PIN(_pin, _api)
#endif

#if PORT_VERSION_INFO_API == STD_ON
static Std_VersionInfoType _Port_VersionInfo =
{
	.vendorID = (uint16)PORT_VENDOR_ID,
	.moduleID = (uint16) PORT_MODULE_ID,
	.sw_major_version = (uint8)PORT_SW_MAJOR_VERSION,
	.sw_minor_version = (uint8)PORT_SW_MINOR_VERSION,
	.sw_patch_version = (uint8)PORT_SW_PATCH_VERSION,
};
#endif

static inline void Internal_RefreshGioPin(uint16 pinNumber) {
	const ArcPort_PadConfigType * pin = &_configPtr->padConfig[pinNumber];
	Port_GioRegisterType *portReg = Gio_Port_Base[pin->regOffset];
	uint32 mask = GET_PIN_MASK(pin->regBit);

	/* @req SWS_Port_00055 */
	/* Pull up/down should be set before direction */

	// Set pull up or down or nothing.
	if (pin->config & PORT_PULL_NONE) {
		portReg->PULDIS |= mask;

	} else {
		portReg->PULDIS &= ~mask;
		if (pin->config & PORT_PULL_UP) {
			portReg->PSL |= mask;

		} else {
			portReg->PSL &= ~mask;
		}
	}

	// Set pin direction
	if (pin->config & PORT_DIR_OUT) {
		portReg->DIR |= mask;

		// Set open drain
		if (pin->config & PORT_ODE_ENABLE) {
			portReg->PDR |= mask;
		} else {
			portReg->PDR &= ~mask;
		}

	} else {
		portReg->DIR &= ~mask;
	}
}

static inline void Internal_RefreshSpiGioPin(uint16 pinNumber) {
	const ArcPort_PadConfigType * pin = &_configPtr->padConfig[pinNumber];
	Port_SpiControlRegisterType *portReg = PORT_SPI5_GIO_BASE;
	uint8 bit = pin->regBit;

	/* @req SWS_Port_00055 */
	/* Pull up/down should be set before direction */

	// Set pull up or down or nothing.
	if (pin->config & PORT_PULL_NONE) {
		portReg->PC7 |= (1 << bit);

	} else {
		portReg->PC7 &= ~(1 << bit);
		if (pin->config & PORT_PULL_UP) {
			portReg->PC8 |= (1 << bit);
		} else {
			portReg->PC8 &= ~(1 << bit);
		}
	}

	// Set pin direction
	if (pin->config & PORT_DIR_OUT) {
		portReg->PC1 |= (1 << bit);

		// Set open drain
		if (pin->config & PORT_ODE_ENABLE) {
			portReg->PC6 |= (1 << bit);
		} else {
			portReg->PC6 &= ~(1 << bit);
		}

	} else {
		portReg->PC1 &= ~(1 << bit);
	}
}

void Internal_RefreshPin(uint16 pinNumber) {
	const ArcPort_PadConfigType * pin = &_configPtr->padConfig[pinNumber];

	// @req SWS_Port_00214
	if (pin->mode == PORT_PIN_MODE_DIO) {
		Internal_RefreshGioPin(pinNumber);
	} else if (pin->mode == PORT_PIN_MODE_DIO_SPI) {
		*(SPI5GCR) = 0x1; /* bring SPI module out of reset */
		Internal_RefreshSpiGioPin(pinNumber);
	} else {
		*(PINMMR_Port_Base[pin->regOffset]) &= ~(0xFF << ((pin->regBit / 8) * 8));
		*(PINMMR_Port_Base[pin->regOffset]) |= (1 << pin->regBit);
	}
}

// @req SWS_Port_00140
void Port_Init(const Port_ConfigType *configType) {
	// @req SWS_Port_00105
	VALIDATE(configType != NULL, PORT_INIT_ID, PORT_E_PARAM_CONFIG);

	_configPtr = (Port_ConfigType *) configType;

	// Bring GIO register out of reset.
	gioREG->GCR0 = 1;

	*((uint32*)PORT_IOMM_KICK0) = PORT_IOMM_KICK0_MAGIC;
	*((uint32*)PORT_IOMM_KICK1) = PORT_IOMM_KICK1_MAGIC;


	// @req SWS_Port_00041
	for (uint16 i = 0; i < PORT_NR_PINS; i++) {
		Internal_RefreshPin(i);
	}

	_portState = PORT_INITIALIZED;

	*((uint32*)PORT_IOMM_KICK0) = 0x00000000;

}

// @req SWS_Port_00086
#if ( PORT_SET_PIN_DIRECTION_API == STD_ON )
// @req SWS_Port_00141
void Port_SetPinDirection( Port_PinType pinNr, Port_PinDirectionType direction )
{
	VALIDATE(_portState == PORT_INITIALIZED, PORT_SET_PIN_DIRECTION_ID, PORT_E_UNINIT);
	VALIDATE_PIN(pinNr, PORT_SET_PIN_DIRECTION_ID);

	const ArcPort_PadConfigType * pin= &_configPtr->padConfig[pinNr];
	Port_GioRegisterType *portReg = Gio_Port_Base[pin->regOffset];
	uint32 mask = GET_PIN_MASK(pin->regBit);

	// @req SWS_Port_00063
	if (direction & PORT_PIN_IN) {
		portReg->DIR |= mask;

	} else {
		portReg->DIR &= ~mask;

	}
}
#endif

// @req SWS_Port_00142
void Port_RefreshPortDirection(void) {
	// @req SWS_Port_00051
	VALIDATE(_portState == PORT_INITIALIZED, PORT_REFRESH_PORT_DIRECTION_ID, PORT_E_UNINIT);
	// @req SWS_Port_00060
	for (uint16 i = 0; i < PORT_NR_PINS; i++) {
		//if (!(_configPtr->padConfig[i].config & PORT_DIRECTION_CHANGEABLE)) {
		Internal_RefreshPin(i);
		//}
	}
}


#if PORT_VERSION_INFO_API == STD_ON
void Port_GetVersionInfo(Std_VersionInfoType* versionInfo)
{
	//No need to check if initialized here, that requirement was removed
	VALIDATE(versionInfo != NULL, PORT_GET_VERSION_INFO_ID, PORT_E_PARAM_POINTER);
	memcpy(versionInfo, &_Port_VersionInfo, sizeof(Std_VersionInfoType));
}
#endif

#if (PORT_SET_PIN_MODE_API == STD_ON)
// @req SWS_Port_00145
void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode) {
	// @req SWS_Port_00051
	VALIDATE(_portState == PORT_INITIALIZED, PORT_SET_PIN_MODE_ID, PORT_E_UNINIT);
	VALIDATE_PIN(Pin, PORT_SET_PIN_MODE_ID);

	uint8 port = GET_PIN_PORT(Pin);
	uint8 pin = GET_PIN_PIN(Pin);
	uint32 mask = GET_PIN_MASK(Pin);

	// @req SWS_Port_00125
	Port_Base[port]->FUN &= ~mask;
	Port_Base[port]->FUN |= ((Mode & 1) << pin);
	return;
}
#endif
