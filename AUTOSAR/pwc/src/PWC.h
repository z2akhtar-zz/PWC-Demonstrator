/*
 * File: PWC.h
 *
 * Code generated for Simulink model 'PWC'.
 *
 * Model version                  : 1.63
 * Simulink Coder version         : 8.7 (R2014b) 08-Sep-2014
 * C/C++ source code generated on : Tue Feb 17 04:15:48 2015
 *
 * Target selection: autosar.tlc
 * Embedded hardware selection: 32-bit Generic
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef RTW_HEADER_PWC_h_
#define RTW_HEADER_PWC_h_

#include "Dio.h"
#include "Dio_Cfg.h"
#include "IoHwAb_Cfg.h"
#include "IoHwAb_Types.h"
#include "IoHwAb_Digital.h"

// For Joystick
#include "stm32f10x.h"
#include "stm32_eval.h"
#include "stm3210c_eval_ioe.h"

#define USE_STM3210C_EVAL
#define IOE_INTERRUPT_MODE

typedef uint8 uint8_T;
typedef boolean boolean_T;

/* Block states (auto storage) for model 'power_window_controller' */
typedef struct {
  uint8_T is_active_c3_power_window_contr;/* '<Root>/WinController' */
  uint8_T is_c3_power_window_controller;/* '<Root>/WinController' */
  uint8_T is_winStates;                /* '<Root>/WinController' */
  uint8_T is_Stopped;                  /* '<Root>/WinController' */
  uint8_T is_movingUp;                 /* '<Root>/WinController' */
  uint8_T is_movingUpX;                /* '<Root>/WinController' */
  uint8_T is_movingDown;               /* '<Root>/WinController' */
  uint8_T temporalCounter_i1;          /* '<Root>/WinController' */
} DW_power_window_controller_f_T;

typedef struct {
  DW_power_window_controller_f_T rtdw;
} MdlrefDW_power_window_control_T;

/* Model reference registration function */
extern void power_window_control_initialize(void);
extern void power_window_controller(const uint8_T *rtu_req, const boolean_T
  *rtu_endStop, const boolean_T *rtu_obstacle, uint8_T *rty_cmd,
  DW_power_window_controller_f_T *localDW);

/* Macros for accessing real-time model data structure */
#ifndef rtmGetStopRequested
# define rtmGetStopRequested(rtm)      ((void*) 0)
#endif

/* Block signals for system '<Root>/WinArbitrator' */
typedef struct {
  uint8_T y;                           /* '<Root>/WinArbitrator' */
} B_WinArbitrator_PWC_T;

/* Block signals (auto storage) */
typedef struct tag_B_PWC_T {
  uint8_T WinController;               /* '<Root>/WinController' */
  B_WinArbitrator_PWC_T sf_WinArbitrator;/* '<Root>/WinArbitrator' */
} B_PWC_T;

/* Block states (auto storage) for system '<Root>' */
typedef struct tag_DW_PWC_T {
  MdlrefDW_power_window_control_T WinController_DWORK1;/* '<Root>/WinController' */
} DW_PWC_T;

/* Block signals (auto storage) */
extern B_PWC_T PWC_B;

/* Block states (auto storage) */
extern DW_PWC_T PWC_DW;

/* Model step and init functions */
extern void Runnable_Step(void);
extern void Runnable_Init(void);

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'PWC'
 * '<S1>'   : 'PWC/WinArbitrator'
 */
#endif                                 /* RTW_HEADER_PWC_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
