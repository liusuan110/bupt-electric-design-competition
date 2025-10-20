#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WINDOW_RECT,   // 矩形窗
    WINDOW_HANN,   // 汉宁窗
    WINDOW_HAMMING,// 海明窗
    WINDOW_BLACKMAN// 布莱克曼窗
} window_type_t;

// 生成长度为 n 的窗口系数到 w（w 长度需 >= n）
void window_fill(float* w, size_t n, window_type_t type);

// 将窗口系数应用到 data（原地乘以窗口）
void window_apply(float* data, const float* w, size_t n);

// 便捷函数：无需单独缓冲，按类型直接对 data 乘窗
void window_apply_inplace(float* data, size_t n, window_type_t type);

#ifdef __cplusplus
}
#endif

#endif // WINDOW_H
