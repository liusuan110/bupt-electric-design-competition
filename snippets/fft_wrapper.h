#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Optional CMSIS-DSP wrapper for real FFT.
// If not using CMSIS, you can replace with a lightweight FFT or call into KissFFT.

typedef struct {
    uint16_t nfft;     // FFT size (power of two)
    float fs;          // sampling frequency
    float* buf;        // length nfft (real input), in-place RFFT
    float* mag;        // magnitude spectrum length nfft/2
} rfft_ctx_t;

// Return 0 on success
int rfft_init(rfft_ctx_t* c, uint16_t nfft, float fs, float* buf, float* mag);

// Compute magnitude spectrum of current buf
// Apply optional Hann window if hann != 0
int rfft_compute_mag(rfft_ctx_t* c, int hann);

#ifdef __cplusplus
}
#endif
