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

/* General Port module requirements */
/** @req SWS_Port_00004 */
/** @req SWS_Port_00079 All pins are changeable during runtime */
/** @req SWS_Port_00081 */
/** @req SWS_Port_00082 */
/** @req SWS_Port_00124 */
/** @req SWS_Port_00013 Symbolic names */
/** @req SWS_Port_00073 */

/** @req SWS_Port_00006 Implemented in the generator/tool */
/** @req SWS_Port_00207 Implemented in the generator/tool */
/** @req SWS_Port_00076 Implemented in the generator */
/** @req SWS_Port_00006 */
/** @req SWS_Port_00075 Atomic access */
/** @req SWS_Port_00129 */
/** @req SWS_Port_00087 Not PORT_E_MODE_UNCHANGEABLE and PORT_E_DIRECTION_UNCHANGEABLE*/

/** @req SWS_Port_00204 Not including Port_Cbk.c*/
/** @req SWS_Port_00129 Additional types are included from other files */


#include "Port.h"
#include "zynq.h"
#if defined(USE_DET)
#include "Det.h"
#endif
#include "Cpu.h"
#include <string.h>
#include "SchM_Port.h"

#define PORT_ENTER_EXCLUSIVE_AREA(_area) SchM_Enter_Port_##_area()
#define PORT_EXIT_EXCLUSIVE_AREA(_area) SchM_Exit_Port_##_area()

#define PORT_DIR_MASK    0xFFFFFFFE
#define MIO_PIN_MAX      54
#define EMIO_PIN_MAX     118


typedef enum {
    PORT_UNINITIALIZED = 0, //!< PORT_UNINITIALIZED
    PORT_INITIALIZED,      //!< PORT_INITIALIZED
} Port_StateType;

static Port_StateType Port_state = PORT_UNINITIALIZED;
static const Port_ConfigType * Port_configPtr = NULL;

#if (PORT_DEV_ERROR_DETECT)
#define VALIDATE_PARAM_CONFIG(_ptr,_api) \
    if( (_ptr)==((void *)0) ) { \
        (void)Det_ReportError(PORT_MODULE_ID, 0, _api, PORT_E_PARAM_CONFIG ); \
        return; \
    }

#define VALIDATE_STATE_INIT(_api)\
    if(PORT_INITIALIZED!=Port_state){\
        (void)Det_ReportError(PORT_MODULE_ID, 0, _api, PORT_E_UNINIT ); \
        return; \
    }

#define VALIDATE_PARAM_PIN(_pin, _api)\
    if(_pin>= MIO_PIN_MAX){\
        (void)Det_ReportError(PORT_MODULE_ID, 0, _api, PORT_E_PARAM_PIN ); \
        return; \
    }

#define VALIDATE_EMIO_PARAM_PIN(_pin, _api)\
    if(_pin>= EMIO_PIN_MAX){\
        (void)Det_ReportError(PORT_MODULE_ID, 0, _api, PORT_E_PARAM_PIN ); \
        return; \
    }


#define VALIDATE_PARAM_POINTER(_pointer, _api)\
    if(_pointer == ((void *)0) ) {\
        (void)Det_ReportError(PORT_MODULE_ID, 0, _api, PORT_E_PARAM_POINTER ); \
        return; \
    }

#else
#define VALIDATE_PARAM_CONFIG(_ptr,_api)
#define VALIDATE_STATE_INIT(_api)
#define VALIDATE_PARAM_PIN(_pin, _api)
#define VALIDATE_PARAM_POINTER(_pointer,_api)
#endif

#define BANK0_WIDTH 32
#define BANK1_WIDTH 22
#define BANK2_WIDTH 32
#define BANK3_WIDTH 32

static void maskWrite( uint32 *reg, uint32 mask, uint32 val) {
    *reg = ( val & mask ) | ( *reg & ~mask );
}

