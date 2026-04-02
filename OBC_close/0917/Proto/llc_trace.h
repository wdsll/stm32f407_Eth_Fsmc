#ifndef LLC_TRACE_H
#define LLC_TRACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
*                                              类型定义
*********************************************************************************************************/
typedef struct {
    uint32_t t_ms;
    uint8_t  state;

    float    vout_v;
    float    iout_a;
    float    vbus_v;

    float    vref_v;
    float    vmeas_v;

    float    f_cmd_hz;
    uint32_t f_act_hz;
} llc_trace_sample_t;

typedef enum {
    LLC_TRACE_CR_MON = 0,
    LLC_TRACE_CR_EVT_BEGIN,
    LLC_TRACE_CR_EVT_END,
} llc_trace_cr_type_t;

typedef struct {
    llc_trace_cr_type_t type;

    uint32_t t_ms;      /* MON */
    uint32_t id;        /* BEGIN / END */

    float vout_v;
    float iout_a;
    float err_v;
    float f_cmd_hz;

    float iout_from_a;
    float iout_to_a;
    float vout_pre_v;

    uint8_t  pass;
    uint32_t dt_ms;
    uint32_t settle_ms;
    float    vmin_v;
    float    vmax_v;
    float    maxerr_v;
} llc_trace_cr_item_t;

typedef enum {
    LLC_TRACE_TX_NONE = 0,
    LLC_TRACE_TX_COLLAPSE_SNAP,
    LLC_TRACE_TX_COLLAPSE_DIAG,
} llc_trace_tx_kind_t;

typedef struct {
    llc_trace_tx_kind_t kind;

    uint8_t  phase;        /* 0=pre,1=trig,2=post，仅 SNAP 有效 */

    uint32_t t_ms;
    uint8_t  state;
    float    vout_v;
    float    f_cmd_hz;
    uint32_t f_act_hz;

    uint8_t  verdict;      /* DIAG */
    uint8_t  reason_state; /* DIAG */
    float    dv_v;         /* DIAG */
    float    df_cmd_hz;    /* DIAG */
    float    df_act_hz;    /* DIAG */
} llc_trace_tx_item_t;

/*********************************************************************************************************
*                                              生命周期
*********************************************************************************************************/
void llc_trace_init(void);
void llc_trace_reset(void);

/*********************************************************************************************************
*                                              采集入口
*********************************************************************************************************/
void llc_trace_fast_tick(const llc_trace_sample_t *s);
void llc_trace_slow_tick(const llc_trace_sample_t *s);
void llc_trace_on_state_change(uint8_t prev, uint8_t next, uint32_t t_ms);

/*********************************************************************************************************
*                                              CR记录队列接口
*********************************************************************************************************/
bool     llc_trace_cr_pending(void);
uint16_t llc_trace_cr_pending_count(void);
bool     llc_trace_cr_peek(llc_trace_cr_item_t *out);
bool     llc_trace_cr_pop(llc_trace_cr_item_t *out);
uint32_t llc_trace_cr_overflow_count(void);

/*********************************************************************************************************
*                                              Collapse待发送接口
*********************************************************************************************************/
bool llc_trace_tx_pending(void);
bool llc_trace_tx_peek(llc_trace_tx_item_t *out);
void llc_trace_tx_consume(void);

#ifdef __cplusplus
}
#endif

#endif /* LLC_TRACE_H */