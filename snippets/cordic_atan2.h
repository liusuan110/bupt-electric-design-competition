#ifndef CORDIC_ATAN2_H
#define CORDIC_ATAN2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CORDIC-based atan2 approximation in radians.
 * @param y          ordinate
 * @param x          abscissa
 * @param iterations number of CORDIC iterations (typ. 12..16 for embedded)
 * @return angle in radians in (-pi, pi]
 */
float cordic_atan2f(float y, float x, int iterations);

#ifdef __cplusplus
}
#endif

#endif // CORDIC_ATAN2_H
