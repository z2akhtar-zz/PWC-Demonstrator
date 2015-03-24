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

#include "Rte_ObstacleDtcType.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */
#define ObstacleDtcType_START_SEC_VAR_CLEARED_UNSPECIFIED
#include <ObstacleDtcType_MemMap.h>

struct {
    struct {
        struct {
            Rte_DE_myBoolean isPresent;
        } obstacle;

    } ObstacleDtcRunnable;
} ImplDE_ObstacleDtc;
#define ObstacleDtcType_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include <ObstacleDtcType_MemMap.h>

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define ObstacleDtcType_START_SEC_VAR_INIT_UNSPECIFIED
#include <ObstacleDtcType_MemMap.h>
const Rte_CDS_ObstacleDtcType ObstacleDtcType_ObstacleDtc = {
    .ObstacleDtcRunnable_obstacle_isPresent = &ImplDE_ObstacleDtc.ObstacleDtcRunnable.obstacle.isPresent
};
#define ObstacleDtcType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <ObstacleDtcType_MemMap.h>

#define ObstacleDtcType_START_SEC_VAR_INIT_UNSPECIFIED
#include <ObstacleDtcType_MemMap.h>
const Rte_Instance Rte_Inst_ObstacleDtcType = &ObstacleDtcType_ObstacleDtc;

#define ObstacleDtcType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <ObstacleDtcType_MemMap.h>

/** === Runnables =================================================================================
 */
#define ObstacleDtcType_START_SEC_CODE
#include <ObstacleDtcType_MemMap.h>

/** ------ ObstacleDtc -----------------------------------------------------------------------
 */
void Rte_ObstacleDtc_ObstacleDtcRunnable(void) {

    /* PRE */

    /* MAIN */

    ObstacleDtcRunnable();

    /* POST */
    Rte_Write_ObstacleDtcType_ObstacleDtc_obstacle_isPresent(ImplDE_ObstacleDtc.ObstacleDtcRunnable.obstacle.isPresent.value);

}
#define ObstacleDtcType_STOP_SEC_CODE
#include <ObstacleDtcType_MemMap.h>

