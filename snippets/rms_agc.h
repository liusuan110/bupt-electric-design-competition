#ifndef RMS_AGC_H
#define RMS_AGC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

float rms_compute(const float* x, size_t n);

typedef struct {
    float target; // 目标 RMS
    float alpha;  // 平滑因子
    float gain;   // 当前增益
} agc_t;

void agc_init(agc_t* a, float target, float alpha, float init_gain);
// 返回增益后的样本
float agc_process(agc_t* a, float x);

#ifdef __cplusplus
}
#endif

#endif // RMS_AGC_H
