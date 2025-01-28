#include "vector.h"
#include "trig_normalization.h"

Vector* vectorSet(Vector* vec, float x, float y) {
  vec->x = x;
  vec->y = y;
  return vec;
}

Vector* vectorAdd(Vector* vec, float x, float y) {
  vec->x += x;
  vec->y += y;
  return vec;
}

Vector* vectorMul(Vector* vec, float v) {
  vec->x *= v;
  vec->y *= v;
  return vec;
}

Vector* rotate(Vector* vec, float angle) {
  float tx = vec->x;
  vec->x = tx * normalized_cosf(angle) - vec->y * normalized_sinf(angle);
  vec->y = tx * normalized_sinf(angle) + vec->y * normalized_cosf(angle);
  return vec;
}

Vector* addWithAngle(Vector* vec, float angle, float length) {
  vec->x += normalized_cosf(angle) * length;
  vec->y += normalized_sinf(angle) * length;
  return vec;
}

float angleTo(Vector* vec, float x, float y) {
  return atan2f(y - vec->y, x - vec->x);
}

float distanceTo(Vector* vec, float x, float y) {
  float ox = x - vec->x;
  float oy = y - vec->y;
  return sqrtf(ox * ox + oy * oy);
}

float vectorAngle(Vector* vec) { return atan2f(vec->y, vec->x); }

float vectorLength(Vector* vec) {
  return sqrtf(vec->x * vec->x + vec->y * vec->y);
}
