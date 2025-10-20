#ifndef NOTCH_H
#define NOTCH_H

#include "biquad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    biquad_t sec; // 单二阶陷波
} notch_t;

// 在采样率 fs 下，对 f0 处构建陷波，Q 控制带宽
static inline void notch_init(notch_t* n, float fs, float f0, float Q) {
    if (!n) return;
    biquad_design_notch(fs, f0, Q, &n->sec);
}

static inline float notch_process(notch_t* n, float x) {
    if (!n) return x;
    return biquad_process(&n->sec, x);
}

#ifdef __cplusplus
}
#endif

#endif // NOTCH_H
