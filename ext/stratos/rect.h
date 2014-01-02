#ifndef STRATOS_RECT_H
#define STRATOS_RECT_H

#include <stdint.h>
#include "stratos.h"

struct _stratos_rect
{
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

typedef struct _stratos_rect stratos_rect_t;

stratos_func void stratos_rect_init(stratos_rect_t* rect);
stratos_func int32_t stratos_rect_get_x(stratos_rect_t* rect);
stratos_func int32_t stratos_rect_get_y(stratos_rect_t* rect);
stratos_func int32_t stratos_rect_get_width(stratos_rect_t* rect);
stratos_func int32_t stratos_rect_get_height(stratos_rect_t* rect);
stratos_func void stratos_rect_set_x(stratos_rect_t* rect, int32_t value);
stratos_func void stratos_rect_set_y(stratos_rect_t* rect, int32_t value);
stratos_func void stratos_rect_set_width(stratos_rect_t* rect, int32_t value);
stratos_func void stratos_rect_set_height(stratos_rect_t* rect, int32_t value);

#endif
