#ifndef STARRUBY_HELPER_H
  #define STARRUBY_HELPER_H

  #define MAX(x, y) (((x) >= (y)) ? (x) : (y))
  #define MIN(x, y) (((x) <= (y)) ? (x) : (y))
  #define MINMAX(x, y, z) MIN(MAX((x), (z)), (y))

  #define MINMAXU255(x) MINMAX(x, 255, 0)
  #define MINMAX255(x) MINMAX(x, 255, -255)
  #define CLAMPU255 MINMAXU255
  #define CLAMP255 MINMAX255
  #define DIV255(x) ((x) / 255)

  #define ALPHA(src, dst, a) DIV255((dst << 8) - dst + (src - dst) * a)

  #ifndef NUMERIC_P
    #define NUMERIC_P(obj) (TYPE(obj) == T_FIXNUM ? true : (TYPE(obj) == T_FLOAT ? true : (TYPE(obj) == T_BIGNUM ? true : false)))
  #endif

#endif
