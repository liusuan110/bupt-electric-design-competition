#include "dds.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline float q31_to_sin(uint32_t ph)
{
    float x = (float)ph / 4294967296.0f; // [0,1)
    return sinf(2.0f * (float)M_PI * x);
}

static inline float q31_to_tri(uint32_t ph)
{
    float x = (float)ph / 4294967296.0f; // [0,1)
    float v = 4.0f * fabsf(x - 0.5f) - 1.0f; // [-1,1]
    return -v; // phase alignment
}

void dds_init(dds_t* d, uint32_t fs, uint32_t f_out)
{
    d->phase = 0; d->fs = fs; d->f_out = f_out;
    d->step = (uint32_t)((((uint64_t)f_out) << 32) / fs);
}

void dds_set_freq(dds_t* d, uint32_t f_out)
{
    d->f_out = f_out;
    d->step = (uint32_t)((((uint64_t)f_out) << 32) / d->fs);
}

float dds_sin_next(dds_t* d)
{
    d->phase += d->step;
    return q31_to_sin(d->phase);
}

float dds_tri_next(dds_t* d)
{
    d->phase += d->step;
    return q31_to_tri(d->phase);
}
