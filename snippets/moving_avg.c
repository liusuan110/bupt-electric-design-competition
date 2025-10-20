#include "moving_avg.h"

void movavg_init(movavg_t* m, float* buf, size_t win) {
    if (!m || !buf || win == 0) return;
    m->sum = 0.0f;
    m->win = win;
    m->idx = 0;
    m->filled = 0;
    m->buf = buf;
    for (size_t i = 0; i < win; ++i) buf[i] = 0.0f;
}

float movavg_update(movavg_t* m, float x) {
    if (!m || !m->buf || m->win == 0) return x;
    m->sum -= m->buf[m->idx];
    m->buf[m->idx] = x;
    m->sum += x;
    m->idx = (m->idx + 1) % m->win;
    if (m->filled < m->win) m->filled++;
    return m->sum / (float)m->filled;
}

void ema_init(ema_t* e, float alpha) {
    if (!e) return;
    e->alpha = alpha;
    e->y = 0.0f;
    e->initialized = 0;
}

float ema_update(ema_t* e, float x) {
    if (!e) return x;
    if (!e->initialized) {
        e->y = x; e->initialized = 1;
    } else {
        e->y = e->alpha * x + (1.0f - e->alpha) * e->y;
    }
    return e->y;
}
