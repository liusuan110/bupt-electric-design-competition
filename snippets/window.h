//
// Window functions utilities
// - Provide common window shapes: Rect, Hann, Hamming, Blackman
// - API to generate window coefficients and apply to data
// Notes: For large N, prefer preallocating window buffer to avoid stack overuse.
//
#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Window types */
typedef enum {
    WINDOW_RECT,    // Rectangular
    WINDOW_HANN,    // Hann
    WINDOW_HAMMING, // Hamming
    WINDOW_BLACKMAN // Blackman
} window_type_t;

/**
 * Generate window coefficients of length n into w.
 * @param w    output buffer (length >= n)
 * @param n    number of points
 * @param type window type
 */
void window_fill(float* w, size_t n, window_type_t type);

/**
 * Apply precomputed window w to data in-place.
 * @param data data buffer length n, will be scaled element-wise by w
 * @param w    window coefficients length n
 * @param n    number of points
 */
void window_apply(float* data, const float* w, size_t n);

/**
 * Convenience: generate and apply window in-place without a separate buffer.
 * For large n, prefer window_fill + window_apply to avoid stack usage.
 */
void window_apply_inplace(float* data, size_t n, window_type_t type);

#ifdef __cplusplus
}
#endif

#endif // WINDOW_H
