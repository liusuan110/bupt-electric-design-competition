//
// Simple real-valued FIR filter (Direct Form)
// - Circular state buffer, length N
// - Coefficients are ordered from k=0..N-1, multiplied with samples x[n], x[n-1], ...
// Complexity per sample: O(N)
// Edge cases: N>0; state/coeffs must be valid; no internal saturation handling
//
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * FIR filter context.
 * - coeffs: pointer to N taps (b[0..N-1])
 * - state: circular buffer of length N, holding last N input samples
 * - N: number of taps
 * - idx: write index in circular buffer
 */
typedef struct {
    const float* coeffs;  // taps[N]
    float* state;         // circular buffer, size N
    size_t N;             // number of taps
    size_t idx;           // write index
} firf_t;

/**
 * Initialize FIR filter.
 * @param f       context (allocated by caller)
 * @param coeffs  pointer to N coefficients (b[0]..b[N-1])
 * @param stateBuf pointer to N-length state buffer (cleared by caller if needed)
 * @param N       number of taps (>0)
 */
void firf_init(firf_t* f, const float* coeffs, float* stateBuf, size_t N);

/**
 * Process one sample.
 * Accumulates y[n] = sum_{k=0..N-1} b[k] * x[n-k].
 * @param f FIR context
 * @param x input sample
 * @return filtered output
 */
static inline float firf_process(firf_t* f, float x)
{
    f->state[f->idx] = x;
    float acc = 0.0f;
    size_t i = f->idx;
    for (size_t k = 0; k < f->N; ++k) {
        acc += f->coeffs[k] * f->state[i];
        i = (i == 0) ? (f->N - 1) : (i - 1);
    }
    f->idx = (f->idx + 1) % f->N;
    return acc;
}

#ifdef __cplusplus
}
#endif
