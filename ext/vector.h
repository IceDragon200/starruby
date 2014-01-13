/*
  StarRuby Vector [HEADER]
  vr 1.0.0
  */
#ifndef STARRUBY_VECTOR_H
#define STARRUBY_VECTOR_H

typedef struct {
  double x, y;
} Vector2;

typedef struct {
  double x, y, z;
} Vector3;

void strb_RubyToVector2(VALUE rbObj, Vector2* vec2);
void strb_RubyToVector3(VALUE rbObj, Vector3* vec3);

#endif
