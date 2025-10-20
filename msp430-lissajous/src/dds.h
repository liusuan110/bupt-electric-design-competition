#pragma once
#include <stdint.h>

typedef struct {
    uint32_t phase;      // 32-bit phase accumulator
    uint32_t step;       // phase step per sample
    uint32_t fs;         // sampling rate (Hz)
    uint32_t f_out;      // target output frequency (Hz)
} dds_t;

void dds_init(dds_t* d, uint32_t fs, uint32_t f_out);
void dds_set_freq(dds_t* d, uint32_t f_out);
// return sine in [-1,1]
float dds_sin_next(dds_t* d);
// return tri in [-1,1]
float dds_tri_next(dds_t* d);