/** @req SWS_Port_00001 */
/** @req SWS_Port_00043 */
/** @req SWS_Port_00214 All I/O registers are initialized by port */
/** @req SWS_Port_00215 */
/** @req SWS_Port_00217 Only I/O registers are initialized by port */
/** @req SWS_Port_00218 Only I/O registers are initialized by port */
/** @req SWS_Port_00042 */
/** @req SWS_Port_00041 */
/** !req SWS_Port_00055 */
/** !req SWS_Port_00005 */

void Port_Init(const Port_ConfigType *config) {
    /* @req SWS_Port_00105 */
    VALIDATE_PARAM_CONFIG(config, PORT_INIT_ID);

    PORT_ENTER_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Disable interrupts */
    /* Unlock SLCR registers */
    SLCR.SLCR_UNLOCK = 0x0000DF0DU; /* Unlock SLCR registers */

    /* @req SWS_Port_00041 */
    // Copy config to register areas
    for (uint32 i = 0; i < NO_OF_PORTS; i++) {
    	if (config->PortConfig[i].reg < MIO_PIN_MAX) {
    		/* Only MIO ports need to be configured here. */
    		SLCR.MIO_PIN[config->PortConfig[i].reg] = config->PortConfig[i].config;
    	}
    }
    /* Lock SLCR register */
    maskWrite((uint32 *)&SLCR.SLCR_LOCK, 0x0000FFFFU ,0x0000767BU);
    PORT_EXIT_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Enable interrupts */

    GPIO_REG.DIRM_0 = config->OutConfig[0].dirm;
    GPIO_REG.OEN_0  = config->OutConfig[0].oen;
    GPIO_REG.MASK_DATA_0_LSW = config->OutConfig[0].mask_data_lsw;
    GPIO_REG.MASK_DATA_0_MSW = config->OutConfig[0].mask_data_msw;

    GPIO_REG.DIRM_1 = config->OutConfig[1].dirm;
    GPIO_REG.OEN_1  = config->OutConfig[1].oen;
    GPIO_REG.MASK_DATA_1_LSW = config->OutConfig[1].mask_data_lsw;
    GPIO_REG.MASK_DATA_1_MSW = config->OutConfig[1].mask_data_msw;

    GPIO_REG.DIRM_2 = config->OutConfig[2].dirm;
    GPIO_REG.OEN_2  = config->OutConfig[2].oen;
    GPIO_REG.MASK_DATA_2_LSW = config->OutConfig[2].mask_data_lsw;
    GPIO_REG.MASK_DATA_2_MSW = config->OutConfig[2].mask_data_msw;

    GPIO_REG.DIRM_3 = config->OutConfig[3].dirm;
    GPIO_REG.OEN_3  = config->OutConfig[3].oen;
    GPIO_REG.MASK_DATA_3_LSW = config->OutConfig[3].mask_data_lsw;
    GPIO_REG.MASK_DATA_3_MSW = config->OutConfig[3].mask_data_msw;

    Port_configPtr = config;
    /** @req SWS_Port_00002 */
    Port_state = PORT_INITIALIZED;
}

