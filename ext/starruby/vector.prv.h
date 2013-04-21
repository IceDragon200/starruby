#ifndef STARRUBY_VECTOR_PRV_H
#define STARRUBY_VECTOR_PRV_H

#define VECTOR_ALLOC(namespace, strct, process) \
  STRUCT_FREE(namespace, strct); \
  static VALUE namespace ## _alloc(VALUE klass) \
  { \
    strct *allc_strct = ALLOC(strct); \
    process; \
    return Data_Wrap_Struct(klass, 0, namespace ## _free, allc_strct); \
  }

#define VECTOR_ALLOC2(namespace, strct, argdefault) \
  VECTOR_ALLOC(namespace, strct, \
    { \
      allc_strct->x = argdefault; \
      allc_strct->y = argdefault; \
    })

#define VECTOR_ALLOC3(namespace, strct, argdefault) \
  VECTOR_ALLOC(namespace, strct, \
    { \
      allc_strct->x = argdefault; \
      allc_strct->y = argdefault; \
      allc_strct->z = argdefault; \
    })

#define VECTOR_SET2(namespace, strct, default_val, reader_conv, writer_conv) \
static VALUE \
namespace ## _set(int argc, VALUE* argv, VALUE self) \
{ \
  rb_check_frozen(self); \
  volatile VALUE rbObj; \
  volatile VALUE rbX, rbY; \
  if(argc == 0) \
  { \
    VALUE zero = reader_conv(default_val); \
    rbX      = zero; \
    rbY      = zero; \
  } \
  else if(argc == 1) \
  { \
    rb_scan_args(argc, argv, "10", &rbObj); \
    strb_CheckObjIsKindOf(rbObj, rb_c ## strct); \
    strct *vec; \
    Data_Get_Struct(rbObj, strct, vec); \
    rbX = reader_conv(vec->x); \
    rbY = reader_conv(vec->y); \
  } \
  else \
  { \
    rb_scan_args(argc, argv, "20", &rbX, &rbY); \
  } \
  strct *vector; \
  Data_Get_Struct(self, strct, vector); \
  vector->x = writer_conv(rbX); \
  vector->y = writer_conv(rbY); \
  return self; \
}

#define VECTOR_SET3(namespace, strct, default_val, reader_conv, writer_conv) \
static VALUE \
namespace ## _set(int argc, VALUE* argv, VALUE self) \
{ \
  rb_check_frozen(self); \
  volatile VALUE rbObj; \
  volatile VALUE rbX, rbY, rbZ; \
  if(argc == 0) \
  { \
    VALUE zero = reader_conv(default_val); \
    rbX      = zero; \
    rbY      = zero; \
    rbZ      = zero; \
  } \
  else if(argc == 1) \
  { \
    rb_scan_args(argc, argv, "10", &rbObj); \
    strb_CheckObjIsKindOf(rbObj, rb_c ## strct); \
    strct *vec; \
    Data_Get_Struct(rbObj, strct, vec); \
    rbX = reader_conv(vec->x); \
    rbY = reader_conv(vec->y); \
    rbZ = reader_conv(vec->z); \
  } \
  else \
  { \
    rb_scan_args(argc, argv, "30", &rbX, &rbY, &rbZ); \
  } \
  strct *vector; \
  Data_Get_Struct(self, strct, vector); \
  vector->x = writer_conv(rbX); \
  vector->y = writer_conv(rbY); \
  vector->z = writer_conv(rbZ); \
  return self; \
}

#define VECTOR_BANG_FUNC(namespace, word) \
  static VALUE \
  namespace ## _ ## word(VALUE self, VALUE rbVal) \
  { \
    VALUE obj_dup = rb_obj_dup(self); \
    return namespace ## _ ## word ## _bang(obj_dup, rbVal); \
  }

#define VECTOR_MATH_FUNC2(namespace, strct, word, symbol, reader_conv, writer_conv) \
  static VALUE \
  namespace ## _ ## word ## _bang(VALUE self, VALUE rbVal) \
  { \
    rb_check_frozen(self); \
    strct *vector; \
    Data_Get_Struct(self, strct, vector); \
    if (NUMERIC_P(rbVal)) \
    { \
      vector->x symbol ## = writer_conv(rbVal); \
      vector->y symbol ## = writer_conv(rbVal); \
    } \
    else \
    { \
      strct *vec2; \
      strb_CheckObjIsKindOf(rbVal, rb_c ## strct); \
      Data_Get_Struct(rbVal, strct, vec2); \
      vector->x symbol ## = vec2->x; \
      vector->y symbol ## = vec2->y; \
    } \
    return self; \
  } \
  VECTOR_BANG_FUNC(namespace, word);

#define VECTOR_MATH_FUNC3(namespace, strct, word, symbol, reader_conv, writer_conv) \
  static VALUE \
  namespace ## _ ## word ## _bang(VALUE self, VALUE rbVal) \
  { \
    rb_check_frozen(self); \
    strct *vector; \
    Data_Get_Struct(self, strct, vector); \
    if (NUMERIC_P(rbVal)) \
    { \
      vector->x symbol ## = writer_conv(rbVal); \
      vector->y symbol ## = writer_conv(rbVal); \
      vector->z symbol ## = writer_conv(rbVal); \
    } \
    else \
    { \
      strct *vec2; \
      strb_CheckObjIsKindOf(rbVal, rb_c ## strct); \
      Data_Get_Struct(rbVal, strct, vec2); \
      vector->x symbol ## = vec2->x; \
      vector->y symbol ## = vec2->y; \
      vector->z symbol ## = vec2->y; \
    } \
    return self; \
  } \
  VECTOR_BANG_FUNC(namespace, word);

#define VECTOR_MATH_FUNCS2(namespace, strct, reader_conv, writer_conv) \
  VECTOR_MATH_FUNC2(namespace, strct, add, +, reader_conv, writer_conv); \
  VECTOR_MATH_FUNC2(namespace, strct, sub, -, reader_conv, writer_conv); \
  VECTOR_MATH_FUNC2(namespace, strct, mul, *, reader_conv, writer_conv); \
  VECTOR_MATH_FUNC2(namespace, strct, div, /, reader_conv, writer_conv);

#define VECTOR_MATH_FUNCS3(namespace, strct, reader_conv, writer_conv) \
  VECTOR_MATH_FUNC3(namespace, strct, add, +, reader_conv, writer_conv); \
  VECTOR_MATH_FUNC3(namespace, strct, sub, -, reader_conv, writer_conv); \
  VECTOR_MATH_FUNC3(namespace, strct, mul, *, reader_conv, writer_conv); \
  VECTOR_MATH_FUNC3(namespace, strct, div, /, reader_conv, writer_conv);

#define VECTOR_TO_A2(namespace, strct, writer_conv) \
  static VALUE \
  namespace ## _to_a(VALUE self) \
  { \
    strct *obj; \
    Data_Get_Struct(self, strct, obj); \
    volatile VALUE ary = rb_ary_new(); \
    rb_ary_push(ary, writer_conv(obj->x)); \
    rb_ary_push(ary, writer_conv(obj->y)); \
    return ary; \
  }

#define VECTOR_TO_A3(namespace, strct, writer_conv) \
  static VALUE \
  namespace ## _to_a(VALUE self) \
  { \
    strct *obj; \
    Data_Get_Struct(self, strct, obj); \
    volatile VALUE ary = rb_ary_new(); \
    rb_ary_push(ary, writer_conv(obj->x)); \
    rb_ary_push(ary, writer_conv(obj->y)); \
    rb_ary_push(ary, writer_conv(obj->z)); \
    return ary; \
  }

#define VECTOR_TO_S2(namespace, strct, writer_conv) \
  static VALUE \
  namespace ## _to_s(VALUE self) \
  { \
    strct *obj; \
    Data_Get_Struct(self, strct, obj); \
    char str[256]; \
    snprintf(str, sizeof(str), \
            "#<%s x=%f, y=%f>", rb_obj_classname(self), \
            toDouble(obj->x), toDouble(obj->y)); \
    return rb_str_new2(str); \
  }

#define VECTOR_TO_S3(namespace, strct, writer_conv) \
  static VALUE \
  namespace ## _to_s(VALUE self) \
  { \
    strct *obj; \
    Data_Get_Struct(self, strct, obj); \
    char str[256]; \
    snprintf(str, sizeof(str), \
            "#<%s x=%f, y=%f z=%f>", rb_obj_classname(self), \
            toDouble(obj->x), toDouble(obj->y), toDouble(obj->z)); \
    return rb_str_new2(str); \
  }

#define VECTOR_DUMP(namespace, strct) \
static VALUE namespace ## _marshal_dump(VALUE self) { return namespace ## _to_a(self); }

#define VECTOR_MARSHAL2(namespace, strct, reader_conv, writer_conv) \
  VECTOR_DUMP(namespace, strct); \
  static VALUE \
  namespace ## _marshal_load(VALUE self, VALUE rbDumpAry) \
  { \
    strct *obj; \
    Data_Get_Struct(self, strct, obj); \
    obj->x = writer_conv(rb_ary_entry(rbDumpAry, 0)); \
    obj->y = writer_conv(rb_ary_entry(rbDumpAry, 1)); \
    return self; \
  }

#define VECTOR_MARSHAL3(namespace, strct, reader_conv, writer_conv) \
  VECTOR_DUMP(namespace, strct); \
  static VALUE \
  namespace ## _marshal_load(VALUE self, VALUE rbDumpAry) \
  { \
    strct *obj; \
    Data_Get_Struct(self, strct, obj); \
    obj->x = writer_conv(rb_ary_entry(rbDumpAry, 0)); \
    obj->y = writer_conv(rb_ary_entry(rbDumpAry, 1)); \
    obj->z = writer_conv(rb_ary_entry(rbDumpAry, 2)); \
    return self; \
  }

#define VECTOR_ACCESSOR2(namespace, strct, reader, writer) \
  STRUCT_ATTR_ACCESSOR(namespace, strct, x, reader, writer); \
  STRUCT_ATTR_ACCESSOR(namespace, strct, y, reader, writer);

#define VECTOR_ACCESSOR3(namespace, strct, reader, writer) \
  STRUCT_ATTR_ACCESSOR(namespace, strct, x, reader, writer); \
  STRUCT_ATTR_ACCESSOR(namespace, strct, y, reader, writer); \
  STRUCT_ATTR_ACCESSOR(namespace, strct, z, reader, writer);

#endif
