/*
 */
#ifndef STARRUBY_ARRAY_PRV_H
  #define STARRUBY_ARRAY_PRV_H

  #include "strb_array.h"

  #define ARRAY_FROM_RUBY_FUNC(type, default_Val, conv) \
    type* strb_## type ##_from_ruby(VALUE rbArray) \
    { \
      VALUE rbSize; \
      rbSize = rb_funcall(rbArray, rb_intern("size"), 0); \
      const uint32_t size = NUM2LONG(rbSize); \
      type* array = strb_Alloc ## type(size, default_Val); \
      for(uint32_t i = 0; i < size; i++) \
        array->data[i] = conv(rb_ary_entry(rbArray, i)); \
      return array; \
    }

#endif
