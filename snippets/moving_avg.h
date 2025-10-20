#ifndef MOVING_AVG_H
#define MOVING_AVG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float sum;
    size_t win;
    size_t idx;
    size_t filled;
    float* buf; // 外部分配或静态数组
} movavg_t;

void movavg_init(movavg_t* m, float* buf, size_t win);
float movavg_update(movavg_t* m, float x);

typedef struct {
    float alpha; // 0..1, 越大越快
    float y;
    int initialized;
} ema_t;

void ema_init(ema_t* e, float alpha);
float ema_update(ema_t* e, float x);

#ifdef __cplusplus
}
#endif

#endif // MOVING_AVG_H
