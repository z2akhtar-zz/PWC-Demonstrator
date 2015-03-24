/**
  ******************************************************************************
  * @file    stm3210c_eval_ioe.h
  * @author  MCD Application Team
  * @version V4.5.0
  * @date    07-March-2011
  * @brief   This file contains all the functions prototypes for the IO Expander
  *   firmware driver.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************  
  */ 

  /* File Info : ---------------------------------------------------------------
    SUPPORTED FEATURES:
      - IO Read/write : Set/Reset and Read (Polling/Interrupt)
      - Joystick: config and Read (Polling/Interrupt)
      - Touch Screen Features: Single point mode (Polling/Interrupt)
      - TempSensor Feature: accuracy not determined (Polling).

    UNSUPPORTED FEATURES:
      - Row ADC Feature is not supported (not implemented on STM3210C-EVAL board)
  ----------------------------------------------------------------------------*/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM3210C_EVAL_IOE_H
#define __STM3210C_EVAL_IOE_H

#ifdef __cplusplus
 extern "C" {
#endif   
   
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"


/** @addtogroup Utilities
  * @{
  */

/** @addtogroup STM32_EVAL
  * @{
  */ 

/** @addtogroup STM3210C_EVAL
  * @{
  */
    
/** @defgroup STM3210C_EVAL_IOE 
  * @{
  */ 

/** @defgroup STM3210C_EVAL_IOE_Exported_Types
  * @{
  */ 

/** 
  * @brief  Touch Screen Information structure  
  */ 
typedef struct
{
  uint16_t TouchDetected;
  uint16_t X;
  uint16_t Y;
  uint16_t Z;
}TS_STATE; 
  
/** 
  * @brief  Joystick State definitions  
  */ 
#ifndef __STM32_EVAL_H
typedef enum 
{ 
  JOY_NONE = 0,
  JOY_SEL = 1,
  JOY_DOWN = 2,
  JOY_LEFT = 3,
  JOY_RIGHT = 4,
  JOY_UP = 5
} JOYState_TypeDef
;
#endif /* __STM32_EVAL_H */
 
/** 
  * @brief  IO_Expander Error codes  
  */ 
typedef enum
{
  IOE_OK = 0,
  IOE_FAILURE, 
  IOE_TIMEOUT,
  PARAM_ERROR,
  IOE1_NOT_OPERATIONAL, 
  IOE2_NOT_OPERATIONAL
}IOE_Status_TypDef;

/** 
  * @brief  IO bit values  
  */ 
typedef enum
{
  BitReset = 0,
  BitSet = 1
}IOE_BitValue_TypeDef;

/** 
  * @brief  IOE DMA Direction  
  */ 
typedef enum
{
  IOE_DMA_TX = 0,
  IOE_DMA_RX = 1
}IOE_DMADirection_TypeDef;

/**
  * @}
  */ 

#define Button_WAKEUP        BUTTON_WAKEUP
#define Button_TAMPER        BUTTON_TAMPER
#define Button_KEY           BUTTON_KEY
#define Button_RIGHT         BUTTON_RIGHT
#define Button_LEFT          BUTTON_LEFT
#define Button_UP            BUTTON_UP
#define Button_DOWN          BUTTON_DOWN
#define Button_SEL           BUTTON_SEL
#define Mode_GPIO            BUTTON_MODE_GPIO
#define Mode_EXTI            BUTTON_MODE_EXTI
#define Button_Mode_TypeDef  ButtonMode_TypeDef
#define JOY_CENTER           JOY_SEL
#define JOY_State_TypeDef    JOYState_TypeDef


#define LCD_RSNWR_GPIO_CLK  LCD_NWR_GPIO_CLK
#define LCD_SPI_GPIO_PORT   LCD_SPI_SCK_GPIO_PORT
#define LCD_SPI_GPIO_CLK    LCD_SPI_SCK_GPIO_CLK
#define R0                  LCD_REG_0
#define R1                  LCD_REG_1
#define R2                  LCD_REG_2
#define R3                  LCD_REG_3
#define R4                  LCD_REG_4
#define R5                  LCD_REG_5
#define R6                  LCD_REG_6
#define R7                  LCD_REG_7
#define R8                  LCD_REG_8
#define R9                  LCD_REG_9
#define R10                 LCD_REG_10
#define R12                 LCD_REG_12
#define R13                 LCD_REG_13
#define R14                 LCD_REG_14
#define R15                 LCD_REG_15
#define R16                 LCD_REG_16
#define R17                 LCD_REG_17
#define R18                 LCD_REG_18
#define R19                 LCD_REG_19
#define R20                 LCD_REG_20
#define R21                 LCD_REG_21
#define R22                 LCD_REG_22
#define R23                 LCD_REG_23
#define R24                 LCD_REG_24
#define R25                 LCD_REG_25
#define R26                 LCD_REG_26
#define R27                 LCD_REG_27
#define R28                 LCD_REG_28
#define R29                 LCD_REG_29
#define R30                 LCD_REG_30
#define R31                 LCD_REG_31
#define R32                 LCD_REG_32
#define R33                 LCD_REG_33
#define R34                 LCD_REG_34
#define R36                 LCD_REG_36
#define R37                 LCD_REG_37
#define R40                 LCD_REG_40
#define R41                 LCD_REG_41
#define R43                 LCD_REG_43
#define R45                 LCD_REG_45
#define R48                 LCD_REG_48
#define R49                 LCD_REG_49
#define R50                 LCD_REG_50
#define R51                 LCD_REG_51
#define R52                 LCD_REG_52
#define R53                 LCD_REG_53
#define R54                 LCD_REG_54
#define R55                 LCD_REG_55
#define R56                 LCD_REG_56
#define R57                 LCD_REG_57
#define R59                 LCD_REG_59
#define R60                 LCD_REG_60
#define R61                 LCD_REG_61
#define R62                 LCD_REG_62
#define R63                 LCD_REG_63
#define R64                 LCD_REG_64
#define R65                 LCD_REG_65
#define R66                 LCD_REG_66
#define R67                 LCD_REG_67
#define R68                 LCD_REG_68
#define R69                 LCD_REG_69
#define R70                 LCD_REG_70
#define R71                 LCD_REG_71
#define R72                 LCD_REG_72
#define R73                 LCD_REG_73
#define R74                 LCD_REG_74
#define R75                 LCD_REG_75
#define R76                 LCD_REG_76
#define R77                 LCD_REG_77
#define R78                 LCD_REG_78
#define R79                 LCD_REG_79
#define R80                 LCD_REG_80
#define R81                 LCD_REG_81
#define R82                 LCD_REG_82
#define R83                 LCD_REG_83
#define R96                 LCD_REG_96
#define R97                 LCD_REG_97
#define R106                LCD_REG_106
#define R118                LCD_REG_118
#define R128                LCD_REG_128
#define R129                LCD_REG_129
#define R130                LCD_REG_130
#define R131                LCD_REG_131
#define R132                LCD_REG_132
#define R133                LCD_REG_133
#define R134                LCD_REG_134
#define R135                LCD_REG_135
#define R136                LCD_REG_136
#define R137                LCD_REG_137
#define R139                LCD_REG_139
#define R140                LCD_REG_140
#define R141                LCD_REG_141
#define R143                LCD_REG_143
#define R144                LCD_REG_144
#define R145                LCD_REG_145
#define R146                LCD_REG_146
#define R147                LCD_REG_147
#define R148                LCD_REG_148
#define R149                LCD_REG_149
#define R150                LCD_REG_150
#define R151                LCD_REG_151
#define R152                LCD_REG_152
#define R153                LCD_REG_153
#define R154                LCD_REG_154
#define R157                LCD_REG_157
#define R192                LCD_REG_192
#define R193                LCD_REG_193
#define R227                LCD_REG_227
#define R229                LCD_REG_229
#define R231                LCD_REG_231
#define R239                LCD_REG_239
#define White               LCD_COLOR_WHITE
#define Black               LCD_COLOR_BLACK
#define Grey                LCD_COLOR_GREY
#define Blue                LCD_COLOR_BLUE
#define Blue2               LCD_COLOR_BLUE2
#define Red                 LCD_COLOR_RED
#define Magenta             LCD_COLOR_MAGENTA
#define Green               LCD_COLOR_GREEN
#define Cyan                LCD_COLOR_CYAN
#define Yellow              LCD_COLOR_YELLOW
#define Line0               LCD_LINE_0
#define Line1               LCD_LINE_1
#define Line2               LCD_LINE_2
#define Line3               LCD_LINE_3
#define Line4               LCD_LINE_4
#define Line5               LCD_LINE_5
#define Line6               LCD_LINE_6
#define Line7               LCD_LINE_7
#define Line8               LCD_LINE_8
#define Line9               LCD_LINE_9
#define Horizontal          LCD_DIR_HORIZONTAL
#define Vertical            LCD_DIR_VERTICAL

/** @defgroup STM3210C_EVAL_IOE_Exported_Constants
  * @{
  */ 

/**
 * @brief Uncomment the line below to enable verfying each written byte in write
 *        operation. The I2C_WriteDeviceRegister() function will then compare the
 *        written and read data and return error status if a mismatch occurs.
 */
/* #define VERIFY_WRITTENDATA */

/**
 * @brief Uncomment the line below if you want to use user defined Delay function
 *        (for precise timing), otherwise default _delay_ function defined within
 *         this driver is used (less precise timing).  
 */
/* #define USE_Delay */

/**
 * @brief Uncomment the line below if you want to use user timeout callback.
 *        Function prototypes is declared in this file but function body may be
 *        implemented into user application.  
 */
/* #define USE_TIMEOUT_USER_CALLBACK */

#ifdef USE_Delay
#include "main.h"
 
  #define _delay_     Delay  /* !< User can provide more timing precise _delay_ function
                                   (with 10ms time base), using SysTick for example */
#else
  #define _delay_     delay      /* !< Default _delay_ function with less precise timing */
#endif    

/*------------------------------------------------------------------------------
    Hardware Configuration 
------------------------------------------------------------------------------*/
/** 
  * @brief  I2C port definitions  
  */
#define IOE_I2C                          I2C1
#define IOE_I2C_CLK                      RCC_APB1Periph_I2C1
#define IOE_I2C_SCL_PIN                  GPIO_Pin_6
#define IOE_I2C_SCL_GPIO_PORT            GPIOB
#define IOE_I2C_SCL_GPIO_CLK             RCC_APB2Periph_GPIOB
#define IOE_I2C_SDA_PIN                  GPIO_Pin_7
#define IOE_I2C_SDA_GPIO_PORT            GPIOB
#define IOE_I2C_SDA_GPIO_CLK             RCC_APB2Periph_GPIOB
#define IOE_I2C_DR                       ((uint32_t)0x40005410)
#define IOE_I2C_SPEED                    300000  

/** 
  * @brief  IOE DMA definitions  
  */
#define IOE_DMA                          DMA1
#define IOE_DMA_CLK                      RCC_AHBPeriph_DMA1
#define IOE_DMA_TX_CHANNEL               DMA1_Channel6
#define IOE_DMA_RX_CHANNEL               DMA1_Channel7
#define IOE_DMA_TX_TCFLAG                DMA1_FLAG_TC6
#define IOE_DMA_RX_TCFLAG                DMA1_FLAG_TC7


/** 
  * @brief  IO Expander Interrupt line on EXTI  
  */ 
#define IOE_IT_PIN                       GPIO_Pin_14
#define IOE_IT_GPIO_PORT                 GPIOB
#define IOE_IT_GPIO_CLK                  RCC_APB2Periph_GPIOB
#define IOE_IT_EXTI_PORT_SOURCE          GPIO_PortSourceGPIOB
#define IOE_IT_EXTI_PIN_SOURCE           GPIO_PinSource14
#define IOE_IT_EXTI_LINE                 EXTI_Line14
#define IOE_IT_EXTI_IRQn                 EXTI15_10_IRQn       

/**
  * @brief Eval Board IO Pins definition 
  */ 
#define AUDIO_RESET_PIN             IO_Pin_2 /* IO_Exapnader_2 */ /* Output */
#define MII_INT_PIN                 IO_Pin_0 /* IO_Exapnader_2 */ /* Output */
#define VBAT_DIV_PIN                IO_Pin_0 /* IO_Exapnader_1 */ /* Output */
#define MEMS_INT1_PIN               IO_Pin_3 /* IO_Exapnader_1 */ /* Input */
#define MEMS_INT2_PIN               IO_Pin_2 /* IO_Exapnader_1 */ /* Input */

 
/**
  * @brief Eval Board both IO Exapanders Pins definition 
  */ 
#define IO1_IN_ALL_PINS          (uint32_t)(MEMS_INT1_PIN | MEMS_INT2_PIN)
#define IO2_IN_ALL_PINS          (uint32_t)(JOY_IO_PINS)
#define IO1_OUT_ALL_PINS         (uint32_t)(VBAT_DIV_PIN)
#define IO2_OUT_ALL_PINS         (uint32_t)(AUDIO_RESET_PIN | MII_INT_PIN)

/** 
  * @brief  The 7 bits IO Expanders adresses and chip IDs  
  */ 
#define IOE_1_ADDR                 0x82    
#define IOE_2_ADDR                 0x88    
#define STMPE811_ID                0x0811


/*------------------------------------------------------------------------------
    Functional and Interrupt Management
------------------------------------------------------------------------------*/
/** 
  * @brief  IO Expander Functionalities definitions  
  */ 
#define IOE_ADC_FCT              0x01
#define IOE_TS_FCT               0x02
#define IOE_IO_FCT               0x04
#define IOE_TEMPSENS_FCT         0x08

/** 
  * @brief  Interrupt source configuration definitons  
  */ 
#define IOE_ITSRC_TSC           0x01  /* IO_Exapnder 1 */
#define IOE_ITSRC_INMEMS        0x02  /* IO_Exapnder 1 */
#define IOE_ITSRC_JOYSTICK      0x04  /* IO_Exapnder 2 */
#define IOE_ITSRC_TEMPSENS      0x08  /* IO_Exapnder 2 */

/** 
  * @brief  Glaobal Interrupts definitions  
  */ 
#define IOE_GIT_GPIO             0x80
#define IOE_GIT_ADC              0x40
#define IOE_GIT_TEMP             0x20
#define IOE_GIT_FE               0x10
#define IOE_GIT_FF               0x08
#define IOE_GIT_FOV              0x04
#define IOE_GIT_FTH              0x02
#define IOE_GIT_TOUCH            0x01


/*------------------------------------------------------------------------------
    STMPE811 device register definition
------------------------------------------------------------------------------*/
/** 
  * @brief  Identification registers  
  */ 
#define IOE_REG_CHP_ID             0x00
#define IOE_REG_ID_VER             0x02

/** 
  * @brief  General Control Registers  
  */ 
#define IOE_REG_SYS_CTRL1          0x03
#define IOE_REG_SYS_CTRL2          0x04
#define IOE_REG_SPI_CFG            0x08 

/** 
  * @brief  Interrupt Control register  
  */ 
#define IOE_REG_INT_CTRL           0x09
#define IOE_REG_INT_EN             0x0A
#define IOE_REG_INT_STA            0x0B
#define IOE_REG_GPIO_INT_EN        0x0C
#define IOE_REG_GPIO_INT_STA       0x0D

/** 
  * @brief  GPIO Registers  
  */ 
#define IOE_REG_GPIO_SET_PIN       0x10
#define IOE_REG_GPIO_CLR_PIN       0x11
#define IOE_REG_GPIO_MP_STA        0x12
#define IOE_REG_GPIO_DIR           0x13
#define IOE_REG_GPIO_ED            0x14
#define IOE_REG_GPIO_RE            0x15
#define IOE_REG_GPIO_FE            0x16
#define IOE_REG_GPIO_AF            0x17

/** 
  * @brief  ADC Registers  
  */ 
#define IOE_REG_ADC_INT_EN         0x0E
#define IOE_REG_ADC_INT_STA        0x0F
#define IOE_REG_ADC_CTRL1          0x20
#define IOE_REG_ADC_CTRL2          0x21
#define IOE_REG_ADC_CAPT           0x22
#define IOE_REG_ADC_DATA_CH0       0x30 /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH1       0x32 /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH2       0x34 /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH3       0x36 /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH4       0x38 /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH5       0x3A /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH6       0x3B /* 16-Bit register */
#define IOE_REG_ADC_DATA_CH7       0x3C /* 16-Bit register */ 

/** 
  * @brief  TouchScreen Registers  
  */ 
#define IOE_REG_TSC_CTRL           0x40
#define IOE_REG_TSC_CFG            0x41
#define IOE_REG_WDM_TR_X           0x42 
#define IOE_REG_WDM_TR_Y           0x44
#define IOE_REG_WDM_BL_X           0x46
#define IOE_REG_WDM_BL_Y           0x48
#define IOE_REG_FIFO_TH            0x4A
#define IOE_REG_FIFO_STA           0x4B
#define IOE_REG_FIFO_SIZE          0x4C
#define IOE_REG_TSC_DATA_X         0x4D 
#define IOE_REG_TSC_DATA_Y         0x4F
#define IOE_REG_TSC_DATA_Z         0x51
#define IOE_REG_TSC_DATA_XYZ       0x52 
#define IOE_REG_TSC_FRACT_XYZ      0x56
#define IOE_REG_TSC_DATA           0x57
#define IOE_REG_TSC_I_DRIVE        0x58
#define IOE_REG_TSC_SHIELD         0x59

/** 
  * @brief  Temperature Sensor registers  
  */ 
#define IOE_REG_TEMP_CTRL          0x60
#define IOE_REG_TEMP_DATA          0x61
#define IOE_REG_TEMP_TH            0x62


/*------------------------------------------------------------------------------
    Functions parameters defines
------------------------------------------------------------------------------*/
/**
  * @brief Touch Screen Pins definition 
  */ 
#define TOUCH_YD                    IO_Pin_1 /* IO_Exapnader_1 */ /* Input */
#define TOUCH_XD                    IO_Pin_2 /* IO_Exapnader_1 */ /* Input */
#define TOUCH_YU                    IO_Pin_3 /* IO_Exapnader_1 */ /* Input */
#define TOUCH_XU                    IO_Pin_4 /* IO_Exapnader_1 */ /* Input */
#define TOUCH_IO_ALL                (uint32_t)(IO_Pin_1 | IO_Pin_2 | IO_Pin_3 | IO_Pin_4)

/**
  * @brief  JOYSTICK Pins definition 
  */ 
#define JOY_IO_SEL                   IO_Pin_7
#define JOY_IO_DOWN                  IO_Pin_6
#define JOY_IO_LEFT                  IO_Pin_5
#define JOY_IO_RIGHT                 IO_Pin_4
#define JOY_IO_UP                    IO_Pin_3
#define JOY_IO_NONE                  JOY_IO_PINS
#define JOY_IO_PINS                  (uint32_t)(IO_Pin_3 | IO_Pin_4 | IO_Pin_5 | IO_Pin_6 | IO_Pin_7)

/** 
  * @brief  IO Pins  
  */ 
#define IO_Pin_0                 0x01
#define IO_Pin_1                 0x02
#define IO_Pin_2                 0x04
#define IO_Pin_3                 0x08
#define IO_Pin_4                 0x10
#define IO_Pin_5                 0x20
#define IO_Pin_6                 0x40
#define IO_Pin_7                 0x80
#define IO_Pin_ALL               0xFF

/** 
  * @brief  IO Pin directions  
  */ 
#define Direction_IN             0x00
#define Direction_OUT            0x01

/** 
  * @brief  Interrupt Line output parameters  
  */ 
#define Polarity_Low             0x00
#define Polarity_High            0x04
#define Type_Level               0x00
#define Type_Edge                0x02

/** 
  * @brief IO Interrupts  
  */ 
#define IO_IT_0                  0x01
#define IO_IT_1                  0x02
#define IO_IT_2                  0x04
#define IO_IT_3                  0x08
#define IO_IT_4                  0x10
#define IO_IT_5                  0x20
#define IO_IT_6                  0x40
#define IO_IT_7                  0x80
#define ALL_IT                   0xFF
#define IOE_JOY_IT               (uint8_t)(IO_IT_3 | IO_IT_4 | IO_IT_5 | IO_IT_6 | IO_IT_7)
#define IOE_TS_IT                (uint8_t)(IO_IT_0 | IO_IT_1 | IO_IT_2)
#define IOE_INMEMS_IT            (uint8_t)(IO_IT_2 | IO_IT_3)

/** 
  * @brief  Edge detection value  
  */ 
#define EDGE_FALLING              0x01
#define EDGE_RISING               0x02

/** 
  * @brief  Global interrupt Enable bit  
  */ 
#define IOE_GIT_EN                0x01

/**
  * @}
  */ 



/** @defgroup STM3210C_EVAL_IOE_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 



/** @defgroup STM3210C_EVAL_IOE_Exported_Functions
  * @{
  */ 

/** 
  * @brief  Configuration and initialization functions  
  */
uint8_t IOE_Config(void);
uint8_t IOE_ITConfig(uint32_t IOE_ITSRC_Source);

/** 
  * @brief  Timeout user callback function. This function is called when a timeout
  *         condition occurs during communication with IO Expander. Only protoype
  *         of this function is decalred in IO Expander driver. Its implementation
  *         may be done into user application. This function may typically stop
  *         current operations and reset the I2C peripheral and IO Expander.
  *         To enable this function use uncomment the define USE_TIMEOUT_USER_CALLBACK
  *         at the top of this file.          
  */
#ifdef USE_TIMEOUT_USER_CALLBACK 
 uint8_t IOE_TimeoutUserCallback(void);
#else
 #define IOE_TimeoutUserCallback()  IOE_TIMEOUT
#endif /* USE_TIMEOUT_USER_CALLBACK */

/** 
  * @brief IO pins control functions
  */
uint8_t IOE_WriteIOPin(uint8_t IO_Pin, IOE_BitValue_TypeDef BitVal);
uint8_t IOE_ReadIOPin(uint32_t IO_Pin);
JOYState_TypeDef
 IOE_JoyStickGetState(void);

/** 
  * @brief Touch Screen controller functions
  */
TS_STATE* IOE_TS_GetState(void);

/** 
  * @brief Interrupts Mangement functions
  */
FlagStatus IOE_GetGITStatus(uint8_t DeviceAddr, uint8_t Global_IT);
uint8_t IOE_ClearGITPending(uint8_t DeviceAddr, uint8_t IO_IT);
FlagStatus IOE_GetIOITStatus(uint8_t DeviceAddr, uint8_t IO_IT);
uint8_t IOE_ClearIOITPending(uint8_t DeviceAddr, uint8_t IO_IT);

/** 
  * @brief Temperature Sensor functions
  */
uint32_t IOE_TempSens_GetData(void);

/** 
  * @brief IO-Expander Control functions
  */
uint8_t IOE_IsOperational(uint8_t DeviceAddr);
uint8_t IOE_Reset(uint8_t DeviceAddr);
uint16_t IOE_ReadID(uint8_t DeviceAddr);

uint8_t IOE_FnctCmd(uint8_t DeviceAddr, uint8_t Fct, FunctionalState NewState);
uint8_t IOE_IOPinConfig(uint8_t DeviceAddr, uint8_t IO_Pin, uint8_t Direction);
uint8_t IOE_GITCmd(uint8_t DeviceAddr, FunctionalState NewState);
uint8_t IOE_GITConfig(uint8_t DeviceAddr, uint8_t Global_IT, FunctionalState NewState);
uint8_t IOE_IOITConfig(uint8_t DeviceAddr, uint8_t IO_IT, FunctionalState NewState);

/** 
  * @brief Low Layer functions
  */
uint8_t IOE_TS_Config(void);
uint8_t IOE_TempSens_Config(void);
uint8_t IOE_IOAFConfig(uint8_t DeviceAddr, uint8_t IO_Pin, FunctionalState NewState);
uint8_t IOE_IOEdgeConfig(uint8_t DeviceAddr, uint8_t IO_Pin, uint8_t Edge);
uint8_t IOE_ITOutConfig(uint8_t Polarity, uint8_t Type);

uint8_t I2C_WriteDeviceRegister(uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t RegisterValue);
uint8_t I2C_ReadDeviceRegister(uint8_t DeviceAddr, uint8_t RegisterAddr);
uint16_t I2C_ReadDataBuffer(uint8_t DeviceAddr, uint32_t RegisterAddr);

#ifdef __cplusplus
}

#endif
#endif /* __STM3210C_EVAL_IOE_H */

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */     
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
