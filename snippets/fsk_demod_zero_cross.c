#include "fsk_demod_zero_cross.h"
#include <math.h>

void fsk_zc_init(fsk_zc_t* d, float fs, float f0, float f1, float thr)
{
    d->fs = fs; d->f0 = f0; d->f1 = f1; d->thr = thr;
    d->lastSign = 0; d->sampleCount = 0;
}

static inline uint32_t signp(float x, float thr) { return (x > thr) ? 1u : 0u; }

int fsk_zc_process(fsk_zc_t* d, float x)
{
    d->sampleCount++;
    uint32_t s = signp(x, d->thr);
    int out = -1;
    if (s != d->lastSign) {
        // zero crossing detected
        float period_samples = (float)d->sampleCount;
        d->sampleCount = 0;
        float f = d->fs / (2.0f * period_samples); // two zero-crossings per cycle
        // decide closer to f0 or f1
        float df0 = fabsf(f - d->f0);
        float df1 = fabsf(f - d->f1);
        out = (df0 < df1) ? 0 : 1;
    }
    d->lastSign = s;
    return out;
}
