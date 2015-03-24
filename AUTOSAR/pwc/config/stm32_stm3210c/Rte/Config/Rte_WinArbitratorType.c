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

#include "Rte_WinArbitratorType.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */
#define WinArbitratorType_START_SEC_VAR_CLEARED_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>

struct {
    struct {
        struct {
            Rte_DE_requestType request;
        } req_a;
        struct {
            Rte_DE_requestType request;
        } req_d;
        struct {
            Rte_DE_requestType request;
        } req_p;

    } WinArbitratorRunnable;
} ImplDE_WinArbitrator;
#define WinArbitratorType_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define WinArbitratorType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>
const Rte_CDS_WinArbitratorType WinArbitratorType_WinArbitrator = {
    .WinArbitratorRunnable_req_d_request = &ImplDE_WinArbitrator.WinArbitratorRunnable.req_d.request,
    .WinArbitratorRunnable_req_p_request = &ImplDE_WinArbitrator.WinArbitratorRunnable.req_p.request,
    .WinArbitratorRunnable_req_a_request = &ImplDE_WinArbitrator.WinArbitratorRunnable.req_a.request
};
#define WinArbitratorType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>

#define WinArbitratorType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>
const Rte_Instance Rte_Inst_WinArbitratorType = &WinArbitratorType_WinArbitrator;

#define WinArbitratorType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>

/** === Runnables =================================================================================
 */
#define WinArbitratorType_START_SEC_CODE
#include <WinArbitratorType_MemMap.h>

/** ------ WinArbitrator -----------------------------------------------------------------------
 */
void Rte_WinArbitrator_WinArbitratorRunnable(void) {

    /* PRE */
    Rte_Read_WinArbitratorType_WinArbitrator_req_d_request(&ImplDE_WinArbitrator.WinArbitratorRunnable.req_d.request.value);

    Rte_Read_WinArbitratorType_WinArbitrator_req_p_request(&ImplDE_WinArbitrator.WinArbitratorRunnable.req_p.request.value);

    /* MAIN */

    WinArbitratorRunnable();

    /* POST */
    Rte_Write_WinArbitratorType_WinArbitrator_req_a_request(ImplDE_WinArbitrator.WinArbitratorRunnable.req_a.request.value);

}
#define WinArbitratorType_STOP_SEC_CODE
#include <WinArbitratorType_MemMap.h>

