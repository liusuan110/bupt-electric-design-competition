#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Optional CMSIS-DSP wrapper for real FFT.
 * If CMSIS is unavailable, a naive DFT fallback is used (small N only).
 */
typedef struct {
    uint16_t nfft;  // FFT size (power of two)
    float fs;       // sampling frequency (Hz)
    float* buf;     // time-domain input buffer of length nfft
    float* mag;     // output magnitude spectrum (length nfft/2)
} rfft_ctx_t;

/** Initialize FFT context. Returns 0 on success. */
int rfft_init(rfft_ctx_t* c, uint16_t nfft, float fs, float* buf, float* mag);

/**
 * Compute magnitude spectrum of current buf into mag.
 * @param hann non-zero to apply Hann window on buf before FFT
 * @return 0 on success; negative on error
 */
int rfft_compute_mag(rfft_ctx_t* c, int hann);

#ifdef __cplusplus
}
#endif
