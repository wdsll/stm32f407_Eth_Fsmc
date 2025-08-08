#ifndef __AD7190_FILTER_H
#define __AD7190_FILTER_H

#include <stdint.h>

#define AD7190_FILTER_DEPTH 8

int32_t AD7190_Filter(int channel, int32_t sample);

#endif /* __AD7190_FILTER_H */
