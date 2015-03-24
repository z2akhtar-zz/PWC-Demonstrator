/**
 * Application Header File
 *
 * @req SWS_Rte_01003
 */

/** === HEADER ====================================================================================
 */

/** --- C++ guard ---------------------------------------------------------------------------------
 * @req SWS_Rte_03709
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** --- Normal include guard ----------------------------------------------------------------------
 */
#ifndef RTE_WINCONTROLLERTYPE_H_
#define RTE_WINCONTROLLERTYPE_H_

/** --- Duplicate application include guard -------------------------------------------------------
 * @req SWS_Rte_01006
 */
#ifdef RTE_APPLICATION_HEADER_FILE
#error Multiple application header files included.
#endif
#define RTE_APPLICATION_HEADER_FILE

/** --- Single runnable API -----------------------------------------------------------------------
 * @req SWS_Rte_02751
 */
#if defined(RTE_RUNNABLEAPI_WinControllerStepRunnable)
#define RTE_RUNNABLEAPI
#endif

/** --- Includes ----------------------------------------------------------------------------------
 * @req SWS_Rte_02751
 * @req SWS_Rte_07131
 */
#include <Rte_DataHandleType.h>
#include <Rte_WinControllerType_Type.h>

/** --- Application Errors ------------------------------------------------------------------------
 * @req SWS_Rte_02575
 * @req SWS_Rte_02576
 * @req SWS_Rte_07143
 */

/** --- Initial Values ----------------------------------------------------------------------------
 * @SWS_Rte_05078
 */

/** --- PIM DATA TYPES ------------------------------------------------------------------------------ */

/** === BODY ======================================================================================
 */

/** @req SWS_Rte_03731
 *  @req SWS_Rte_07137
 *  @req SWS_Rte_07138
 *  !req SWS_Rte_06523
 *  @req SWS_Rte_03730
 *  @req SWS_Rte_07677
 *  @req SWS_Rte_02620
 *  @req SWS_Rte_02621
 *  @req SWS_Rte_01055
 *  @req SWS_Rte_03726 */

/** @req SWS_Rte_01343
 *  @req SWS_Rte_01342
 *  !req SWS_Rte_06524
 *  @req SWS_Rte_01053
 */

/** @req SWS_Rte_07132
 *  @req SWS_Rte_03714 
 *  @req SWS_Rte_03725 
 *	@req SWS_Rte_03752
 *	@req SWS_Rte_02623
 */
typedef struct {
    Rte_DE_myBoolean * const WinControllerStepRunnable_endStop_isPresent;
    Rte_DE_myBoolean * const WinControllerStepRunnable_obstacle_isPresent;
    Rte_DE_requestType * const WinControllerStepRunnable_req_request;
    Rte_DE_commandType * const WinControllerStepRunnable_cmd_command;
} Rte_CDS_WinControllerType;

/** --- Instance handle type ---------------------------------------------------------------------- */
typedef Rte_CDS_WinControllerType const * const Rte_Instance;

/** --- Singleton instance handle -----------------------------------------------------------------
 *  @req SWS_Rte_03793
 */
extern const Rte_Instance Rte_Inst_WinControllerType;
#define self (Rte_Inst_WinControllerType)

/** --- Calibration API --------------------------------------------------------------------------- */

/** --- Per Instance Memory API ------------------------------------------------------------------- */

/** --- Indirect port API ------------------------------------------------------------------------- */

/** --- Single Runnable APIs ---------------------------------------------------------------------- */
#if defined(RTE_RUNNABLEAPI)
/** --- WinControllerStepRunnable */
#if defined(RTE_RUNNABLEAPI_WinControllerStepRunnable)

void WinControllerRunnable(void);

static inline requestType Rte_IRead_WinControllerStepRunnable_req_request(void) {
    return self->WinControllerStepRunnable_req_request->value;
}

static inline myBoolean Rte_IRead_WinControllerStepRunnable_obstacle_isPresent(void) {
    return self->WinControllerStepRunnable_obstacle_isPresent->value;
}

static inline myBoolean Rte_IRead_WinControllerStepRunnable_endStop_isPresent(void) {
    return self->WinControllerStepRunnable_endStop_isPresent->value;
}

static inline void Rte_IWrite_WinControllerStepRunnable_cmd_command(/*IN*/commandType value) {
    self->WinControllerStepRunnable_cmd_command->value = value;
}

#endif
#endif

/** --- All Runnable APIs ------------------------------------------------------------------------- */
#if !defined(RTE_RUNNABLEAPI)
void WinControllerRunnable(void);

static inline requestType Rte_IRead_WinControllerStepRunnable_req_request(void) {
    return self->WinControllerStepRunnable_req_request->value;
}

static inline myBoolean Rte_IRead_WinControllerStepRunnable_obstacle_isPresent(void) {
    return self->WinControllerStepRunnable_obstacle_isPresent->value;
}

static inline myBoolean Rte_IRead_WinControllerStepRunnable_endStop_isPresent(void) {
    return self->WinControllerStepRunnable_endStop_isPresent->value;
}

static inline void Rte_IWrite_WinControllerStepRunnable_cmd_command(/*IN*/commandType value) {
    self->WinControllerStepRunnable_cmd_command->value = value;
}
#endif

/** === FOOTER ====================================================================================
 */

#endif /* RTE_WINCONTROLLERTYPE_H_ */

/** @req SWS_Rte_03710 */
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
