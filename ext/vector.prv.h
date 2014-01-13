#ifndef STARRUBY_VECTOR_PRV_H
#define STARRUBY_VECTOR_PRV_H

#define VECTOR_ALLOC(nspc, strct, process) \
STRUCT_FREE(nspc, strct); \
static VALUE nspc ## _alloc(VALUE klass) \
{ \
  strct *allc_strct = ALLOC(strct); \
  process; \
  return Data_Wrap_Struct(klass, 0, nspc ## _free, allc_strct); \
}

#define VECTOR_ALLOC2(nspc, strct, argdefault) \
VECTOR_ALLOC(nspc, strct, \
  { \
    allc_strct->x = argdefault; \
    allc_strct->y = argdefault; \
  })

#define VECTOR_ALLOC3(nspc, strct, argdefault) \
VECTOR_ALLOC(nspc, strct, \
  { \
    allc_strct->x = argdefault; \
    allc_strct->y = argdefault; \
    allc_strct->z = argdefault; \
  })

#define VECTOR_SET2(nspc, strct, default_val, reader_conv, writer_conv) \
static VALUE \
nspc ## _set_bang(int argc, VALUE* argv, VALUE self) \
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
    strct vec; \
    strb_RubyToVector2(rbObj, &vec); \
    rbX = reader_conv(vec.x); \
    rbY = reader_conv(vec.y); \
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
} \
static VALUE \
nspc ## _set(int argc, VALUE* argv, VALUE self) \
{ \
  VALUE obj_dup = rb_obj_dup(self); \
  return nspc ## _set_bang(argc, argv, obj_dup); \
}

#define VECTOR_SET3(nspc, strct, default_val, reader_conv, writer_conv) \
static VALUE \
nspc ## _set_bang(int argc, VALUE* argv, VALUE self) \
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
    strct vec; \
    strb_RubyToVector3(rbObj, &vec); \
    rbX = reader_conv(vec.x); \
    rbY = reader_conv(vec.y); \
    rbZ = reader_conv(vec.z); \
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
} \
static VALUE \
nspc ## _set(int argc, VALUE* argv, VALUE self) \
{ \
  VALUE obj_dup = rb_obj_dup(self); \
  return nspc ## _set_bang(argc, argv, obj_dup); \
}

#define VECTOR_BANG_FUNC(nspc, word) \
static VALUE \
nspc ## _ ## word(VALUE self, VALUE rbVal) \
{ \
  VALUE obj_dup = rb_obj_dup(self); \
  return nspc ## _ ## word ## _bang(obj_dup, rbVal); \
}

#define VECTOR_MATH_FUNC2(nspc, strct, word, symbol, reader_conv, writer_conv) \
static VALUE                                             \
nspc ## _ ## word ## _bang(VALUE self, VALUE rbVal) \
{                                                        \
  rb_check_frozen(self);                                 \
  strct *vector;                                         \
  strct src_vector;                                      \
  strb_RubyToVector2(rbVal, &src_vector);                \
  Data_Get_Struct(self, strct, vector);                  \
  vector->x symbol ## = src_vector.x;                    \
  vector->y symbol ## = src_vector.y;                    \
  return self;                                           \
}                                                        \
VECTOR_BANG_FUNC(nspc, word);

#define VECTOR_MATH_FUNC3(nspc, strct, word, symbol, reader_conv, writer_conv) \
static VALUE                                             \
nspc ## _ ## word ## _bang(VALUE self, VALUE rbVal) \
{                                                        \
  rb_check_frozen(self);                                 \
  strct *vector;                                         \
  strct src_vector;                                      \
  strb_RubyToVector3(rbVal, &src_vector);                \
  Data_Get_Struct(self, strct, vector);                  \
  vector->x symbol ## = src_vector.x;                    \
  vector->y symbol ## = src_vector.y;                    \
  vector->z symbol ## = src_vector.z;                    \
  return self;                                           \
}                                                        \
VECTOR_BANG_FUNC(nspc, word);

#define VECTOR_MATH_FUNCS2(nspc, strct, reader_conv, writer_conv) \
VECTOR_MATH_FUNC2(nspc, strct, add, +, reader_conv, writer_conv); \
VECTOR_MATH_FUNC2(nspc, strct, sub, -, reader_conv, writer_conv); \
VECTOR_MATH_FUNC2(nspc, strct, mul, *, reader_conv, writer_conv); \
VECTOR_MATH_FUNC2(nspc, strct, div, /, reader_conv, writer_conv);

