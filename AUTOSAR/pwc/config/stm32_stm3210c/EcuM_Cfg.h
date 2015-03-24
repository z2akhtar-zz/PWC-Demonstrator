/*
 * Generator version: 1.0.0
 * AUTOSAR version:   4.0.3
 */

#ifndef ECUM_CFG_H_
#define ECUM_CFG_H_ 


#if !(((ECUM_SW_MAJOR_VERSION == 1) && (ECUM_SW_MINOR_VERSION == 0)) )
#error EcuM: Configuration file expected EcuMFixed module version to be 1.0.X*
#endif

#define ECUM_VERSION_INFO_API          STD_OFF
#define ECUM_DEV_ERROR_DETECT          STD_OFF

#define ECUM_MAIN_FUNCTION_PERIOD      (200)
#define ECUM_NVRAM_READALL_TIMEOUT     (10000)
#define ECUM_NVRAM_WRITEALL_TIMEOUT    (10000)
#define ECUM_NVRAM_MIN_RUN_DURATION    (10000)

#define ECUM_RESET_LOOP_DETECTION      STD_OFF

#define ECUM_VALIDATION_TIMEOUT 0

	 
typedef enum {
   ECUM_USER_User_1,
   ECUM_USER_ENDMARK // Must be the last in list!
} EcuM_User;


// EcuM Sleep Mode IDs
#define ECUM_SLEEP_MODE_ECUMSLEEPMODE 0


#define ECUM_SLEEP_MODE_CNT 1 
#define ECUM_SLEEP_MODE_FIRST	0

#define ECUM_COMM_NETWORKS_CNT 0
#define ECUM_WKSOURCE_POWER 			(1uL)
#define ECUM_WKSOURCE_RESET 			(1uL<<1u)
#define ECUM_WKSOURCE_INTERNAL_RESET 	(1uL<<2u)
#define ECUM_WKSOURCE_INTERNAL_WDG 		(1uL<<3u)
#define	ECUM_WKSOURCE_EXTERNAL_WDG 		(1uL<<4u)
#define EcuMConf_EcuMWakeupSource_EcuMWakeupSource  (1uL << 5u)


#define ECUM_WKSOURCE_SYSTEM_CNT 5
#define ECUM_WKSOURCE_USER_CNT   1 

#endif /* ECUM_CFG_H_ */

