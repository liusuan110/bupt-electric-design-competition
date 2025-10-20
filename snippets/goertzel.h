#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float coeff;      // 2*cos(2*pi*f_target/fs)
    float q1, q2;     // state
    float scale;      // optional input scaling
} goertzel_t;

// Initialize Goertzel detector
// f_target: target frequency (Hz)
// fs: sampling frequency (Hz)
// scale: multiply input sample by scale before processing (e.g., 1.0f)
void goertzel_init(goertzel_t* g, float f_target, float fs, float scale);

// Process one sample
static inline void goertzel_process(goertzel_t* g, float x)
{
    x *= g->scale;
    float q0 = g->coeff * g->q1 - g->q2 + x;
    g->q2 = g->q1;
    g->q1 = q0;
}

// Compute power at target frequency after a block
float goertzel_power(goertzel_t* g);

// Reset state for next block
static inline void goertzel_reset(goertzel_t* g) { g->q1 = g->q2 = 0.0f; }

#ifdef __cplusplus
}
#endif
