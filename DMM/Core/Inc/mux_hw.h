/*
 * mux_hw.h
 *
 *  Created on: Sep 26, 2025
 *      Author: huyan
 */

#ifndef INC_MUX_HW_H_
#define INC_MUX_HW_H_

#include <stdint.h>
#if UNIT_TEST
void MUX_FUN_Select_test(uint8_t ch);
void MUX_I_Select_test(uint8_t ch);
void MUX_R_Select_test(uint8_t ch);
#define MUX_FUN_Select(ch) MUX_FUN_Select_test(ch)
#define MUX_I_Select(ch)   MUX_I_Select_test(ch)
#define MUX_R_Select(ch)   MUX_R_Select_test(ch)
#else
void MUX_FUN_Select(uint8_t ch);
void MUX_I_Select(uint8_t ch);
void MUX_R_Select(uint8_t ch);
#endif  

#endif /* INC_MUX_HW_H_ */
