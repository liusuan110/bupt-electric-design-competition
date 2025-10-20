#ifndef NOTCH_H
#define NOTCH_H

#include "biquad.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Simple notch filter wrapper using a single biquad section. */
typedef struct {
    biquad_t sec; // single 2nd-order section
} notch_t;

/** Initialize notch at center frequency f0 with quality factor Q. */
static inline void notch_init(notch_t* n, float fs, float f0, float Q) {
    if (!n) return;
    biquad_design_notch(fs, f0, Q, &n->sec);
}

/** Process one sample through notch filter. */
static inline float notch_process(notch_t* n, float x) {
    if (!n) return x;
    return biquad_process(&n->sec, x);
}

#ifdef __cplusplus
}
#endif

#endif // NOTCH_H
