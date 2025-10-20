#include "biquad.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void biquad_init(biquad_t* s, float b0, float b1, float b2, float a1, float a2) {
    if (!s) return;
    s->b0 = b0; s->b1 = b1; s->b2 = b2;
    s->a1 = a1; s->a2 = a2;
    s->z1 = 0.0f; s->z2 = 0.0f;
}

void biquad_chain_init(biquad_chain_t* c, biquad_t* stages, size_t num) {
    if (!c) return;
    c->stages = stages;
    c->numStages = num;
}

float biquad_chain_process(biquad_chain_t* c, float x) {
    if (!c || !c->stages) return x;
    float y = x;
    for (size_t i = 0; i < c->numStages; ++i) {
        y = biquad_process(&c->stages[i], y);
    }
    return y;
}

// RBJ Audio EQ Cookbook 公式
static void normalize(float* b0, float* b1, float* b2, float* a0, float* a1, float* a2) {
    float ia0 = 1.0f / (*a0);
    *b0 *= ia0; *b1 *= ia0; *b2 *= ia0;
    *a1 *= ia0; *a2 *= ia0; *a0 = 1.0f;
}

void biquad_design_lowpass(float fs, float fc, float Q, biquad_t* out) {
    float w0 = 2.0f * (float)M_PI * fc / fs;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s / (2.0f * Q);
    float b0 = (1.0f - c) * 0.5f;
    float b1 = 1.0f - c;
    float b2 = (1.0f - c) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;
    normalize(&b0, &b1, &b2, &a0, &a1, &a2);
    biquad_init(out, b0, b1, b2, -a1, -a2);
}

void biquad_design_highpass(float fs, float fc, float Q, biquad_t* out) {
    float w0 = 2.0f * (float)M_PI * fc / fs;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s / (2.0f * Q);
    float b0 = (1.0f + c) * 0.5f;
    float b1 = -(1.0f + c);
    float b2 = (1.0f + c) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;
    normalize(&b0, &b1, &b2, &a0, &a1, &a2);
    biquad_init(out, b0, b1, b2, -a1, -a2);
}

void biquad_design_bandpass(float fs, float f0, float Q, biquad_t* out) {
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s / (2.0f * Q);
    float b0 = Q * alpha;
    float b1 = 0.0f;
    float b2 = -Q * alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;
    normalize(&b0, &b1, &b2, &a0, &a1, &a2);
    biquad_init(out, b0, b1, b2, -a1, -a2);
}

void biquad_design_notch(float fs, float f0, float Q, biquad_t* out) {
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s / (2.0f * Q);
    float b0 = 1.0f;
    float b1 = -2.0f * c;
    float b2 = 1.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * c;
    float a2 = 1.0f - alpha;
    normalize(&b0, &b1, &b2, &a0, &a1, &a2);
    biquad_init(out, b0, b1, b2, -a1, -a2);
}
