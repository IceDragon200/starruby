#ifndef STARRUBY_TEXTURE_PRV_H
#define STARRUBY_TEXTURE_PRV_H

#include "texture.h"

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

static inline VALUE rb_GetErrno(char* name)
{
  return rb_const_get(rb_const_get(rb_cObject, rb_intern("Errno")),
                      rb_intern(name));
}

#endif
