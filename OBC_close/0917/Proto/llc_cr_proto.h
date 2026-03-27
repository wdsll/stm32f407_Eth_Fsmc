#ifndef LLC_CR_PROTO_H
#define LLC_CR_PROTO_H

#include <stdint.h>


typedef enum
{
    CR_LOG_MON = 0,
    CR_LOG_EVT_BEGIN,
    CR_LOG_EVT_END
} cr_log_type_t;

typedef struct
{
    cr_log_type_t type;
    uint32_t t_ms;
    uint32_t id;
    uint8_t  pass;
    uint32_t dt_ms;
    uint32_t settle_ms;
    float    vout_v;
    float    iout_a;
    float    err_v;
    float    f_cmd_hz;
    float    iout_from_a;
    float    iout_to_a;
    float    vout_pre_v;
    float    vmin_v;
    float    vmax_v;
    float    maxerr_v;
} llc_cr_log_item_t;

void llc_cr_proto_log_emit_bin(const llc_cr_log_item_t *item);
void llc_cr_proto_collapse_emit_bin(uint8_t tag, uint32_t t_ms, uint8_t state, float vout_v, float f_cmd_hz, uint32_t f_act_hz);
void llc_cr_proto_collapse_diag_emit_bin(uint8_t verdict, uint8_t reason_state, float dv_v, float df_cmd_hz, float df_act_hz);
#endif /* LLC_CR_PROTO_H */