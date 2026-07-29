#ifndef CHARGER_CONFIG_H
#define CHARGER_CONFIG_H

#include <stdint.h>

/* Conservative commissioning limits; tune only after calibrated low-voltage tests. */
#define CHARGER_CONTROL_PERIOD_MS       1u
#define CHARGER_PRECHARGE_TIME_MS       1000u
#define CHARGER_RELAY_SETTLE_TIME_MS    300u
#define CHARGER_SOFTSTART_TIME_MS       2000u
#define CHARGER_BUS_MIN_MV              250000u
#define CHARGER_BUS_MAX_MV              430000u
#define CHARGER_OUTPUT_MAX_MV           85000u
#define CHARGER_CURRENT_MAX_MA           22000u
#define CHARGER_TEMPERATURE_MAX_DC       900

/* Open-loop command endpoints (0..1000 permille). */
#define CHARGER_CV_START_PERMILLE        900u
#define CHARGER_CV_RUN_PERMILLE          500u
#define CHARGER_CURRENT_LIMIT_PERMILLE   250u

#endif
