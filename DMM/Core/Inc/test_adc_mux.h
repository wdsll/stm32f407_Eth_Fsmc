/*
 * test_adc_mux.h
 *
 *  Created on: Sep 26, 2025
 *      Author: huyan
 */

#ifndef INC_TEST_ADC_MUX_H_
#define INC_TEST_ADC_MUX_H_

#include <stdio.h>

#define MU__QUOTE_INNER(x) #x
#define MU__QUOTE(x) MU__QUOTE_INNER(x)
#define MU__LOCATION __FILE__ ":" MU__QUOTE(__LINE__)

#define mu_assert(message, test) \
do { \
    if (!(test)) return message; \
} while (0)
#define mu_assert_float(message, expected, actual, epsilon) \
    do { \
        float mu__diff = (float)((expected) - (actual)); \
        if (mu__diff < 0) mu__diff = -mu__diff; \
        if (mu__diff > (epsilon)) { \
            static char mu__msg[128]; \
            snprintf(mu__msg, sizeof(mu__msg), "%s expected %.6f got %.6f", message, (double)(expected), (double)(actual)); \
            return mu__msg; \
        } \
    } while (0)

#define mu_run_test(test) do { \
    const char *message = test(); \
    tests_run++; \
    if (message) return message; \
} while (0)

#define mu_check(condition) mu_assert(MU__LOCATION " check failed", (condition))

extern int tests_run;

#endif /* INC_TEST_ADC_MUX_H_ */
