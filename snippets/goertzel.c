#include "goertzel.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * 初始化 Goertzel 单频检测器。
 * @param g       Goertzel 上下文（外部分配）
 * @param f_target 目标频点（Hz）
 * @param fs      采样率（Hz）
 * @param scale   输入缩放系数（用于防溢出/定点兼容，可为 1.0f）
 * 注意：系数基于 f_target 与 fs 计算，若运行中改变其一，需要重新初始化。
 */
void goertzel_init(goertzel_t* g, float f_target, float fs, float scale)
{
    float omega = 2.0f * (float)M_PI * (f_target / fs);
    g->coeff = 2.0f * cosf(omega);
    g->q1 = 0.0f;
    g->q2 = 0.0f;
    g->scale = scale;
}

/**
 * 计算当前窗口内的目标频点能量（未归一化）。
 * @param g Goertzel 上下文
 * @return 能量值（与输入缩放、窗口长度相关；用于相对比较/门限判定）
 */
float goertzel_power(goertzel_t* g)
{
    // Power ~ q1^2 + q2^2 - coeff*q1*q2
    float p = g->q1 * g->q1 + g->q2 * g->q2 - g->coeff * g->q1 * g->q2;
    return p;
}
