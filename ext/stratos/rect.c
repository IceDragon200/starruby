/*
 * -- Insert ASCII art here --
 * Stratos
 *
 *      Stratos Rect
 *
 *      By IceDragon (mistdragon100@gmail.com)
 *
 *      See readme.txt for copyright information.
 */
#include "rect.h"

stratos_func void
stratos_rect_init(stratos_rect_t* rect)
{
  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;
}

/* get */
stratos_func int32_t
stratos_rect_get_x(stratos_rect_t* rect)
{
  return rect->x;
}

stratos_func int32_t
stratos_rect_get_y(stratos_rect_t* rect)
{
  return rect->y;
}

stratos_func int32_t
stratos_rect_get_width(stratos_rect_t* rect)
{
  return rect->width;
}

stratos_func int32_t
stratos_rect_get_height(stratos_rect_t* rect)
{
  return rect->height;
}

/* set */
stratos_func void
stratos_rect_set_x(stratos_rect_t* rect, int32_t value)
{
  rect->x = value;
}

stratos_func void
stratos_rect_set_y(stratos_rect_t* rect, int32_t value)
{
  rect->y = value;
}

stratos_func void
stratos_rect_set_width(stratos_rect_t* rect, int32_t value)
{
  rect->width = value;
}

stratos_func void
stratos_rect_set_height(stratos_rect_t* rect, int32_t value)
{
  rect->height = value;
}
