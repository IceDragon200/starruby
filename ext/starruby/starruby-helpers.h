#ifndef STARRUBY_HELPER_H
  #define STARRUBY_HELPER_H

  #define MAX(x, y) (((x) >= (y)) ? (x) : (y))
  #define MIN(x, y) (((x) <= (y)) ? (x) : (y))
  #define MINMAX(x, y, z) MIN(MAX((x), (z)), (y))

  #define MINMAXU255(x) MINMAX(x, 255, 0)
  #define MINMAX255(x) MINMAX(x, 255, -255)
  #define DIV255(x) ((x) / 255)

  #ifndef NUMERIC_P
    #define NUMERIC_P(obj) (TYPE(obj) == T_FIXNUM ? true : (TYPE(obj) == T_FLOAT ? true : (TYPE(obj) == T_BIGNUM ? true : false)))
  #endif

#endif
