#ifndef CORDIC_ATAN2_H
#define CORDIC_ATAN2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 迭代次数越多精度越高，嵌入式建议 12~16 次
float cordic_atan2f(float y, float x, int iterations);

#ifdef __cplusplus
}
#endif

#endif // CORDIC_ATAN2_H
