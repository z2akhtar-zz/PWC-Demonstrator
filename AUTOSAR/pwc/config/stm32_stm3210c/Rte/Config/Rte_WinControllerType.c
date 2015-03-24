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

#include "Rte_WinControllerType.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */
#define WinControllerType_START_SEC_VAR_CLEARED_UNSPECIFIED
#include <WinControllerType_MemMap.h>

struct {
    struct {
        struct {
            Rte_DE_commandType command;
        } cmd;
        struct {
            Rte_DE_myBoolean isPresent;
        } endStop;
        struct {
            Rte_DE_myBoolean isPresent;
        } obstacle;
        struct {
            Rte_DE_requestType request;
        } req;

    } WinControllerStepRunnable;
} ImplDE_WinController;
#define WinControllerType_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include <WinControllerType_MemMap.h>

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define WinControllerType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>
const Rte_CDS_WinControllerType WinControllerType_WinController = {
    .WinControllerStepRunnable_req_request = &ImplDE_WinController.WinControllerStepRunnable.req.request,
    .WinControllerStepRunnable_obstacle_isPresent = &ImplDE_WinController.WinControllerStepRunnable.obstacle.isPresent,
    .WinControllerStepRunnable_endStop_isPresent = &ImplDE_WinController.WinControllerStepRunnable.endStop.isPresent,
    .WinControllerStepRunnable_cmd_command = &ImplDE_WinController.WinControllerStepRunnable.cmd.command
};
#define WinControllerType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>

#define WinControllerType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>
const Rte_Instance Rte_Inst_WinControllerType = &WinControllerType_WinController;

#define WinControllerType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>

/** === Runnables =================================================================================
 */
#define WinControllerType_START_SEC_CODE
#include <WinControllerType_MemMap.h>

/** ------ WinController -----------------------------------------------------------------------
 */
void Rte_WinController_WinControllerStepRunnable(void) {

    /* PRE */
    Rte_Read_WinControllerType_WinController_req_request(&ImplDE_WinController.WinControllerStepRunnable.req.request.value);

    Rte_Read_WinControllerType_WinController_obstacle_isPresent(&ImplDE_WinController.WinControllerStepRunnable.obstacle.isPresent.value);

    Rte_Read_WinControllerType_WinController_endStop_isPresent(&ImplDE_WinController.WinControllerStepRunnable.endStop.isPresent.value);

    /* MAIN */

    WinControllerRunnable();

    /* POST */
    Rte_Write_WinControllerType_WinController_cmd_command(ImplDE_WinController.WinControllerStepRunnable.cmd.command.value);

}
#define WinControllerType_STOP_SEC_CODE
#include <WinControllerType_MemMap.h>

