/*
 * Generator version: 1.0.0
 * AUTOSAR version:   4.0.3
 */

#include "EcuM.h" 
#include "EcuM_Generated_Types.h" 

#if defined(USE_CANSM) 
extern const CanSM_ConfigType CanSM_Config;
#endif

#if defined(USE_LIN) 
extern const Lin_ConfigType Lin_Config;
#endif

#if defined(USE_LINIF) 
extern const LinIf_ConfigType LinIf_Config;
#endif

#if defined(USE_LINSM) 
extern const LinSM_ConfigType LinSM_Config;
#endif

#if defined(USE_NM)
extern const Nm_ConfigType Nm_Config;
#endif

#if defined(USE_UDPNM)
extern const UdpNm_ConfigType UdpNm_Config;
#endif

#if defined(USE_COMM)
extern const ComM_ConfigType ComM_Config;
#endif

#if defined(USE_BSWM)
extern const BswM_ConfigType  BswM_Config;
#endif

#if defined(USE_J1939TP)
extern const J1939Tp_ConfigType J1939Tp_Config;
#endif
  
#if defined(USE_PDUR) || defined(USE_COM) || defined(USE_CANIF) || defined(USE_CANTP) || defined (USE_CANNM)
extern const PostbuildConfigType Postbuild_Config;
#endif

#if defined(USE_DCM)
extern const Dcm_ConfigType DCM_Config;
#endif  

#if defined(USE_DEM)
extern const Dem_ConfigType DEM_Config;
#endif 

#if defined(USE_ICU)
extern const Icu_ConfigType IcuConfig;
#endif 

#if defined(USE_XCP)
extern const Xcp_ConfigType XcpConfig;
#endif 

const EcuM_SleepModeType EcuM_SleepModeConfig[] = {
	{  // EcuMSleepMode
	   .EcuMSleepModeId = ECUM_SLEEP_MODE_ECUMSLEEPMODE,
	   .EcuMWakeupSourceMask = EcuMConf_EcuMWakeupSource_EcuMWakeupSource,
  	   .EcuMSleepModeMcuMode = McuConf_McuModeSettingConf_RUN,
    }

};


const EcuM_WakeupSourceConfigType EcuM_WakeupSourceConfig[] = {
   {  // EcuMWakeupSource
      .EcuMWakeupSourceId = EcuMConf_EcuMWakeupSource_EcuMWakeupSource,
      .EcuMWakeupSourcePolling = FALSE,
      .EcuMValidationTimeout = ECUM_VALIDATION_TIMEOUT_ILL,
      
#if defined(USE_COMM)
      .EcuMComMChannel = ECUM_COMM_CHANNEL_ILL
#endif
   }
};





const EcuM_ConfigType EcuMConfig = {
    .EcuMPostBuildVariant = 1,
    .EcuMConfigConsistencyHashLow = PRE_COMPILED_DATA_HASH_LOW, 
    .EcuMConfigConsistencyHashHigh = PRE_COMPILED_DATA_HASH_HIGH,  /* @req EcuM2795 */
    .EcuMDefaultShutdownTarget = ECUM_STATE_OFF,
    .EcuMDefaultSleepMode = ECUM_SLEEP_MODE_FIRST,
    .EcuMDefaultAppMode = OSDEFAULTAPPMODE,
    .EcuMNvramReadAllTimeout = ECUM_NVRAM_READALL_TIMEOUT,
    .EcuMNvramWriteAllTimeout = ECUM_NVRAM_WRITEALL_TIMEOUT,
    .EcuMRunMinimumDuration = ECUM_NVRAM_MIN_RUN_DURATION,
    .EcuMNormalMcuMode = McuConf_McuModeSettingConf_NORMAL,
    .EcuMSleepModeConfig = EcuM_SleepModeConfig,
    .EcuMWakeupSourceConfig = EcuM_WakeupSourceConfig,
#if defined(USE_DEM)
	.EcuMDemInconsistencyEventId	= DEM_EVENT_ID_NULL,
	.EcuMDemRamCheckFailedEventId	= DEM_EVENT_ID_NULL,
	.EcuMDemAllRunRequestsKilledEventId	= DEM_EVENT_ID_NULL,
#endif
#if defined(USE_COMM)
    .EcuMComMConfig = NULL,
#endif
#if defined(USE_MCU)
    .McuConfig = McuConfigData,
#endif
#if defined(USE_PORT)
    .PortConfig = &PortConfigData,
#endif
#if defined(USE_CAN)
    .CanConfig =&CanConfigData,
#endif
#if defined(USE_DIO)
    .DioCfg = &DioConfigData,
#endif
#if defined(USE_CANSM)
    .CanSMConfig = &CanSM_Config,
#endif
#if defined(USE_LIN)
    .LinConfig = &Lin_Config,
#endif
#if defined(USE_LINIF)
    .LinIfConfig = &LinIf_Config,
#endif
#if defined(USE_LINSM)
    .LinSMConfig = &LinSM_Config,
#endif
#if defined(USE_UDPNM)
    .UdpNmConfig = &UdpNm_Config,
#endif
#if defined(USE_COMM)
    .ComMConfig = &ComM_Config,
#endif
#if defined(USE_BSWM)
    .BswMConfig = &BswM_Config,
#endif
#if defined(USE_J1939TP)
    .J1939TpConfig = &J1939Tp_Config,
#endif
#if defined(USE_NM)
    .NmConfig = &Nm_Config,
#endif
#if defined(USE_J1939TP)
    .J1939TpConfig = &J1939Tp_Config,
#endif
#if defined(USE_DMA)
    .DmaConfig = DmaConfig,
#endif
#if defined(USE_ADC)
#if defined(CFG_ZYNQ)
    .AdcConfig = NULL,
#else
    .AdcConfig = AdcConfig,
#endif
#endif
#if defined(USE_PWM)
    .PwmConfig = &PwmConfig,
#endif
#if defined(USE_OCU)
	.OcuConfig = &OcuConfig,
#endif
#if defined(USE_ICU)
	.IcuConfig = &IcuConfig,
#endif
#if defined(USE_WDG)
    .WdgConfig = &WdgConfig,
#endif
#if defined(USE_WDGM)
    .WdgMConfig = &WdgMConfig,
#endif
#if defined(USE_WDGIF)
    .WdgIfConfig = &WdgIfConfig,
#endif
#if defined(USE_GPT)
   .GptConfig = GptConfigData,
#endif
#if defined(USE_FLS)
   .FlashConfig = FlsConfigSet, 
#endif
#if defined(USE_EEP)
   .EepConfig = EepConfigData,
#endif
#if defined(USE_SPI)
   .SpiConfig = &SpiConfigData,
#endif
#if defined(USE_DCM)
   .DcmConfig = &DCM_Config,
#endif
#if defined(USE_DEM)
   .DemConfig = &DEM_Config,
#endif
#if defined(USE_XCP)
   .XcpConfig = &XcpConfig,
#endif
#if defined(USE_PDUR) || defined(USE_COM) || defined(USE_CANIF) || defined(USE_CANTP) || defined(USE_CANNM)
   .PostBuildConfig = &Postbuild_Config 
#endif
};


