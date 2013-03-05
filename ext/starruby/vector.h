#ifndef STARRUBY_VECTOR_H
#define STARRUBY_VECTOR_H

typedef struct {
  int32_t x, y, z;
} Point3I;

typedef struct {
  double x, y, z;
} Point3F;

typedef struct {
  int32_t x, y;
} Point2I;

typedef struct {
  double x, y;
} Point2F;

typedef Point2I Vector2I;
typedef Point2F Vector2F;
typedef Point3I Vector3I;
typedef Point3F Vector3F;

#endif
