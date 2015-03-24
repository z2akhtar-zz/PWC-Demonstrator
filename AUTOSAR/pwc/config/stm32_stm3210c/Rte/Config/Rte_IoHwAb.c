/** === HEADER ====================================================================================
 */

#include <Rte.h>

#include <Os.h>
#if ((OS_AR_RELEASE_MAJOR_VERSION != RTE_AR_RELEASE_MAJOR_VERSION) || (OS_AR_RELEASE_MINOR_VERSION != RTE_AR_RELEASE_MINOR_VERSION))
#error Os version mismatch
#endif

#include <Com.h>
#if ((COM_AR_RELEASE_MAJOR_VERSION != RTE_AR_RELEASE_MAJOR_VERSION) || (COM_AR_RELEASE_MINOR_VERSION != RTE_AR_RELEASE_MINOR_VERSION))
#error Com version mismatch
#endif

#include <Rte_Hook.h>
#include <Rte_Internal.h>
#include <Rte_Calprms.h>

#include "Rte_IoHwAb.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define IoHwAb_START_SEC_VAR_INIT_UNSPECIFIED
#include <IoHwAb_MemMap.h>
const Rte_CDS_IoHwAb IoHwAb_ioHwAb = {
    ._dummy = 0
};
#define IoHwAb_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <IoHwAb_MemMap.h>

#define IoHwAb_START_SEC_VAR_INIT_UNSPECIFIED
#include <IoHwAb_MemMap.h>
const Rte_Instance Rte_Inst_IoHwAb = &IoHwAb_ioHwAb;

#define IoHwAb_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <IoHwAb_MemMap.h>

/** === Runnables =================================================================================
 */
#define IoHwAb_START_SEC_CODE
#include <IoHwAb_MemMap.h>

/** ------ ioHwAb -----------------------------------------------------------------------
 */
Std_ReturnType Rte_ioHwAb_DigitalWrite(/*IN*/IoHwAb_SignalType_ portDefArg1, /*IN*/DigitalLevel Level) {
    Std_ReturnType retVal = RTE_E_OK;

    /* PRE */

    /* MAIN */

    retVal = IoHwAb_Digital_Write(portDefArg1, Level);

    /* POST */

    return retVal;
}
Std_ReturnType Rte_ioHwAb_DigitalRead(/*IN*/IoHwAb_SignalType_ portDefArg1, /*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    Std_ReturnType retVal = RTE_E_OK;

    /* PRE */

    /* MAIN */

    retVal = IoHwAb_Digital_Read(portDefArg1, Level, Quality);

    /* POST */

    return retVal;
}
#define IoHwAb_STOP_SEC_CODE
#include <IoHwAb_MemMap.h>

