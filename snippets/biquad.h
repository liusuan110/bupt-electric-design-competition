#ifndef BIQUAD_H
#define BIQUAD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // 系数：y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    float b0, b1, b2;
    float a1, a2;
    // 状态
    float z1, z2;
} biquad_t;

typedef struct {
    biquad_t* stages; // 指向多个级联节
    size_t numStages; // 节数
} biquad_chain_t;

void biquad_init(biquad_t* s, float b0, float b1, float b2, float a1, float a2);
static inline float biquad_process(biquad_t* s, float x) {
    float y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

void biquad_chain_init(biquad_chain_t* c, biquad_t* stages, size_t num);
float biquad_chain_process(biquad_chain_t* c, float x);

// 常用滤波器设计（双一阶等效或双二阶参数化）：
// 基于采样率 fs、中心/截止频率 f0、品质因数 Q，生成二阶节系数。
void biquad_design_lowpass(float fs, float fc, float Q, biquad_t* out);
void biquad_design_highpass(float fs, float fc, float Q, biquad_t* out);
void biquad_design_bandpass(float fs, float f0, float Q, biquad_t* out);
void biquad_design_notch(float fs, float f0, float Q, biquad_t* out);

#ifdef __cplusplus
}
#endif

#endif // BIQUAD_H
