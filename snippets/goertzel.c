#include "goertzel.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void goertzel_init(goertzel_t* g, float f_target, float fs, float scale)
{
    float omega = 2.0f * (float)M_PI * (f_target / fs);
    g->coeff = 2.0f * cosf(omega);
    g->q1 = 0.0f;
    g->q2 = 0.0f;
    g->scale = scale;
}

float goertzel_power(goertzel_t* g)
{
    // Power ~ q1^2 + q2^2 - coeff*q1*q2
    float p = g->q1 * g->q1 + g->q2 * g->q2 - g->coeff * g->q1 * g->q2;
    return p;
}
