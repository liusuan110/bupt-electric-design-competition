#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Goertzel 检测器上下文 */
typedef struct {
    float coeff;      ///< 系数：2*cos(2*pi*f_target/fs)
    float q1, q2;     ///< 内部状态
    float scale;      ///< 输入缩放（用于定点/防溢出，可设 1.0f）
} goertzel_t;

/** 初始化 Goertzel 检测器
 *  @param g 上下文
 *  @param f_target 目标频点(Hz)
 *  @param fs 采样率(Hz)
 *  @param scale 输入缩放（例如 1.0f）
 */
void goertzel_init(goertzel_t* g, float f_target, float fs, float scale);

/** 处理单个样本（在线更新状态） */
static inline void goertzel_process(goertzel_t* g, float x)
{
    x *= g->scale;
    float q0 = g->coeff * g->q1 - g->q2 + x;
    g->q2 = g->q1;
    g->q1 = q0;
}

/** 计算当前窗口的目标频点能量（未归一化） */
float goertzel_power(goertzel_t* g);

/** 重置状态以开始下一窗口 */
static inline void goertzel_reset(goertzel_t* g) { g->q1 = g->q2 = 0.0f; }

#ifdef __cplusplus
}
#endif
