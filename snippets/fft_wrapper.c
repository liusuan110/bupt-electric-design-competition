#include "fft_wrapper.h"
#include <math.h>

// If CMSIS-DSP is available, include headers and implement using arm_rfft_fast_f32
#ifdef USE_CMSIS_DSP
#include "arm_math.h"
#include "arm_const_structs.h"

typedef struct {
    rfft_ctx_t base;
    arm_rfft_fast_instance_f32 inst;
} _rfft_impl_t;

static _rfft_impl_t _impl;

int rfft_init(rfft_ctx_t* c, uint16_t nfft, float fs, float* buf, float* mag)
{
    _impl.base.nfft = nfft;
    _impl.base.fs = fs;
    _impl.base.buf = buf;
    _impl.base.mag = mag;
    if (arm_rfft_fast_init_f32(&_impl.inst, nfft) != ARM_MATH_SUCCESS) {
        return -1;
    }
    *c = _impl.base;
    return 0;
}

static void apply_hann(float* x, uint16_t N)
{
    for (uint16_t n = 0; n < N; ++n) {
        float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265358979f * n / (N - 1)));
        x[n] *= w;
    }
}

int rfft_compute_mag(rfft_ctx_t* c, int hann)
{
    if (!_impl.base.buf || !_impl.base.mag) return -1;
    if (hann) apply_hann(_impl.base.buf, _impl.base.nfft);
    // CMSIS RFFT outputs N complex bins packed in N floats (N real, N imag interleaved by API)
    // Use arm_rfft_fast_f32 then compute magnitude via arm_cmplx_mag_f32 on half spectrum
    static float tmp[4096]; // adjust or allocate statically as needed
    if (_impl.base.nfft > 4096) return -2; // guard
    arm_rfft_fast_f32(&_impl.inst, _impl.base.buf, tmp, 0);
    // tmp contains N complex spectrum in real/imag pairs? For fast_rfft: length N floats (N/2 complex)
    // Use CMSIS helper to compute magnitude
    arm_cmplx_mag_f32(tmp, _impl.base.mag, _impl.base.nfft / 2);
    return 0;
}

#else
// Fallback: naive DFT magnitude (O(N^2)) for small N, for quick validation only.
static void apply_hann(float* x, uint16_t N)
{
    for (uint16_t n = 0; n < N; ++n) {
        float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265358979f * n / (N - 1)));
        x[n] *= w;
    }
}

int rfft_init(rfft_ctx_t* c, uint16_t nfft, float fs, float* buf, float* mag)
{
    c->nfft = nfft; c->fs = fs; c->buf = buf; c->mag = mag; return 0;
}

int rfft_compute_mag(rfft_ctx_t* c, int hann)
{
    if (!c || !c->buf || !c->mag) return -1;
    const uint16_t N = c->nfft;
    if (hann) apply_hann(c->buf, N);
    for (uint16_t k = 0; k < N/2; ++k) {
        float re = 0.0f, im = 0.0f;
        for (uint16_t n = 0; n < N; ++n) {
            float ang = -2.0f * 3.14159265358979f * k * n / N;
            float cs = cosf(ang), sn = sinf(ang);
            re += c->buf[n] * cs;
            im += c->buf[n] * sn;
        }
        c->mag[k] = sqrtf(re*re + im*im);
    }
    return 0;
}
#endif
