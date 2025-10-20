#ifndef RMS_AGC_H
#define RMS_AGC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Compute RMS of an array (float). */
float rms_compute(const float* x, size_t n);

/** Simple automatic gain control (AGC) structure. */
typedef struct {
    float target; // target RMS level
    float alpha;  // smoothing for gain updates
    float gain;   // current gain
} agc_t;

/** Initialize AGC. */
void agc_init(agc_t* a, float target, float alpha, float init_gain);

/** Process one sample through AGC; returns gain-adjusted sample. */
float agc_process(agc_t* a, float x);

#ifdef __cplusplus
}
#endif

#endif // RMS_AGC_H
