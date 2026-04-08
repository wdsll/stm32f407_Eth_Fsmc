
/*
 * File: MCU_Control.h
 *
 * Code generated for Simulink model 'MCU_Control'.
 *
 * Model version                  : 1.6
 * Simulink Coder version         : 9.3 (R2020a) 18-Nov-2019
 * C/C++ source code generated on : Thu Dec 11 15:42:27 2025
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Intel->x86-64 (Windows64)
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef RTW_HEADER_MCU_Control_h_
#define RTW_HEADER_MCU_Control_h_
#include <math.h>
#ifndef MCU_Control_COMMON_INCLUDES_
# define MCU_Control_COMMON_INCLUDES_
#include "rtwtypes.h"
#endif                                 /* MCU_Control_COMMON_INCLUDES_ */

#include "MCU_Control_types.h"

/* Macros for accessing real-time model data structure */
#ifndef rtmGetErrorStatus
# define rtmGetErrorStatus(rtm)        ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
# define rtmSetErrorStatus(rtm, val)   ((rtm)->errorStatus = (val))
#endif

/* Block states (default storage) for system '<Root>' */
typedef struct {
  real_T DelayInput1_DSTATE;           /* '<S2>/Delay Input1' */
  real_T elapsed_ms;                   /* '<S1>/llc_vref_softstart_step1' */
  real_T duration_ms;                  /* '<S1>/llc_vref_softstart_step1' */
  real_T start_duty;                   /* '<S1>/llc_vref_softstart_step1' */
  real_T target_duty_cur;              /* '<S1>/llc_vref_softstart_step1' */
  real_T last_duty;                    /* '<S1>/llc_vref_softstart_step1' */
  boolean_T initialized_not_empty;     /* '<S1>/llc_vref_softstart_step1' */
  boolean_T active_flag;               /* '<S1>/llc_vref_softstart_step1' */
} DW_MCU_Control_T;

/* Real-time Model Data Structure */
struct tag_RTM_MCU_Control_T {
  const char_T * volatile errorStatus;

  /*
   * Timing:
   * The following substructure contains information regarding
   * the timing information for the model.
   */
  struct {
    uint32_T clockTick0;
  } Timing;
};

/* Block states (default storage) */
extern DW_MCU_Control_T MCU_Control_DW;

/* Model entry point functions */
extern void MCU_Control_initialize(void);
extern void MCU_Control_step(void);
extern void MCU_Control_terminate(void);

/* Real-time Model object */
extern RT_MODEL_MCU_Control_T *const MCU_Control_M;

/*-
 * These blocks were eliminated from the model due to optimizations:
 *
 * Block '<Root>/Scope2' : Unused code path elimination
 */

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
 * '<Root>' : 'MCU_Control'
 * '<S1>'   : 'MCU_Control/open_loop'
 * '<S2>'   : 'MCU_Control/open_loop/Detect Increase'
 * '<S3>'   : 'MCU_Control/open_loop/llc_vref_softstart_step1'
 */
#endif                                 /* RTW_HEADER_MCU_Control_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */

