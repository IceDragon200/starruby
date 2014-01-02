/*
  StarRuby accessor
  */
#ifndef STARRUBY_ACCESSOR_H
  #define STARRUBY_ACCESSOR_H

  #define STRUCT_ATTR_ACCESSOR(namespace, strct, attr, to_ruby, to_c) \
    static VALUE namespace ## _get_ ## attr(VALUE self)               \
    {                                                                 \
      strct *obj1;                                                    \
      Data_Get_Struct(self, strct, obj1);                             \
      return to_ruby(obj1->attr);                                     \
    }                                                                 \
    static VALUE namespace ## _set_ ## attr(VALUE self, VALUE arg1)   \
    {                                                                 \
      rb_check_frozen(self);                                          \
      strct *obj1;                                                    \
      Data_Get_Struct(self, strct, obj1);                             \
      obj1->attr = to_c(arg1);                                        \
      return Qnil;                                                    \
    }

  #define STRUCT_FREE(namespace, strct)          \
    static void namespace ## _free(strct *arg1)  \
    {                                            \
      free(arg1);                                \
    }

#endif
