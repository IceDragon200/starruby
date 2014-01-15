#ifndef STARRUBY_TEXTURE_PRV_H
#define STARRUBY_TEXTURE_PRV_H

#include "texture.h"

#define is_valid_tone(tone_p) (tone_p && (tone_p->saturation < 255 || tone_p->red != 0 || tone_p->green != 0 || tone_p->blue != 0))
#define is_valid_color(color_p) (color_p && (color_p->alpha > 0))
#define is_x_in_rect(rect, _x_) (((rect).x <= (_x_)) && (((rect).x + (rect).width) > (_x_)))
#define is_y_in_rect(rect, _y_) (((rect).y <= (_y_)) && (((rect).y + (rect).height) > (_y_)))
#define is_xy_in_rect(rect, _x_, _y_) (is_x_in_rect(rect, _x_) && is_y_in_rect(rect, _y_))
// Use Pointers for the Pixels

#define LOOP(process, length) \
  do {                        \
    int n = (length + 7) / 8; \
    switch (length % 8) {     \
    case 0: do { process;     \
      case 7: process;        \
      case 6: process;        \
      case 5: process;        \
      case 4: process;        \
      case 3: process;        \
      case 2: process;        \
      case 1: process;        \
      } while (--n > 0);      \
    }                         \
  } while (false)

#define RENDER_PIXEL(_dst, _src)                              \
  do {                                                        \
    if (_dst.alpha == 0) {                                    \
      _dst = _src;                                            \
    } else {                                                  \
      if (_dst.alpha < _src.alpha) {                          \
        _dst.alpha = _src.alpha;                              \
      }                                                       \
      _dst.red   = ALPHA(_src.red,   _dst.red,   _src.alpha); \
      _dst.green = ALPHA(_src.green, _dst.green, _src.alpha); \
      _dst.blue  = ALPHA(_src.blue,  _dst.blue,  _src.alpha); \
    }                                                         \
  } while (false)

#endif
