/** @file adc.h
*   @brief ADC Driver Header File
*   @date 11.September.2009
*   @version 1.00.000
*   
*   This file contains:
*   - Definitions
*   - Types
*   - Interface Prototypes
*   .
*   which are relevant for the ADC driver.
*/

/* (c) Texas Instruments 2009, All rights reserved. */


#ifndef __ADC_H__
#define __ADC_H__

/* USER CODE BEGIN (0) */
/* USER CODE END */

/** @def Ambient_Light_Sensor
*   @note This value should be used for selecting ADC channel hooked up 
*	to Ambient Light Sensor
*/
#define Ambient_Light_Sensor 	9U

/** @def Temperature_Sensor
*   @note This value should be used for selecting ADC channel hooked up 
*	to Temperature Sensor
*/
#define Temperature_Sensor 		8U

/* ADC General Definitions */

/** @def adcGROUP0
*   @brief Alias name for ADC event group
*
*   @note This value should be used for API argument @a group
*/
#define adcGROUP0 0U

/** @def adcGROUP1
*   @brief Alias name for ADC group 1
*
*   @note This value should be used for API argument @a group
*/
#define adcGROUP1 1U

/** @def adcGROUP2
*   @brief Alias name for ADC group 2
*
*   @note This value should be used for API argument @a group
*/
#define adcGROUP2 2U

/** @enum adcResolution
*   @brief Alias names for data resolution
*   This enumeration is used to provide alias names for the data resolution:
*     - 12 bit resolution
*     - 10 bit resolution
*     - 8  bit resolution
*/

enum adcResolution
{
    ADC_12_BIT = 0x00000000, /**< Alias for 12 bit data resolution */
    ADC_10_BIT = 0x00000100, /**< Alias for 10 bit data resolution */
    ADC_8_BIT  = 0x00000200  /**< Alias for 8 bit data resolution  */
};

/** @enum adcFiFoStatus
*   @brief Alias names for FiFo status
*   This enumeration is used to provide alias names for the current FiFo states:
*     - FiFo is not full
*     - FiFo is full
*     - FiFo overflow occured
*/

enum adcFiFoStatus
{
    ADC_FIFO_IS_NOT_FULL = 0, /**< Alias for FiFo is not full       */
    ADC_FIFO_IS_FULL     = 1, /**< Alias for FiFo is full           */
    ADC_FIFO_OVERFLOW    = 3  /**< Alias for FiFo overflow occured  */
};

/** @enum adcConversionStatus
*   @brief Alias names for conversion status
*   This enumeration is used to provide alias names for the current conversion states:
*     - Conversion is not finished
*     - Conversion is finished
*/

enum adcConversionStatus
{
    ADC_CONVERSION_IS_NOT_FINISHED = 0, /**< Alias for current conversion is not finished */
    ADC_CONVERSION_IS_FINISHED     = 8  /**< Alias for current conversion is  finished    */
};

/** @enum adcHwTriggerSource
*   @brief Alias names for hardware trigger source
*   This enumeration is used to provide alias names for the hardware trigger sources:
*     - event pin
*     - 10 bit resolution
*     - HET pin 8
*     - HET pin 10
*     - RTI compare 0 match
*     - HET pin 17
*     - HET pin 19 
*     - GIO port b pin 0
*     - GIO port b pin 1
*/

enum adcHwTriggerSource
{
    ADC_EVENT,     /**< Alias for event pin           */
    ADC_HET8,      /**< Alias for HET pin 8           */
    ADC_HET10,     /**< Alias for HET pin 10          */
    ADC_RTI_COMP0, /**< Alias for RTI compare 0 match */
    ADC_HET17,     /**< Alias for HET pin 17          */
    ADC_HET19,     /**< Alias for HET pin 19          */
    ADC_GIOB0,     /**< Alias for GIO port b pin 0    */
    ADC_GIOB1      /**< Alias for GIO port b pin 1    */
};

/* USER CODE BEGIN (1) */
/* USER CODE END */


/** @typedef adcData_t
*   @brief ADC Conversion data structure
*
*   This type is used to pass adc conversion data.
*/
typedef struct tagAdcData
{
    unsigned short id;     /**< Channel/Pin Id        */ //prathap
    unsigned short value;  /**< Conversion data value */
} adcData_t;

