#include "rms_agc.h"
#include <math.h>

/** Compute root-mean-square of x[0..n-1]. */
float rms_compute(const float* x, size_t n) {
    if (!x || n == 0) return 0.0f;
    double s = 0.0; // 累加用双精度降低误差
    for (size_t i = 0; i < n; ++i) s += (double)x[i] * (double)x[i];
    return (float)sqrt(s / (double)n);
}

/** Initialize AGC parameters. */
void agc_init(agc_t* a, float target, float alpha, float init_gain) {
    if (!a) return;
    a->target = target;
    a->alpha = alpha;
    a->gain = init_gain;
}

/**
 * Update gain based on instantaneous magnitude (EMA-like) and apply to sample.
 * Note: For better stability, consider windowed RMS measurement before gain update.
 */
float agc_process(agc_t* a, float x) {
    if (!a) return x;
    // 简化：按瞬时幅度估计近似 RMS，实际可引入窗口 RMS 或 EMA
    float y = a->gain * x;
    float mag = fabsf(y) + 1e-6f;
    float err = a->target / mag;
    a->gain = a->alpha * err * a->gain + (1.0f - a->alpha) * a->gain;
    return a->gain * x;
}
