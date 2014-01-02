/*
  Slim RGX Rect Struct
 */
#ifndef STARRUBY_RECT_H
#define STARRUBY_RECT_H

#include <stdint.h>
#include <stdbool.h>
#include <ruby.h>

// Struct
typedef struct strb_rect
{
  int32_t x, y, width, height;
} Rect;

void strb_RubyToRect(VALUE rbObj, Rect* rect);

inline bool
strb_modify_rect_in_rect(const Rect* src_rect, Rect* dst_rect)
{
  if (dst_rect->x < src_rect->x) {
    dst_rect->width -= -dst_rect->x - src_rect->x;
    dst_rect->x = src_rect->x;
  }
  if (dst_rect->y < src_rect->y) {
    dst_rect->height -= -dst_rect->y - src_rect->y;
    dst_rect->y = src_rect->x;
  }
  const int32_t sx2 = (src_rect->x + src_rect->width);
  const int32_t sy2 = (src_rect->y + src_rect->height);
  /* Is the X, Y outside the texture bounds? */
  if (sx2 <= dst_rect->x || sy2 <= dst_rect->y) {
    return false;
  }
  const int32_t x2 = dst_rect->x + dst_rect->width;
  const int32_t y2 = dst_rect->y + dst_rect->height;
  if (x2 > sx2) {
    dst_rect->width -= x2 - sx2;
  }
  if (y2 > sy2) {
    dst_rect->height -= y2 - sy2;
  }

  if (dst_rect->width <= 0 || dst_rect->height <= 0) {
    return false;
  }

  return true;
}

#endif
