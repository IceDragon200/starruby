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

  #define STRUCT_CHECK_TYPE_FUNC(namespace, strct)                                    \
    void strb_Check ## namespace(VALUE rbObj)                                         \
    {                                                                                 \
      Check_Type(rbObj, T_DATA);                                                      \
      if (RDATA(rbObj)->dfree != (RUBY_DATA_FUNC)namespace ## _free) {                \
        rb_raise(rb_eTypeError, "wrong argument type %s, expected StarRuby::" #strct, \
                 rb_obj_classname(rbObj));                                            \
      }                                                                               \
    }

#endif
