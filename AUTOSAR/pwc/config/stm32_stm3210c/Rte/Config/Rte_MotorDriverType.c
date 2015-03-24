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

#include "Rte_MotorDriverType.h"

/** === Inter-Runnable Variable Buffers ===========================================================
 */

/** === Inter-Runnable Variable Functions =========================================================
 */

/** === Implicit Buffer Instances =================================================================
 */
#define MotorDriverType_START_SEC_VAR_CLEARED_UNSPECIFIED
#include <MotorDriverType_MemMap.h>

struct {
    struct {
        struct {
            Rte_DE_commandType command;
        } cmd;

    } MotorDriverRunnable;
} ImplDE_MotorDriver;
#define MotorDriverType_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include <MotorDriverType_MemMap.h>

/** === Per Instance Memories =====================================================================
 */

/** === Component Data Structure Instances ========================================================
 */
#define MotorDriverType_START_SEC_VAR_INIT_UNSPECIFIED
#include <MotorDriverType_MemMap.h>
const Rte_CDS_MotorDriverType MotorDriverType_MotorDriver = {
    .MotorDriverRunnable_cmd_command = &ImplDE_MotorDriver.MotorDriverRunnable.cmd.command
};
#define MotorDriverType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <MotorDriverType_MemMap.h>

#define MotorDriverType_START_SEC_VAR_INIT_UNSPECIFIED
#include <MotorDriverType_MemMap.h>
const Rte_Instance Rte_Inst_MotorDriverType = &MotorDriverType_MotorDriver;

#define MotorDriverType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <MotorDriverType_MemMap.h>

/** === Runnables =================================================================================
 */
#define MotorDriverType_START_SEC_CODE
#include <MotorDriverType_MemMap.h>

/** ------ MotorDriver -----------------------------------------------------------------------
 */
void Rte_MotorDriver_MotorDriverRunnable(void) {

    /* PRE */
    Rte_Read_MotorDriverType_MotorDriver_cmd_command(&ImplDE_MotorDriver.MotorDriverRunnable.cmd.command.value);

    /* MAIN */

    MotorDriverRunnable();

    /* POST */

}
#define MotorDriverType_STOP_SEC_CODE
#include <MotorDriverType_MemMap.h>

