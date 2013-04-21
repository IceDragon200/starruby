/*
  StarRuby Vector [HEADER]
  vr 1.0.0
  */
#ifndef STARRUBY_VECTOR_H
  #define STARRUBY_VECTOR_H

  #include "starruby.prv.h"

  typedef struct {
    int32_t x, y;
  } Vector2I;

  typedef struct {
    double x, y;
  } Vector2F;

  typedef struct {
    int32_t x, y, z;
  } Vector3I;

  typedef struct {
    double x, y, z;
  } Vector3F;

  typedef Vector2I Point2I;
  typedef Vector2F Point2F;
  typedef Vector3I Point3I;
  typedef Vector3F Point3F;

#endif
