#ifndef BIQUAD_H
#define BIQUAD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Direct Form I (transposed) biquad:
 * y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
 * State variables (z1,z2) store intermediate values.
 */
typedef struct {
    float b0, b1, b2; // feedforward
    float a1, a2;     // feedback (note: a0 normalized to 1)
    float z1, z2;     // state
} biquad_t;

/** Cascaded biquad chain */
typedef struct {
    biquad_t* stages; // array of biquads
    size_t numStages; // number of stages
} biquad_chain_t;

/** Initialize biquad with coefficients (a0 assumed normalized to 1). */
void biquad_init(biquad_t* s, float b0, float b1, float b2, float a1, float a2);

/** Process one sample through a biquad. */
static inline float biquad_process(biquad_t* s, float x) {
    float y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

/** Initialize a cascaded chain of biquads. */
void biquad_chain_init(biquad_chain_t* c, biquad_t* stages, size_t num);

/** Process one sample through the cascaded chain. */
float biquad_chain_process(biquad_chain_t* c, float x);

// RBJ Audio EQ cookbook-based designs
void biquad_design_lowpass(float fs, float fc, float Q, biquad_t* out);
void biquad_design_highpass(float fs, float fc, float Q, biquad_t* out);
void biquad_design_bandpass(float fs, float f0, float Q, biquad_t* out);
void biquad_design_notch(float fs, float f0, float Q, biquad_t* out);

#ifdef __cplusplus
}
#endif

#endif // BIQUAD_H
