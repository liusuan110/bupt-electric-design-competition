#ifndef MOVING_AVG_H
#define MOVING_AVG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Sliding window moving average (boxcar) */
typedef struct {
    float sum;      // running sum of elements
    size_t win;     // window length
    size_t idx;     // current write index
    size_t filled;  // number of valid elements (<=win)
    float* buf;     // external buffer of length 'win'
} movavg_t;

/** Initialize moving average with caller-provided buffer. */
void movavg_init(movavg_t* m, float* buf, size_t win);

/** Update with one sample and return current average. */
float movavg_update(movavg_t* m, float x);

/** Exponential moving average (IIR smoothing) */
typedef struct {
    float alpha; // 0..1, larger -> faster response
    float y;     // current value
    int initialized;
} ema_t;

/** Initialize EMA with smoothing factor alpha. */
void ema_init(ema_t* e, float alpha);

/** Update EMA with one sample and return current value. */
float ema_update(ema_t* e, float x);

#ifdef __cplusplus
}
#endif

#endif // MOVING_AVG_H
