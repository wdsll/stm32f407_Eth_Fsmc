#include "ad7190_filter.h"

static int32_t filter_buf[4][AD7190_FILTER_DEPTH];
static uint8_t filter_index[4];

int32_t AD7190_Filter(int channel, int32_t sample)
{
    if(channel < 0 || channel >= 4)
    {
        return sample;
    }

    filter_buf[channel][filter_index[channel]] = sample;
    filter_index[channel] = (filter_index[channel] + 1) % AD7190_FILTER_DEPTH;

    int64_t sum = 0;
    for(int i = 0; i < AD7190_FILTER_DEPTH; ++i)
    {
        sum += filter_buf[channel][i];
    }

    return (int32_t)(sum / AD7190_FILTER_DEPTH);
}
