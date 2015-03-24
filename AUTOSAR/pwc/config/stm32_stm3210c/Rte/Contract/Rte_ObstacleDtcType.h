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
#ifndef RTE_OBSTACLEDTCTYPE_H_
#define RTE_OBSTACLEDTCTYPE_H_

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
#if defined(RTE_RUNNABLEAPI_ObstacleDtcRunnable)
#define RTE_RUNNABLEAPI
#endif

/** --- Includes ----------------------------------------------------------------------------------
 * @req SWS_Rte_02751
 * @req SWS_Rte_07131
 */
#include <Rte_DataHandleType.h>
#include <Rte_ObstacleDtcType_Type.h>

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

extern Std_ReturnType Rte_Call_ObstacleDtcType_ObstacleDtc_isObstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality);

/** @req SWS_Rte_07132
 *  @req SWS_Rte_03714 
 *  @req SWS_Rte_03725 
 *	@req SWS_Rte_03752
 *	@req SWS_Rte_02623
 */
typedef struct {
    Rte_DE_myBoolean * const ObstacleDtcRunnable_obstacle_isPresent;
} Rte_CDS_ObstacleDtcType;

/** --- Instance handle type ---------------------------------------------------------------------- */
typedef Rte_CDS_ObstacleDtcType const * const Rte_Instance;

/** --- Singleton instance handle -----------------------------------------------------------------
 *  @req SWS_Rte_03793
 */
extern const Rte_Instance Rte_Inst_ObstacleDtcType;
#define self (Rte_Inst_ObstacleDtcType)

/** --- Calibration API --------------------------------------------------------------------------- */

/** --- Per Instance Memory API ------------------------------------------------------------------- */

/** --- Indirect port API ------------------------------------------------------------------------- */

/** --- Single Runnable APIs ---------------------------------------------------------------------- */
#if defined(RTE_RUNNABLEAPI)
/** --- ObstacleDtcRunnable */
#if defined(RTE_RUNNABLEAPI_ObstacleDtcRunnable)

void ObstacleDtcRunnable(void);

static inline void Rte_IWrite_ObstacleDtcRunnable_obstacle_isPresent(/*IN*/myBoolean value) {
    self->ObstacleDtcRunnable_obstacle_isPresent->value = value;
}

static inline Std_ReturnType Rte_Call_isObstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    return Rte_Call_ObstacleDtcType_ObstacleDtc_isObstacle_Read(Level, Quality);
}

#endif
#endif

/** --- All Runnable APIs ------------------------------------------------------------------------- */
#if !defined(RTE_RUNNABLEAPI)
void ObstacleDtcRunnable(void);

static inline void Rte_IWrite_ObstacleDtcRunnable_obstacle_isPresent(/*IN*/myBoolean value) {
    self->ObstacleDtcRunnable_obstacle_isPresent->value = value;
}

static inline Std_ReturnType Rte_Call_isObstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    return Rte_Call_ObstacleDtcType_ObstacleDtc_isObstacle_Read(Level, Quality);
}
#endif

/** === FOOTER ====================================================================================
 */

#endif /* RTE_OBSTACLEDTCTYPE_H_ */

/** @req SWS_Rte_03710 */
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