/** @typedef adcBASE_t
*   @brief ADC Register Frame Definition
*
*   This type is used to access the ADC Registers.
*/
typedef volatile struct tagAdcBase
{
    unsigned RSTCR;              /**< 0x0000: Reset control register                            */
    unsigned OPMODECR;           /**< 0x0004: Operating mode control register                   */
    unsigned CLOCKCR;            /**< 0x0008: Clock control register                            */
    unsigned CALCR;              /**< 0x000C: Calibration control register                      */
    unsigned GxMODECR[3U];       /**< 0x0010,0x0014,0x0018: Group 0-2 mode control register     */
    unsigned G0SRC;              /**< 0x001C: Group 0 trigger source control register           */
    unsigned G1SRC;              /**< 0x0020: Group 1 trigger source control register           */
    unsigned G2SRC;              /**< 0x0024: Group 2 trigger source control register           */
    unsigned GxINTENA[3U];       /**< 0x0028,0x002C,0x0030: Group 0-2 interrupt enable register */
    unsigned GxINTFLG[3U];       /**< 0x0034,0x0038,0x003C: Group 0-2 interrupt flag register   */
    unsigned GxINTCR[3U];        /**< 0x0040-0x0048: Group 0-2 interrupt threshold register     */
    unsigned G0DMACR;            /**< 0x004C: Group 0 DMA control register                      */
    unsigned G1DMACR;            /**< 0x0050: Group 1 DMA control register                      */
    unsigned G2DMACR;            /**< 0x0054: Group 2 DMA control register                      */
    unsigned BNDCR;              /**< 0x0058: Buffer boundary control register                  */
#ifdef _little_endian__
    unsigned BNDEND  : 16U;      /**< 0x005C: Buffer boundary end register                      */
    unsigned BUFINIT : 16U;      /**< 0x005C: Buffer initialization register                    */
#else
    unsigned BUFINIT : 16U;      /**< 0x005C: Buffer initialization register                    */
    unsigned BNDEND  : 16U;      /**< 0x005C: Buffer boundary end register                      */
#endif
    unsigned G0SAMP;             /**< 0x0060: Group 0 sample window register                    */
    unsigned G1SAMP;             /**< 0x0064: Group 1 sample window register                    */
    unsigned G2SAMP;             /**< 0x0068: Group 2 sample window register                    */
    unsigned G0SR;               /**< 0x006C: Group 0 status register                           */
    unsigned G1SR;               /**< 0x0070: Group 1 status register                           */
    unsigned G2SR;               /**< 0x0074: Group 2 status register                           */
    unsigned GxSEL[3U];          /**< 0x0078-0x007C: Group 0-2 channel select register          */
    unsigned CALR;               /**< 0x0084: Calibration register                              */
    unsigned SMSTATE;            /**< 0x0088: State machine state register                      */
    unsigned LASTCONV;           /**< 0x008C: Last conversion register                          */
    struct
    {
        unsigned BUF0;           /**< 0x0090,0x00B0,0x00D0: Group 0-2 result buffer 1 register  */
        unsigned BUF1;           /**< 0x0094,0x00B4,0x00D4: Group 0-2 result buffer 1 register  */
        unsigned BUF2;           /**< 0x0098,0x00B8,0x00D8: Group 0-2 result buffer 2 register  */
        unsigned BUF3;           /**< 0x009C,0x00BC,0x00DC: Group 0-2 result buffer 3 register  */
        unsigned BUF4;           /**< 0x00A0,0x00C0,0x00E0: Group 0-2 result buffer 4 register  */
        unsigned BUF5;           /**< 0x00A4,0x00C4,0x00E4: Group 0-2 result buffer 5 register  */
        unsigned BUF6;           /**< 0x00A8,0x00C8,0x00E8: Group 0-2 result buffer 6 register  */
        unsigned BUF7;           /**< 0x00AC,0x00CC,0x00EC: Group 0-2 result buffer 7 register  */
    } GxBUF[3U];
    unsigned G0EMUBUFFER;        /**< 0x00F0: Group 0 emulation result buffer                   */
    unsigned G1EMUBUFFER;        /**< 0x00F4: Group 1 emulation result buffer                   */
    unsigned G2EMUBUFFER;        /**< 0x00F8: Group 2 emulation result buffer                   */
    unsigned EVTDIR;             /**< 0x00FC: Event pin direction register                      */
    unsigned EVTOUT;             /**< 0x0100: Event pin digital output register                 */
    unsigned EVTIN;              /**< 0x0104: Event pin digital input register                  */
    unsigned EVTSET;             /**< 0x0108: Event pin set register                            */
    unsigned EVTCLR;             /**< 0x010C: Event pin clear register                          */
    unsigned EVTPDR;             /**< 0x0110: Event pin open drain register                     */
    unsigned EVTDIS;             /**< 0x0114: Event pin pull disable register                   */
    unsigned EVTPSEL;            /**< 0x0118: Event pin pull select register                    */
    unsigned G0SAMPDISEN;        /**< 0x011C: Group 0 sample discharge register                 */
    unsigned G1SAMPDISEN;        /**< 0x0120: Group 1 sample discharge register                 */
    unsigned G2SAMPDISEN;        /**< 0x0124: Group 2 sample discharge register                 */
    unsigned MAGINTCR1;          /**< 0x0128: Magnitude interrupt control register 1            */
    unsigned MAGINT1MASK;        /**< 0x012C: Magnitude interrupt mask register 1               */
    unsigned MAGINTCR2;          /**< 0x0130: Magnitude interrupt control register 2            */
    unsigned MAGINT2MASK;        /**< 0x0134: Magnitude interrupt mask register 2               */
    unsigned MAGINTCR3;          /**< 0x0138: Magnitude interrupt control register 3            */
    unsigned MAGINT3MASK;        /**< 0x013C: Magnitude interrupt mask register 3               */
    unsigned MAGINTCR4;          /**< 0x0140: Magnitude interrupt control register 4            */
    unsigned MAGINT4MASK;        /**< 0x0144: Magnitude interrupt mask register 4               */
    unsigned MAGINTCR5;          /**< 0x0148: Magnitude interrupt control register 5            */
    unsigned MAGINT5MASK;        /**< 0x014C: Magnitude interrupt mask register 5               */
    unsigned MAGINTCR6;          /**< 0x0150: Magnitude interrupt control register 6            */
    unsigned MAGINT6MASK;        /**< 0x0154: Magnitude interrupt mask register 6               */
    unsigned MAGTHRINTENASET;    /**< 0x0158: Magnitude interrupt set register                  */
    unsigned MAGTHRINTENACLR;    /**< 0x015C: Magnitude interrupt clear register                */
    unsigned MAGTHRINTFLG;       /**< 0x0160: Magnitude interrupt flag register                 */
    unsigned MAGTHRINTOFFSET;    /**< 0x0164: Magnitude interrupt offset register               */
    unsigned GxFIFORESETCR[3U];  /**< 0x0168,0x016C,0x0170: Group 0-2 fifo reset register       */
    unsigned G0RAMADDR;          /**< 0x0174: Group 0 RAM pointer register                      */
    unsigned G1RAMADDR;          /**< 0x0178: Group 1 RAM pointer register                      */
    unsigned G2RAMADDR;          /**< 0x017C: Group 2 RAM pointer register                      */
    unsigned PARCR;              /**< 0x0180: Parity control register                           */
    unsigned PARADDR;            /**< 0x0184: Parity error address register                     */
} adcBASE_t;


