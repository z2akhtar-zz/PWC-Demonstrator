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

#include "Rte_SwitchType.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */
#define SwitchType_START_SEC_VAR_CLEARED_UNSPECIFIED
#include <SwitchType_MemMap.h>

struct {
    struct {
        struct {
            Rte_DE_requestType request;
        } req;

    } SwitchRunnable;
} ImplDE_Switch;
#define SwitchType_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include <SwitchType_MemMap.h>

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define SwitchType_START_SEC_VAR_INIT_UNSPECIFIED
#include <SwitchType_MemMap.h>
const Rte_CDS_SwitchType SwitchType_Switch = {
    .SwitchRunnable_req_request = &ImplDE_Switch.SwitchRunnable.req.request
};
#define SwitchType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <SwitchType_MemMap.h>

#define SwitchType_START_SEC_VAR_INIT_UNSPECIFIED
#include <SwitchType_MemMap.h>
const Rte_Instance Rte_Inst_SwitchType = &SwitchType_Switch;

#define SwitchType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <SwitchType_MemMap.h>

/** === Runnables =================================================================================
 */
#define SwitchType_START_SEC_CODE
#include <SwitchType_MemMap.h>

/** ------ Switch -----------------------------------------------------------------------
 */
void Rte_Switch_SwitchRunnable(void) {

    /* PRE */

    /* MAIN */

    SwitchRunnable();

    /* POST */
    Rte_Write_SwitchType_Switch_req_request(ImplDE_Switch.SwitchRunnable.req.request.value);

}
#define SwitchType_STOP_SEC_CODE
#include <SwitchType_MemMap.h>

