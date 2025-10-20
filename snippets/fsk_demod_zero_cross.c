#include "fsk_demod_zero_cross.h"
#include <math.h>

/**
 * 初始化基于过零计数的 2-FSK 解调器。
 * @param d   解调上下文
 * @param fs  采样率（Hz）
 * @param f0  频率0（Hz）
 * @param f1  频率1（Hz）
 * @param thr 过零阈值（建议为信号中点或略大于 0，用于抗噪）
 */
void fsk_zc_init(fsk_zc_t* d, float fs, float f0, float f1, float thr)
{
    d->fs = fs; d->f0 = f0; d->f1 = f1; d->thr = thr;
    d->lastSign = 0; d->sampleCount = 0;
}

static inline uint32_t signp(float x, float thr) { return (x > thr) ? 1u : 0u; }

/**
 * 处理单个样本，检测是否发生过零并输出比特。
 * @param d 解调上下文
 * @param x 当前输入样本
 * @return -1 表示未决（尚未到达一个完整周期）；0/1 表示解调出的比特值。
 * 边界：当噪声较大时，thr 需要适当提高；对非正弦波形该方法仍可用但精度下降。
 */
int fsk_zc_process(fsk_zc_t* d, float x)
{
    d->sampleCount++;
    uint32_t s = signp(x, d->thr);
    int out = -1;
    if (s != d->lastSign) {
        // zero crossing detected
        float period_samples = (float)d->sampleCount;
        d->sampleCount = 0;
        float f = d->fs / (2.0f * period_samples); // two zero-crossings per cycle
        // decide closer to f0 or f1
        float df0 = fabsf(f - d->f0);
        float df1 = fabsf(f - d->f1);
        out = (df0 < df1) ? 0 : 1;
    }
    d->lastSign = s;
    return out;
}
