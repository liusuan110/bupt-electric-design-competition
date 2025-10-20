#ifndef AUTOCORR_FREQ_H
#define AUTOCORR_FREQ_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 计算自相关并估计主周期索引（样本点）。
// x: 输入信号，n: 长度，minLag/maxLag: 搜索滞后范围（例如 fs/2kHz 到 fs/100Hz）
// 返回：估计的周期样本数，若失败返回 0。
size_t autocorr_estimate_period(const float* x, size_t n, size_t minLag, size_t maxLag);

#ifdef __cplusplus
}
#endif

#endif // AUTOCORR_FREQ_H
