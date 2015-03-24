/**
 * Generated RTE
 *
 * @req SWS_Rte_01169
 */

/** === HEADER ====================================================================================
 */

/** @req SWS_Rte_01279 */
#include <Rte.h>

/** @req SWS_Rte_01257 */
#include <Os.h>

#if ((OS_AR_RELEASE_MAJOR_VERSION != RTE_AR_RELEASE_MAJOR_VERSION) || (OS_AR_RELEASE_MINOR_VERSION != RTE_AR_RELEASE_MINOR_VERSION))
#error Os version mismatch
#endif

/** @req SWS_Rte_03794 */
#include <Com.h>

#if ((COM_AR_RELEASE_MAJOR_VERSION != RTE_AR_RELEASE_MAJOR_VERSION) || (COM_AR_RELEASE_MINOR_VERSION != RTE_AR_RELEASE_MINOR_VERSION))
#error Com version mismatch
#endif

/** @req SWS_Rte_01326 */
#include <Rte_Hook.h>

#include <Rte_Internal.h>

#include <Ioc.h>

/** === Os Macros =================================================================================
 */

#define END_OF_TASK(taskName) SYS_CALL_TerminateTask()

#define ARC_STRINGIFY(value)  ARC_STRINGIFY2(value)
#define ARC_STRINGIFY2(value) #value

#if defined(ARC_INJECTED_HEADER_RTE_C)
#define  THE_INCLUDE ARC_STRINGIFY(ARC_INJECTED_HEADER_RTE_C)
#include THE_INCLUDE
#undef   THE_INCLUDE
#endif

#if !defined(RTE_EXTENDED_TASK_LOOP_CONDITION)
#define RTE_EXTENDED_TASK_LOOP_CONDITION 1
#endif

/** === Generated API =============================================================================
 */

/** === Runnables =================================================================================
 */
extern void Rte_EndStopDtc_EndStopDtcRunnable(void);
extern void Rte_MotorDriver_MotorDriverRunnable(void);
extern void Rte_ObstacleDtc_ObstacleDtcRunnable(void);
extern void Rte_Switch_SwitchRunnable(void);
extern void Rte_WinArbitrator_WinArbitratorRunnable(void);
extern void Rte_WinController_WinControllerStepRunnable(void);

/** === Tasks =====================================================================================
 */
void CriticalTask(void) { /** @req SWS_Rte_02251 */
    EventMaskType Event;
    do {
        SYS_CALL_WaitEvent (EVENT_MASK_CriticalEvent);
        SYS_CALL_GetEvent(TASK_ID_CriticalTask, &Event);

        if (Event & EVENT_MASK_CriticalEvent) {
            SYS_CALL_ClearEvent(EVENT_MASK_CriticalEvent);
            Rte_ObstacleDtc_ObstacleDtcRunnable();
        }
        if (Event & EVENT_MASK_CriticalEvent) {
            SYS_CALL_ClearEvent(EVENT_MASK_CriticalEvent);
            Rte_EndStopDtc_EndStopDtcRunnable();
        }
        if (Event & EVENT_MASK_CriticalEvent) {
            SYS_CALL_ClearEvent(EVENT_MASK_CriticalEvent);
            Rte_WinController_WinControllerStepRunnable();
        }

    } while (RTE_EXTENDED_TASK_LOOP_CONDITION);
}

void StepTask(void) { /** @req SWS_Rte_02251 */
    EventMaskType Event;
    do {
        SYS_CALL_WaitEvent (EVENT_MASK_StepEvent);
        SYS_CALL_GetEvent(TASK_ID_StepTask, &Event);

        if (Event & EVENT_MASK_StepEvent) {
            SYS_CALL_ClearEvent(EVENT_MASK_StepEvent);
            Rte_Switch_SwitchRunnable();
        }
        if (Event & EVENT_MASK_StepEvent) {
            SYS_CALL_ClearEvent(EVENT_MASK_StepEvent);
            Rte_WinArbitrator_WinArbitratorRunnable();
        }
        if (Event & EVENT_MASK_StepEvent) {
            SYS_CALL_ClearEvent(EVENT_MASK_StepEvent);
            Rte_MotorDriver_MotorDriverRunnable();
        }

    } while (RTE_EXTENDED_TASK_LOOP_CONDITION);
}

