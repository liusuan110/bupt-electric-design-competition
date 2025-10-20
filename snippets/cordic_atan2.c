#include "cordic_atan2.h"
#include <math.h>

static float cordic_atan_table(int i) {
    // 角度表：atan(2^-i)
    static const float table[24] = {
        0.7853981633974483f, 0.4636476090008061f, 0.24497866312686414f,
        0.12435499454676144f, 0.06241880999595735f, 0.031239833430268277f,
        0.015623728620476831f, 0.007812341060101111f, 0.0039062301319669718f,
        0.0019531225164788188f, 0.0009765621895593195f, 0.0004882812111948983f,
        0.00024414062014936177f, 0.00012207031189367021f, 6.103515617420877e-05f,
        3.0517578115526096e-05f, 1.5258789061315762e-05f, 7.62939453110197e-06f,
        3.814697265606496e-06f, 1.907348632810187e-06f, 9.536743164059608e-07f,
        4.7683715820308884e-07f, 2.3841857910155797e-07f, 1.1920928955078068e-07f
    };
    return (i < 24) ? table[i] : atanf(ldexpf(1.0f, -i));
}

float cordic_atan2f(float y, float x, int iterations) {
    // 处理象限
    float angle = 0.0f;
    float tx = x, ty = y;
    if (tx < 0.0f) {
        tx = -tx; ty = -ty; angle = (float)M_PI; // 旋转 180°
    }
    // 旋转模式
    float z = 0.0f;
    float pow2 = 1.0f; // 2^-i
    for (int i = 0; i < iterations; ++i) {
        float dx = (ty >= 0.0f) ? pow2 * ty : -pow2 * ty;
        float dy = (ty >= 0.0f) ? -pow2 * tx : pow2 * tx;
        float zi = cordic_atan_table(i);
        float tx_new = tx + dx;
        float ty_new = ty + dy;
        z += (ty >= 0.0f) ? zi : -zi;
        tx = tx_new; ty = ty_new;
        pow2 *= 0.5f;
    }
    return angle + z;
}
