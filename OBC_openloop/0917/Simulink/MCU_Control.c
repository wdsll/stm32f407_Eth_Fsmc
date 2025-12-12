
/*
 * File: MCU_Control.c
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

#include "MCU_Control.h"
#include "MCU_Control_private.h"

/* Block states (default storage) */
DW_MCU_Control_T MCU_Control_DW;

/* Real-time model */
RT_MODEL_MCU_Control_T MCU_Control_M_;
RT_MODEL_MCU_Control_T *const MCU_Control_M = &MCU_Control_M_;

/* Model step function */
void MCU_Control_step(void)
{
  real_T rtb_Uk1;
  real_T rtb_Step_target_duty1;

  /* UnitDelay: '<S2>/Delay Input1'
   *
   * Block description for '<S2>/Delay Input1':
   *
   *  Store in Global RAM
   */
  rtb_Uk1 = MCU_Control_DW.DelayInput1_DSTATE;

  /* Step: '<S1>/Step_enable1' incorporates:
   *  Step: '<S1>/Step_target_duty1'
   *  UnitDelay: '<S2>/Delay Input1'
   *
   * Block description for '<S2>/Delay Input1':
   *
   *  Store in Global RAM
   */
  rtb_Step_target_duty1 = ((MCU_Control_M->Timing.clockTick0) * 0.0001);
  MCU_Control_DW.DelayInput1_DSTATE = !(rtb_Step_target_duty1 < 0.0005);

  /* Step: '<S1>/Step_target_duty1' */
  if (rtb_Step_target_duty1 < 0.0005) {
    rtb_Step_target_duty1 = 0.0;
  } else {
    rtb_Step_target_duty1 = 0.5;
  }

  /* MATLAB Function: '<S1>/llc_vref_softstart_step1' incorporates:
   *  RelationalOperator: '<S2>/FixPt Relational Operator'
   *  UnitDelay: '<S2>/Delay Input1'
   *
   * Block description for '<S2>/Delay Input1':
   *
   *  Store in Global RAM
   */
  if (!MCU_Control_DW.initialized_not_empty) {
    MCU_Control_DW.initialized_not_empty = true;
    MCU_Control_DW.start_duty = 0.1;
    if (MCU_Control_DW.start_duty < 0.12000000000000001) {
      MCU_Control_DW.start_duty = 0.12000000000000001;
    } else {
      if (MCU_Control_DW.start_duty > 0.88) {
        MCU_Control_DW.start_duty = 0.88;
      }
    }

    MCU_Control_DW.target_duty_cur = MCU_Control_DW.start_duty;
    MCU_Control_DW.last_duty = MCU_Control_DW.start_duty;
  }

  if (MCU_Control_DW.DelayInput1_DSTATE > rtb_Uk1) {
    MCU_Control_DW.duration_ms = 300.0;
    MCU_Control_DW.start_duty = 0.1;
    if (MCU_Control_DW.start_duty < 0.12000000000000001) {
      MCU_Control_DW.start_duty = 0.12000000000000001;
    } else {
      if (MCU_Control_DW.start_duty > 0.88) {
        MCU_Control_DW.start_duty = 0.88;
      }
    }

    if (rtb_Step_target_duty1 < 0.12000000000000001) {
      rtb_Step_target_duty1 = 0.12000000000000001;
    }

    MCU_Control_DW.target_duty_cur = rtb_Step_target_duty1;
    if ((rtb_Step_target_duty1 <= MCU_Control_DW.start_duty) ||
        (MCU_Control_DW.duration_ms <= 0.0)) {
      MCU_Control_DW.active_flag = false;
      MCU_Control_DW.elapsed_ms = 0.0;
      MCU_Control_DW.last_duty = rtb_Step_target_duty1;
    } else {
      MCU_Control_DW.active_flag = true;
      MCU_Control_DW.elapsed_ms = 0.0;
      MCU_Control_DW.last_duty = MCU_Control_DW.start_duty;
    }
  } else {
    if (MCU_Control_DW.active_flag) {
      MCU_Control_DW.elapsed_ms += 0.1;
      if (MCU_Control_DW.elapsed_ms >= MCU_Control_DW.duration_ms) {
        MCU_Control_DW.active_flag = false;
        MCU_Control_DW.elapsed_ms = MCU_Control_DW.duration_ms;
        if (MCU_Control_DW.target_duty_cur < 0.0) {
          MCU_Control_DW.last_duty = 0.0;
        } else if (MCU_Control_DW.target_duty_cur > 0.99) {
          MCU_Control_DW.last_duty = 0.99;
        } else {
          MCU_Control_DW.last_duty = MCU_Control_DW.target_duty_cur;
        }

        if (MCU_Control_DW.last_duty < 0.12000000000000001) {
          MCU_Control_DW.last_duty = 0.12000000000000001;
        } else {
          if (MCU_Control_DW.last_duty > 0.88) {
            MCU_Control_DW.last_duty = 0.88;
          }
        }
      } else {
        if (MCU_Control_DW.duration_ms > 0.0) {
          rtb_Uk1 = MCU_Control_DW.elapsed_ms / MCU_Control_DW.duration_ms;
        } else {
          rtb_Uk1 = 1.0;
        }

        if (rtb_Uk1 < 0.0) {
          rtb_Uk1 = 0.0;
        } else {
          if (rtb_Uk1 > 1.0) {
            rtb_Uk1 = 1.0;
          }
        }

        if (rtb_Uk1 <= 0.0) {
          rtb_Uk1 = 0.0;
        } else if (rtb_Uk1 >= 1.0) {
          rtb_Uk1 = 1.0;
        } else {
          rtb_Uk1 = (1.0 - cos(3.1415926535897931 * rtb_Uk1)) * 0.5;
        }

        MCU_Control_DW.last_duty = (MCU_Control_DW.target_duty_cur -
          MCU_Control_DW.start_duty) * rtb_Uk1 + MCU_Control_DW.start_duty;
        if (MCU_Control_DW.last_duty < 0.12000000000000001) {
          MCU_Control_DW.last_duty = 0.12000000000000001;
        } else {
          if (MCU_Control_DW.last_duty > 0.88) {
            MCU_Control_DW.last_duty = 0.88;
          }
        }
      }
    }
  }

  /* End of MATLAB Function: '<S1>/llc_vref_softstart_step1' */

  /* Update absolute time for base rate */
  /* The "clockTick0" counts the number of times the code of this task has
   * been executed. The resolution of this integer timer is 0.0001, which is the step size
   * of the task. Size of "clockTick0" ensures timer will not overflow during the
   * application lifespan selected.
   */
  MCU_Control_M->Timing.clockTick0++;
}

/* Model initialize function */
void MCU_Control_initialize(void)
{
  /* SystemInitialize for MATLAB Function: '<S1>/llc_vref_softstart_step1' */
  MCU_Control_DW.duration_ms = 300.0;
}

/* Model terminate function */
void MCU_Control_terminate(void)
{
  /* (no terminate code required) */
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */

