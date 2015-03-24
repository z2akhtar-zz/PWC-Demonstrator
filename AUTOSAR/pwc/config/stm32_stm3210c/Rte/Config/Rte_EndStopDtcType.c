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

#include "Rte_EndStopDtcType.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */
#define EndStopDtcType_START_SEC_VAR_CLEARED_UNSPECIFIED
#include <EndStopDtcType_MemMap.h>

struct {
    struct {
        struct {
            Rte_DE_myBoolean isPresent;
        } endStop;

    } EndStopDtcRunnable;
} ImplDE_EndStopDtc;
#define EndStopDtcType_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include <EndStopDtcType_MemMap.h>

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define EndStopDtcType_START_SEC_VAR_INIT_UNSPECIFIED
#include <EndStopDtcType_MemMap.h>
const Rte_CDS_EndStopDtcType EndStopDtcType_EndStopDtc = {
    .EndStopDtcRunnable_endStop_isPresent = &ImplDE_EndStopDtc.EndStopDtcRunnable.endStop.isPresent
};
#define EndStopDtcType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <EndStopDtcType_MemMap.h>

#define EndStopDtcType_START_SEC_VAR_INIT_UNSPECIFIED
#include <EndStopDtcType_MemMap.h>
const Rte_Instance Rte_Inst_EndStopDtcType = &EndStopDtcType_EndStopDtc;

#define EndStopDtcType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <EndStopDtcType_MemMap.h>

/** === Runnables =================================================================================
 */
#define EndStopDtcType_START_SEC_CODE
#include <EndStopDtcType_MemMap.h>

/** ------ EndStopDtc -----------------------------------------------------------------------
 */
void Rte_EndStopDtc_EndStopDtcRunnable(void) {

    /* PRE */

    /* MAIN */

    EndStopDtcRunnable();

    /* POST */
    Rte_Write_EndStopDtcType_EndStopDtc_endStop_isPresent(ImplDE_EndStopDtc.EndStopDtcRunnable.endStop.isPresent.value);

}
#define EndStopDtcType_STOP_SEC_CODE
#include <EndStopDtcType_MemMap.h>