#if ( PORT_SET_PIN_DIRECTION_API == STD_ON )
/** @req SWS_Port_00138 */
/** @req SWS_Port_00054 */
void Port_SetPinDirection( Port_PinType pin, Port_PinDirectionType direction )
{
    VALIDATE_STATE_INIT(PORT_SET_PIN_DIRECTION_ID);
    VALIDATE_EMIO_PARAM_PIN(pin, PORT_SET_PIN_DIRECTION_ID);

    if (pin < MIO_PIN_MAX)
    {
    	/* Handle MIO ports */

    	Port_PinType pinno;

    PORT_ENTER_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Disable interrupts */
    	SLCR.SLCR_UNLOCK = 0x0000DF0DU; /* Unlock SLCR registers */

    	/** @req SWS_Port_00063 */
    	if (direction==PORT_PIN_IN) {
    		SLCR.MIO_PIN[pin] |= 1; //Enable tri-state
    		//Reset direction and output enable
    		if (pin < BANK0_WIDTH) {
    			GPIO_REG.DIRM_0 = (GPIO_REG.DIRM_0 & ~((uint32)1 << pin));
    			GPIO_REG.OEN_0  = (GPIO_REG.OEN_0  & ~((uint32)1 << pin));
    		} else {
    			pinno = pin - BANK0_WIDTH;
    			GPIO_REG.DIRM_1 = (GPIO_REG.DIRM_1 & ~((uint32)1 << pinno));
    			GPIO_REG.OEN_1  = (GPIO_REG.OEN_1  & ~((uint32)1 << pinno));
    		}
    	}
    	else {
    		SLCR.MIO_PIN[pin] &= PORT_DIR_MASK; //Disable Tristate
    		// Set direction and output enable
    		if (pin < BANK0_WIDTH) {
    			GPIO_REG.DIRM_0 = (GPIO_REG.DIRM_0 | (1 << pin));
    			GPIO_REG.OEN_0  = (GPIO_REG.OEN_0  | (1 << pin));
    		} else {
    			pinno = pin - BANK0_WIDTH;
    			GPIO_REG.DIRM_1 = (GPIO_REG.DIRM_1 | (1 << pinno));
    			GPIO_REG.OEN_1  = (GPIO_REG.OEN_1  | (1 << pinno));
    		}
    	}

    	/* Lock SLCR register */
    	maskWrite((uint32 *)&SLCR.SLCR_LOCK, 0x0000FFFFU ,0x0000767BU);
    PORT_EXIT_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Enable interrupts */
    }
    else
    {
    	/* Handle EMIO ports */

    	Port_PinType pinno = pin - MIO_PIN_MAX;

       	/** @req SWS_Port_00063 */
        	if (direction==PORT_PIN_IN) {

        		//Reset direction and output enable
        		if (pinno < BANK2_WIDTH) {
        			GPIO_REG.DIRM_2 = (GPIO_REG.DIRM_2 & ~((uint32)1 << pinno));
        			GPIO_REG.OEN_2  = (GPIO_REG.OEN_2  & ~((uint32)1 << pinno));
        		} else {
        			pinno = pinno - BANK2_WIDTH;
        			GPIO_REG.DIRM_3 = (GPIO_REG.DIRM_3 & ~((uint32)1 << pinno));
        			GPIO_REG.OEN_3  = (GPIO_REG.OEN_3  & ~((uint32)1 << pinno));
        		}
        	}
        	else {
        		// Set direction and output enable
        		if (pinno < BANK2_WIDTH) {
        			GPIO_REG.DIRM_2 = (GPIO_REG.DIRM_2 | (1 << pinno));
        			GPIO_REG.OEN_2  = (GPIO_REG.OEN_2  | (1 << pinno));
        		} else {
        			pinno = pinno - BANK2_WIDTH;
        			GPIO_REG.DIRM_3 = (GPIO_REG.DIRM_3 | (1 << pinno));
        			GPIO_REG.OEN_3  = (GPIO_REG.OEN_3  | (1 << pinno));
        		}
        	}
    }
}
#endif

/** !req SWS_Port_00066 */
void Port_RefreshPortDirection(void) {
    VALIDATE_STATE_INIT(PORT_REFRESH_PORT_DIRECTION_ID);

    PORT_ENTER_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Disable interrupts */
    SLCR.SLCR_UNLOCK = 0x0000DF0DU; /* Unlock SLCR registers */

    // Lock interrupts
    /** @req SWS_Port_00060 */
    /** !req SWS_Port_00061 Event port pin changeable parameters are refreshed */
    for (uint32 i = 0; i < NO_OF_PORTS; i++) {
    	if (Port_configPtr->PortConfig[i].reg < MIO_PIN_MAX) {

			//Refresh tri-state enable setting to default
			SLCR.MIO_PIN[Port_configPtr->PortConfig[i].reg] = (PORT_DIR_MASK & Port_configPtr->PortConfig[i].config)
															   | (~(PORT_DIR_MASK) & Port_configPtr->PortConfig[i].config);
    	}
    }
    //Set directions and output enable for GPIO
    GPIO_REG.DIRM_0 = Port_configPtr->OutConfig[0].dirm;
    GPIO_REG.OEN_0  = Port_configPtr->OutConfig[0].oen;
    GPIO_REG.DIRM_1 = Port_configPtr->OutConfig[1].dirm;
    GPIO_REG.OEN_1  = Port_configPtr->OutConfig[1].oen;
    GPIO_REG.DIRM_2 = Port_configPtr->OutConfig[2].dirm;
    GPIO_REG.OEN_2  = Port_configPtr->OutConfig[2].oen;
    GPIO_REG.DIRM_3 = Port_configPtr->OutConfig[3].dirm;
    GPIO_REG.OEN_3  = Port_configPtr->OutConfig[3].oen;

    // Restore interrupts

    /* Lock SLCR register */
    maskWrite((uint32 *)&SLCR.SLCR_LOCK, 0x0000FFFFU ,0x0000767BU);
    PORT_EXIT_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Enable interrupts */
}

