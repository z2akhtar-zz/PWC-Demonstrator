/*
 * Generator version: 2.0.0
 * AUTOSAR version:   4.1.2
 */

#include "Mcu.h"
 



const Mcu_ClockSettingConfigType Mcu_ClockSettingConfigData[] = {
   {
      .McuClockReferencePointFrequency = 25000000UL,
	  .Pll1    = 9,
      .Pll2    = 8,
      .Pll3    = 0,
   }
};

const Mcu_PerClockConfigType McuPerClockConfigData =
{ 
.AHBClocksEnable  = RCC_AHBPeriph_DMA1 ,
.APB1ClocksEnable = RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2 | RCC_APB1Periph_TIM2 ,
.APB2ClocksEnable = RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB | RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC };

const Mcu_ConfigType McuConfigData[] = {
   {
      .McuNumberOfMcuModes = 3u,
      .McuRamSectors = 0u,
      .McuClockSettings = 1u,
      .McuDefaultClockSettings = McuConf_McuClockSettingConfig_McuClockSettingConfig, 
      .McuClockSettingConfig = &Mcu_ClockSettingConfigData[0],	

      .McuRamSectorSettingConfig = NULL

   }
};

