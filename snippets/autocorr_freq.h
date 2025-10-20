#ifndef AUTOCORR_FREQ_H
#define AUTOCORR_FREQ_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Estimate fundamental period via normalized autocorrelation peak search.
 * @param x       input buffer
 * @param n       length of x
 * @param minLag  minimum lag to search (in samples)
 * @param maxLag  maximum lag to search (in samples)
 * @return estimated period in samples; 0 if not found
 */
size_t autocorr_estimate_period(const float* x, size_t n, size_t minLag, size_t maxLag);

#ifdef __cplusplus
}
#endif

#endif // AUTOCORR_FREQ_H
