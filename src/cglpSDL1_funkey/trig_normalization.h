#ifndef TRIG_NORMALIZATION_H
#define TRIG_NORMALIZATION_H

#include <math.h>

// Macro to normalize the angle between 0 and 2π
#define NORMALIZE_ANGLE(angle) (angle = fmodf(angle, 2 * M_PI), (angle < 0) ? (angle += 2 * M_PI) : angle)

// Wrapper for sinf
static inline float normalized_sinf(float angle) {
    NORMALIZE_ANGLE(angle);
    return sinf(angle);
}

// Wrapper for cosf
static inline float normalized_cosf(float angle) {
    NORMALIZE_ANGLE(angle);
    return cosf(angle);
}

// Wrapper for tanf
static inline float normalized_tanf(float angle) {
    NORMALIZE_ANGLE(angle);
    return tanf(angle);
}

// Wrapper for asinf
static inline float normalized_asinf(float angle) {
    if (angle < -1.0f) angle = -1.0f;
    if (angle > 1.0f) angle = 1.0f;
    return sinf(angle);
}

// Wrapper for acosf
static inline float normalized_acosf(float angle) {
    if (angle < -1.0f) angle = -1.0f;
    if (angle > 1.0f) angle = 1.0f;
    return cosf(angle);
}

// Wrapper for atanf
static inline float normalized_atanf(float angle) {
    return atanf(angle);
}

// Wrapper for atan2f
static inline float normalized_atan2f(float y, float x) {
    return atan2f(y, x);
}

#endif // TRIG_NORMALIZATION_H