#if PORT_VERSION_INFO_API == STD_ON
/** @req SWS_Port_00225 */
/** @req SWS_Port_00087 */

void Port_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    VALIDATE_PARAM_POINTER(versioninfo, PORT_GET_VERSION_INFO_ID);
    versioninfo->vendorID = PORT_VENDOR_ID;
    versioninfo->moduleID = PORT_MODULE_ID;
    versioninfo->sw_major_version = PORT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = PORT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = PORT_SW_PATCH_VERSION;
}
#endif

#if (PORT_SET_PIN_MODE_API == STD_ON)
/** @req SWS_Port_00128 */
/** !req SWS_Port_00077*/
/** @req SWS_Port_00212 */
/** @req SWS_Port_00087 PORT_E_MODE_UNCHANGEABLE and PORT_E_PARAM_INVALID_MODE not available */
void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode)
{
    uint8_t pinno;
    uint32 mask;

    boolean dir; //Tri-State enable

    /** @req SWS_Port_00125 */
    VALIDATE_STATE_INIT(PORT_SET_PIN_MODE_ID);

    /* This API is only valid for MIO channels. Validate this. */
    VALIDATE_PARAM_PIN(Pin, PORT_SET_PIN_MODE_ID);

    PORT_ENTER_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Disable interrupts */
    SLCR.SLCR_UNLOCK = 0x0000DF0DU; /* Unlock SLCR registers */

    //The port control configuration registers (SLCR_MIO_PIN_XXX) in the SLCR allow software control of the static electrical
    //characteristics of external pins. The multiplexer level selection, selection of pullup
    // devices, the slew rate of I/O signals, input disable for HSTL and direction.
    SLCR.MIO_PIN[Pin] = Mode;

    dir = (1 & Mode);
    pinno = Pin % BANK0_WIDTH;
    mask = (dir ? ~((uint32)(1<<pinno)) :(uint32) (1<<pinno));

    //Check if GPIO bank0 or 1. Set the direction and output enables based on the Tri-state bit in Mode
    if (Pin < BANK0_WIDTH) {
        GPIO_REG.DIRM_0 = dir ? (GPIO_REG.DIRM_0 & mask) : (GPIO_REG.DIRM_0 | mask);
        GPIO_REG.OEN_0  = dir ? (GPIO_REG.OEN_0 & mask) : (GPIO_REG.OEN_0 | mask);
    }
    else {
        GPIO_REG.DIRM_1 = dir ? (GPIO_REG.DIRM_1 & mask) : (GPIO_REG.DIRM_1 | mask);
        GPIO_REG.OEN_1  = dir ? (GPIO_REG.OEN_1 & mask) : (GPIO_REG.OEN_1 | mask);
    }

    /* Lock SLCR register */
    maskWrite((uint32 *)&SLCR.SLCR_LOCK, 0x0000FFFFU ,0x0000767BU);
    PORT_EXIT_EXCLUSIVE_AREA(EXCLUSIVE_AREA_0); /* Enable interrupts */
}
#endif