#define VECTOR_MATH_FUNCS3(nspc, strct, reader_conv, writer_conv) \
VECTOR_MATH_FUNC3(nspc, strct, add, +, reader_conv, writer_conv); \
VECTOR_MATH_FUNC3(nspc, strct, sub, -, reader_conv, writer_conv); \
VECTOR_MATH_FUNC3(nspc, strct, mul, *, reader_conv, writer_conv); \
VECTOR_MATH_FUNC3(nspc, strct, div, /, reader_conv, writer_conv);

#define VECTOR_TO_A2(nspc, strct, writer_conv) \
static VALUE \
nspc ## _to_a(VALUE self) \
{ \
  strct *obj; \
  Data_Get_Struct(self, strct, obj); \
  volatile VALUE ary = rb_ary_new(); \
  rb_ary_push(ary, writer_conv(obj->x)); \
  rb_ary_push(ary, writer_conv(obj->y)); \
  return ary; \
}

#define VECTOR_TO_A3(nspc, strct, writer_conv) \
static VALUE \
nspc ## _to_a(VALUE self) \
{ \
  strct *obj; \
  Data_Get_Struct(self, strct, obj); \
  volatile VALUE ary = rb_ary_new(); \
  rb_ary_push(ary, writer_conv(obj->x)); \
  rb_ary_push(ary, writer_conv(obj->y)); \
  rb_ary_push(ary, writer_conv(obj->z)); \
  return ary; \
}

#define VECTOR_TO_S2(nspc, strct, writer_conv)     \
static VALUE                                          \
nspc ## _to_s(VALUE self)                        \
{                                                     \
  strct *obj;                                         \
  Data_Get_Struct(self, strct, obj);                  \
  char str[256];                                      \
  snprintf(str, sizeof(str),                          \
          "#<%s x=%f, y=%f>", rb_obj_classname(self), \
          (double)obj->x, (double)obj->y);        \
  return rb_str_new2(str);                            \
}

#define VECTOR_TO_S3(nspc, strct, writer_conv)                \
static VALUE                                                     \
nspc ## _to_s(VALUE self)                                   \
{                                                                \
  strct *obj;                                                    \
  Data_Get_Struct(self, strct, obj);                             \
  char str[256];                                                 \
  snprintf(str, sizeof(str),                                     \
          "#<%s x=%f, y=%f z=%f>", rb_obj_classname(self),       \
          (double)obj->x, (double)obj->y, (double)obj->z); \
  return rb_str_new2(str);                                       \
}

#define VECTOR_DUMP(nspc, strct) \
static VALUE nspc ## _marshal_dump(VALUE self) { return nspc ## _to_a(self); }

#define VECTOR_MARSHAL2(nspc, strct, reader_conv, writer_conv) \
VECTOR_DUMP(nspc, strct); \
static VALUE \
nspc ## _marshal_load(VALUE self, VALUE rbDumpAry) \
{ \
  strct *obj; \
  Data_Get_Struct(self, strct, obj); \
  obj->x = writer_conv(rb_ary_entry(rbDumpAry, 0)); \
  obj->y = writer_conv(rb_ary_entry(rbDumpAry, 1)); \
  return self; \
}

#define VECTOR_MARSHAL3(nspc, strct, reader_conv, writer_conv) \
VECTOR_DUMP(nspc, strct); \
static VALUE \
nspc ## _marshal_load(VALUE self, VALUE rbDumpAry) \
{ \
  strct *obj; \
  Data_Get_Struct(self, strct, obj); \
  obj->x = writer_conv(rb_ary_entry(rbDumpAry, 0)); \
  obj->y = writer_conv(rb_ary_entry(rbDumpAry, 1)); \
  obj->z = writer_conv(rb_ary_entry(rbDumpAry, 2)); \
  return self; \
}

#define VECTOR_ACCESSOR2(nspc, strct, reader, writer) \
STRUCT_ATTR_ACCESSOR(nspc, strct, x, reader, writer); \
STRUCT_ATTR_ACCESSOR(nspc, strct, y, reader, writer);

#define VECTOR_ACCESSOR3(nspc, strct, reader, writer) \
STRUCT_ATTR_ACCESSOR(nspc, strct, x, reader, writer); \
STRUCT_ATTR_ACCESSOR(nspc, strct, y, reader, writer); \
STRUCT_ATTR_ACCESSOR(nspc, strct, z, reader, writer);

#endif