/** @def adcREG1
*   @brief ADC1 Register Frame Pointer
*
*   This pointer is used by the ADC driver to access the ADC1 registers.
*/
#define adcREG1 ((adcBASE_t *)0xFFF7C000U)

/** @def adcREG2
*   @brief ADC2 Register Frame Pointer
*
*   This pointer is used by the ADC driver to access the ADC2 registers.
*/
#define adcREG2 ((adcBASE_t *)0xFFF7C200U)

/* USER CODE BEGIN (2) */
/* USER CODE END */


/* ADC Interface Functions */

void adcInit(void);
void adcStartConversion(adcBASE_t *adc, unsigned group);
void adcStopConversion(adcBASE_t *adc, unsigned group);
void adcResetFiFo(adcBASE_t *adc, unsigned group);
int  adcGetData(adcBASE_t *adc, unsigned group, adcData_t *data);
int  adcIsFifoFull(adcBASE_t *adc, unsigned group);
int  adcIsConversionComplete(adcBASE_t *adc, unsigned group);
void adcEnableNotification(adcBASE_t *adc, unsigned group);
void adcDisableNotification(adcBASE_t *adc, unsigned group);

/** @fn void adcNotification(adcBASE_t *adc, unsigned group)
*   @brief Group notification
*   @param[in] adc Pointer to ADC node:
*              - adcREG1: ADC1 module pointer
*              - adcREG2: ADC2 module pointer
*   @param[in] group number of ADC node:
*              - adcGROUP0: ADC event group
*              - adcGROUP1: ADC group 1
*              - adcGROUP2: ADC group 2
*
*   @note This function has to be provide by the user.
*/
void adcNotification(adcBASE_t *adc, unsigned group);

/* USER CODE BEGIN (3) */
/* USER CODE END */
void adcStartConversion_selChn(adcBASE_t *adc, unsigned channel, unsigned fifo_size, unsigned group); // Prathap
void MIBADC1_Parity(void);
void MIBADC2_Parity(void);
void Ambient_Light_Sensor_demo(void);
void Temp_Sensor_demo(void);


/** @def adcREG1
*   @brief ADC1 Register Frame Pointer
*
*   This pointer is used by the ADC driver to access the ADC1 registers.
*/
#define adcMEM1 0xFF3E0000U

/** @def adcREG2
*   @brief ADC2 Register Frame Pointer
*
*   This pointer is used by the ADC driver to access the ADC2 registers.
*/
#define adcMEM2 0xFF3A0000U

#define REQENASET0_BASE_ADDR 0xFFFFFE30
#define REQENASET1_BASE_ADDR 0xFFFFFE34

#define REQENACLR0_BASE_ADDR 0xFFFFFE40
#define REQENACLR1_BASE_ADDR 0xFFFFFE44

#define ADC_IOMM_KICK0 0xFFFFEA38
#define ADC_IOMM_KICK1 0xFFFFEA3C
/* USER CODE BEGIN (2) */
/* USER CODE END */

#endif
