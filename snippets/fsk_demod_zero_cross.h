#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Zero-crossing based 2-FSK demodulator state */
typedef struct {
    float thr;           // zero-crossing threshold (e.g., 0)
    uint32_t lastSign;   // 0: neg or zero, 1: pos
    uint32_t sampleCount;// samples since last zero-cross
    float fs;            // sampling rate
    float f0, f1;        // two FSK frequencies
} fsk_zc_t;

/** Initialize demodulator with sampling rate and target tones. */
void fsk_zc_init(fsk_zc_t* d, float fs, float f0, float f1, float thr);

/**
 * Feed one sample.
 * @return -1 (no decision yet), 0 (detected f0), 1 (detected f1)
 */
int fsk_zc_process(fsk_zc_t* d, float x);

#ifdef __cplusplus
}
#endif
