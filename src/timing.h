#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>


typedef struct {

    uint32_t tick_rate;
    uint32_t tick;
    float tick_time;
    double server_time;

    float accumulator;
    float max_accumulator;

    uint64_t perf_freq;
    uint64_t last_time;
    uint64_t fps_time_accum;
    uint32_t frame_count;

} Timing;


#endif // TIMING_H