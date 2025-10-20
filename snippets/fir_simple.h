#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const float* coeffs;  // taps[N]
    float* state;         // circular buffer, size N
    size_t N;             // number of taps
    size_t idx;           // write index
} firf_t;

// Initialize FIR filter
void firf_init(firf_t* f, const float* coeffs, float* stateBuf, size_t N);

// Process one sample
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
