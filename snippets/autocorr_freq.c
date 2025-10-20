#include "autocorr_freq.h"

static float dot(const float* a, const float* b, size_t n) {
    float s = 0.0f;
    for (size_t i = 0; i < n; ++i) s += a[i] * b[i];
    return s;
}

size_t autocorr_estimate_period(const float* x, size_t n, size_t minLag, size_t maxLag) {
    if (!x || n < 4 || minLag >= maxLag) return 0;
    if (maxLag >= n) maxLag = n - 1;
    float best = -1e30f;
    size_t bestLag = 0;
    // 归一化能量
    float e0 = dot(x, x, n);
    if (e0 <= 1e-12f) return 0;
    for (size_t lag = minLag; lag <= maxLag; ++lag) {
        size_t m = n - lag;
        float r = dot(x, x + lag, m);
        float rn = r / (e0 + 1e-12f);
        if (rn > best) { best = rn; bestLag = lag; }
    }
    // 简单阈值，避免噪声误检
    if (bestLag == 0 || best < 0.05f) return 0;
    return bestLag;
}
