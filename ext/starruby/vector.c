/*
  StarRuby
    Vector

    by IceDragon
    vr 0.9.1
    dc 24/02/2013
    dm 21/04/2013
 */
#include "starruby.prv.h"
#include "vector.h"
#include "vector.prv.h"

volatile VALUE rb_cVector   = Qundef;
volatile VALUE rb_cVector2  = Qundef;
volatile VALUE rb_cVector3  = Qundef;
volatile VALUE rb_cVector2I = Qundef;
volatile VALUE rb_cVector3I = Qundef;
volatile VALUE rb_cVector2F = Qundef;
volatile VALUE rb_cVector3F = Qundef;

// ATTR_ACCESSOR(namespace, struct, attribute, reader, writer)
VECTOR_ACCESSOR2(Vector2I, Vector2, DBL2FIX, NUM2DBL);
VECTOR_ACCESSOR2(Vector2F, Vector2, DBL2NUM, NUM2DBL);
VECTOR_ACCESSOR3(Vector3I, Vector3, DBL2FIX, NUM2DBL);
VECTOR_ACCESSOR3(Vector3F, Vector3, DBL2NUM, NUM2DBL);

VECTOR_ALLOC2(Vector2I, Vector2, 0);
VECTOR_ALLOC3(Vector3I, Vector3, 0);
VECTOR_ALLOC2(Vector2F, Vector2, 0.0);
VECTOR_ALLOC3(Vector3F, Vector3, 0.0);

VECTOR_SET2(Vector2I, Vector2, 0, DBL2FIX, NUM2DBL);
VECTOR_SET3(Vector3I, Vector3, 0, DBL2FIX, NUM2DBL);
VECTOR_SET2(Vector2F, Vector2, 0.0, DBL2NUM, NUM2DBL);
VECTOR_SET3(Vector3F, Vector3, 0.0, DBL2NUM, NUM2DBL);

VECTOR_MATH_FUNCS2(Vector2I, Vector2, DBL2FIX, NUM2DBL);
VECTOR_MATH_FUNCS2(Vector2F, Vector2, DBL2NUM, NUM2DBL);
VECTOR_MATH_FUNCS3(Vector3I, Vector3, DBL2FIX, NUM2DBL);
VECTOR_MATH_FUNCS3(Vector3F, Vector3, DBL2NUM, NUM2DBL);

VECTOR_TO_A2(Vector2I, Vector2, DBL2FIX);
VECTOR_TO_A3(Vector3I, Vector3, DBL2FIX);
VECTOR_TO_A2(Vector2F, Vector2, DBL2NUM);
VECTOR_TO_A3(Vector3F, Vector3, DBL2NUM);

VECTOR_TO_S2(Vector2I, Vector2, DBL2FIX);
VECTOR_TO_S3(Vector3I, Vector3, DBL2FIX);
VECTOR_TO_S2(Vector2F, Vector2, DBL2NUM);
VECTOR_TO_S3(Vector3F, Vector3, DBL2NUM);

VECTOR_MARSHAL2(Vector2I, Vector2, DBL2FIX, NUM2DBL);
VECTOR_MARSHAL3(Vector3I, Vector3, DBL2FIX, NUM2DBL);
VECTOR_MARSHAL2(Vector2F, Vector2, DBL2NUM, NUM2DBL);
VECTOR_MARSHAL3(Vector3F, Vector3, DBL2NUM, NUM2DBL);

Void strb_RubyToVector2(VALUE rbObj, Vector2* vec2)
{
  switch (TYPE(rbObj)) {
    case T_FLOAT:
    case T_FIXNUM:
    case T_BIGNUM: {
      vec2->x = NUM2DBL(rbObj);
      vec2->y = vec2->x;
    }
    case T_DATA: {
      strb_CheckObjIsKindOf(rbObj, rb_cVector2);
      Vector2* src_vec2;
      Data_Get_Struct(rbObj, Vector2, src_vec2);
      *vec2 = *src_vec2;
      break;
    }
    case T_ARRAY: {
      if (hrbCheckArraySize(rbObj, ==, 1)) {
        vec2->x = hrbArrayEntryAsDouble(rbObj, 0);
        vec2->y = vec2->x;
      } else if (hrbCheckArraySize(rbObj, ==, 2)) {
        vec2->x = hrbArrayEntryAsDouble(rbObj, 0);
        vec2->y = hrbArrayEntryAsDouble(rbObj, 1);
      } else {
        rb_raise(rb_eArgError, "Expected %s of size 1 or 2",
                 rb_class2name(rb_cArray));
      }
      break;
    }
    default: {
      rb_raise(rb_eTypeError, "Can't convert %s into %s",
               rb_obj_classname(rbObj), rb_class2name(rb_cVector2));
    }
  }
}

Void strb_RubyToVector3(VALUE rbObj, Vector3* vec3)
{
  switch (TYPE(rbObj)) {
    case T_FLOAT:
    case T_FIXNUM:
    case T_BIGNUM: {
      vec3->x = NUM2DBL(rbObj);
      vec3->y = vec3->x;
      vec3->z = vec3->y;
    }
    case T_DATA: {
      strb_CheckObjIsKindOf(rbObj, rb_cVector3);
      Vector3* src_vec3;
      Data_Get_Struct(rbObj, Vector3, src_vec3);
      *vec3 = *src_vec3;
      break;
    }
    case T_ARRAY: {
      if (hrbCheckArraySize(rbObj, ==, 1)) {
        vec3->x = hrbArrayEntryAsDouble(rbObj, 0);
        vec3->y = vec3->x;
        vec3->z = vec3->y;
      } else if (hrbCheckArraySize(rbObj, ==, 3)) {
        vec3->x = hrbArrayEntryAsDouble(rbObj, 0);
        vec3->y = hrbArrayEntryAsDouble(rbObj, 1);
        vec3->z = hrbArrayEntryAsDouble(rbObj, 2);
      } else {
        rb_raise(rb_eArgError, "Expected %s of size 1 or 3",
                 rb_class2name(rb_cArray));
      }
      break;
    }
    default: {
      rb_raise(rb_eTypeError, "Can't convert %s into %s",
               rb_obj_classname(rbObj), rb_class2name(rb_cVector3));
    }
  }
}

static VALUE Vector_compare(VALUE self, VALUE rbOther)
{
  VALUE ary1, ary2;
  ary1 = rb_funcall(self, ID_to_a, 0);
  ary2 = rb_funcall(rbOther, ID_to_a, 0);
  return rb_funcall(self, ID_compare, 2, ary1, ary2);
}

static VALUE Vector2_to_Vector2F(VALUE self)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  return rb_class_new_instance(2, (VALUE[]){DBL2NUM(vec2->x),
                                            DBL2NUM(vec2->y)},
                               rb_cVector2F);
}

static VALUE Vector2_to_Vector2I(VALUE self)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  return rb_class_new_instance(2, (VALUE[]){DBL2NUM(vec2->x),
                                            DBL2NUM(vec2->y)},
                               rb_cVector2I);
}

static VALUE Vector3_to_Vector3F(VALUE self)
{
  Vector3* vec3;
  Data_Get_Struct(self, Vector3, vec3);
  return rb_class_new_instance(3, (VALUE[]){DBL2NUM(vec3->x),
                                            DBL2NUM(vec3->y),
                                            DBL2NUM(vec3->z)},
                               rb_cVector3F);
}

static VALUE Vector3_to_Vector3I(VALUE self)
{
  Vector3* vec3;
  Data_Get_Struct(self, Vector3, vec3);
  return rb_class_new_instance(3, (VALUE[]){DBL2NUM(vec3->x),
                                            DBL2NUM(vec3->y),
                                            DBL2NUM(vec3->z)},
                               rb_cVector3I);
}

Double strb_Vector2Magnitude(Vector2* vec2)
{
  return sqrt(vec2->x * vec2->x + vec2->y * vec2->y);
}

Double strb_Vector3Magnitude(Vector3* vec3)
{
  return sqrt(vec3->x * vec3->x + vec3->y * vec3->y + vec3->z * vec3->z);
}

Double strb_Vector2Radian(Vector2* vec2)
{
  return atan2(vec2->y, vec2->x);
}

static VALUE Vector2_magnitude(VALUE self)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  return DBL2NUM(strb_Vector2Magnitude(vec2));
}

static VALUE Vector2_magnitude_eq(VALUE self, VALUE rbMagnitude)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  Double new_magnitude = NUM2DBL(rbMagnitude);
  Double rad = strb_Vector2Radian(vec2);
  vec2->x = new_magnitude * cos(rad);
  vec2->y = new_magnitude * sin(rad);
  return Qnil;
}

static VALUE Vector3_magnitude(VALUE self)
{
  Vector3* vec3;
  Data_Get_Struct(self, Vector3, vec3);
  return DBL2NUM(strb_Vector3Magnitude(vec3));
}

static VALUE Vector2_radian(VALUE self)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  return DBL2NUM(strb_Vector2Radian(vec2));
}

static VALUE Vector2_radian_eq(VALUE self, VALUE rbRadian)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  Double new_radian = NUM2DBL(rbRadian);
  Double mag = strb_Vector2Magnitude(vec2);
  vec2->x = mag * cos(new_radian);
  vec2->y = mag * sin(new_radian);
  return Qnil;
}

static VALUE Vector2_is_zero(VALUE self)
{
  Vector2* vec2;
  Data_Get_Struct(self, Vector2, vec2);
  return CBOOL2RVAL((vec2->x == 0.0) && (vec2->y == 0.0));
}

static VALUE Vector3_is_zero(VALUE self)
{
  Vector3* vec3;
  Data_Get_Struct(self, Vector3, vec3);
  return CBOOL2RVAL((vec3->x == 0.0) && (vec3->y == 0.0) && (vec3->z == 0.0));
}

static VALUE Vector2_s_zero(VALUE klass)
{
  return rb_class_new_instance(2, (VALUE[]){DBL2NUM(0.0),
                                            DBL2NUM(0.0)},
                                klass);
}

static VALUE Vector3_s_zero(VALUE klass)
{
  return rb_class_new_instance(3, (VALUE[]){DBL2NUM(0.0),
                                            DBL2NUM(0.0),
                                            DBL2NUM(0.0)},
                                klass);
}

static VALUE Vector2_s_one(VALUE klass)
{
  return rb_class_new_instance(2, (VALUE[]){DBL2NUM(1.0),
                                            DBL2NUM(1.0)},
                                klass);
}

static VALUE Vector3_s_one(VALUE klass)
{
  return rb_class_new_instance(3, (VALUE[]){DBL2NUM(1.0),
                                            DBL2NUM(1.0),
                                            DBL2NUM(1.0)},
                                klass);
}

static VALUE Vector2_init_copy(VALUE self, VALUE rbOther)
{
  Vector2* src_vec2;
  Vector2* dst_vec2;
  Data_Get_Struct(rbOther, Vector2, src_vec2);
  Data_Get_Struct(self, Vector2, dst_vec2);
  dst_vec2->x = src_vec2->x;
  dst_vec2->y = src_vec2->y;
  return Qnil;
}

static VALUE Vector3_init_copy(VALUE self, VALUE rbOther)
{
  Vector3* src_vec3;
  Vector3* dst_vec3;
  Data_Get_Struct(rbOther, Vector3, src_vec3);
  Data_Get_Struct(self, Vector3, dst_vec3);
  dst_vec3->x = src_vec3->x;
  dst_vec3->y = src_vec3->y;
  dst_vec3->z = src_vec3->z;
  return Qnil;
}

VALUE strb_InitializeVector(VALUE rb_mStarRuby)
{
  rb_cVector   = rb_define_class_under(rb_mStarRuby, "Vector", rb_cObject);
  rb_cVector2  = rb_define_class_under(rb_mStarRuby, "Vector2", rb_cVector);
  rb_cVector3  = rb_define_class_under(rb_mStarRuby, "Vector3", rb_cVector);
  rb_cVector2I = rb_define_class_under(rb_mStarRuby, "Vector2I", rb_cVector2);
  rb_cVector2F = rb_define_class_under(rb_mStarRuby, "Vector2F", rb_cVector2);
  rb_cVector3I = rb_define_class_under(rb_mStarRuby, "Vector3I", rb_cVector3);
  rb_cVector3F = rb_define_class_under(rb_mStarRuby, "Vector3F", rb_cVector3);

  rb_include_module(rb_cVector, rb_mComparable);
  rb_define_method(rb_cVector, "<=>", Vector_compare, 1);

  rb_define_singleton_method(rb_cVector2, "zero", Vector2_s_zero, 0);
  rb_define_singleton_method(rb_cVector3, "zero", Vector3_s_zero, 0);
  rb_define_singleton_method(rb_cVector2, "one", Vector2_s_one, 0);
  rb_define_singleton_method(rb_cVector3, "one", Vector3_s_one, 0);

  rb_define_method(rb_cVector2, "to_vec2i", Vector2_to_Vector2I, 0);
  rb_define_method(rb_cVector2, "to_vec2f", Vector2_to_Vector2F, 0);
  rb_define_method(rb_cVector2, "magnitude", Vector2_magnitude, 0);
  rb_define_method(rb_cVector2, "magnitude=", Vector2_magnitude_eq, 1);
  rb_define_method(rb_cVector2, "radian", Vector2_radian, 0);
  rb_define_method(rb_cVector2, "radian=", Vector2_radian_eq, 1);
  rb_define_method(rb_cVector2, "zero?", Vector2_is_zero, 0);
  rb_define_private_method(rb_cVector2, "initialize_copy", Vector2_init_copy, 1);

  rb_define_method(rb_cVector3, "to_vec3i", Vector3_to_Vector3I, 0);
  rb_define_method(rb_cVector3, "to_vec3f", Vector3_to_Vector3F, 0);
  rb_define_method(rb_cVector3, "magnitude", Vector3_magnitude, 0);
  rb_define_method(rb_cVector3, "zero?", Vector3_is_zero, 0);
  rb_define_private_method(rb_cVector3, "initialize_copy", Vector3_init_copy, 1);

  rb_define_alloc_func(rb_cVector2I, Vector2I_alloc);
  rb_define_alloc_func(rb_cVector3I, Vector3I_alloc);
  rb_define_alloc_func(rb_cVector2F, Vector2F_alloc);
  rb_define_alloc_func(rb_cVector3F, Vector3F_alloc);

  rb_define_private_method(rb_cVector2I, "initialize", Vector2I_set, -1);
  rb_define_private_method(rb_cVector2F, "initialize", Vector2F_set, -1);
  rb_define_private_method(rb_cVector3I, "initialize", Vector3I_set, -1);
  rb_define_private_method(rb_cVector3F, "initialize", Vector3F_set, -1);

  rb_define_method(rb_cVector2I, "set", Vector2I_set, -1);
  rb_define_method(rb_cVector2F, "set", Vector2F_set, -1);
  rb_define_method(rb_cVector3I, "set", Vector3I_set, -1);
  rb_define_method(rb_cVector3F, "set", Vector3F_set, -1);

  rb_define_method(rb_cVector2I, "x", Vector2I_get_x, 0);
  rb_define_method(rb_cVector2I, "y", Vector2I_get_y, 0);
  rb_define_method(rb_cVector2I, "x=", Vector2I_set_x, 1);
  rb_define_method(rb_cVector2I, "y=", Vector2I_set_y, 1);

  rb_define_method(rb_cVector2F, "x", Vector2F_get_x, 0);
  rb_define_method(rb_cVector2F, "y", Vector2F_get_y, 0);
  rb_define_method(rb_cVector2F, "x=", Vector2F_set_x, 1);
  rb_define_method(rb_cVector2F, "y=", Vector2F_set_y, 1);

  rb_define_method(rb_cVector3I, "x", Vector3I_get_x, 0);
  rb_define_method(rb_cVector3I, "y", Vector3I_get_y, 0);
  rb_define_method(rb_cVector3I, "z", Vector3I_get_z, 0);
  rb_define_method(rb_cVector3I, "x=", Vector3I_set_x, 1);
  rb_define_method(rb_cVector3I, "y=", Vector3I_set_y, 1);
  rb_define_method(rb_cVector3I, "z=", Vector3I_set_z, 1);

  rb_define_method(rb_cVector3F, "x", Vector3F_get_x, 0);
  rb_define_method(rb_cVector3F, "y", Vector3F_get_y, 0);
  rb_define_method(rb_cVector3F, "z", Vector3F_get_z, 0);
  rb_define_method(rb_cVector3F, "x=", Vector3F_set_x, 1);
  rb_define_method(rb_cVector3F, "y=", Vector3F_set_y, 1);
  rb_define_method(rb_cVector3F, "z=", Vector3F_set_z, 1);

  rb_define_method(rb_cVector2I, "add!", Vector2I_add_bang, 1);
  rb_define_method(rb_cVector2I, "sub!", Vector2I_sub_bang, 1);
  rb_define_method(rb_cVector2I, "mul!", Vector2I_mul_bang, 1);
  rb_define_method(rb_cVector2I, "div!", Vector2I_div_bang, 1);
  rb_define_method(rb_cVector2I, "add", Vector2I_add, 1);
  rb_define_method(rb_cVector2I, "sub", Vector2I_sub, 1);
  rb_define_method(rb_cVector2I, "mul", Vector2I_mul, 1);
  rb_define_method(rb_cVector2I, "div", Vector2I_div, 1);
  rb_define_alias(rb_cVector2I, "+", "add");
  rb_define_alias(rb_cVector2I, "-", "sub");
  rb_define_alias(rb_cVector2I, "*", "mul");
  rb_define_alias(rb_cVector2I, "/", "div");

  rb_define_method(rb_cVector3I, "add!", Vector3I_add_bang, 1);
  rb_define_method(rb_cVector3I, "sub!", Vector3I_sub_bang, 1);
  rb_define_method(rb_cVector3I, "mul!", Vector3I_mul_bang, 1);
  rb_define_method(rb_cVector3I, "div!", Vector3I_div_bang, 1);
  rb_define_method(rb_cVector3I, "add", Vector3I_add, 1);
  rb_define_method(rb_cVector3I, "sub", Vector3I_sub, 1);
  rb_define_method(rb_cVector3I, "mul", Vector3I_mul, 1);
  rb_define_method(rb_cVector3I, "div", Vector3I_div, 1);
  rb_define_alias(rb_cVector3I, "+", "add");
  rb_define_alias(rb_cVector3I, "-", "sub");
  rb_define_alias(rb_cVector3I, "*", "mul");
  rb_define_alias(rb_cVector3I, "/", "div");

  rb_define_method(rb_cVector2F, "add!", Vector2F_add_bang, 1);
  rb_define_method(rb_cVector2F, "sub!", Vector2F_sub_bang, 1);
  rb_define_method(rb_cVector2F, "mul!", Vector2F_mul_bang, 1);
  rb_define_method(rb_cVector2F, "div!", Vector2F_div_bang, 1);
  rb_define_method(rb_cVector2F, "add", Vector2F_add, 1);
  rb_define_method(rb_cVector2F, "sub", Vector2F_sub, 1);
  rb_define_method(rb_cVector2F, "mul", Vector2F_mul, 1);
  rb_define_method(rb_cVector2F, "div", Vector2F_div, 1);
  rb_define_alias(rb_cVector2F, "+", "add");
  rb_define_alias(rb_cVector2F, "-", "sub");
  rb_define_alias(rb_cVector2F, "*", "mul");
  rb_define_alias(rb_cVector2F, "/", "div");

  rb_define_method(rb_cVector3F, "add!", Vector3F_add_bang, 1);
  rb_define_method(rb_cVector3F, "sub!", Vector3F_sub_bang, 1);
  rb_define_method(rb_cVector3F, "mul!", Vector3F_mul_bang, 1);
  rb_define_method(rb_cVector3F, "div!", Vector3F_div_bang, 1);
  rb_define_method(rb_cVector3F, "add", Vector3F_add, 1);
  rb_define_method(rb_cVector3F, "sub", Vector3F_sub, 1);
  rb_define_method(rb_cVector3F, "mul", Vector3F_mul, 1);
  rb_define_method(rb_cVector3F, "div", Vector3F_div, 1);
  rb_define_alias(rb_cVector3F, "+", "add");
  rb_define_alias(rb_cVector3F, "-", "sub");
  rb_define_alias(rb_cVector3F, "*", "mul");
  rb_define_alias(rb_cVector3F, "/", "div");

  rb_define_method(rb_cVector2I, "to_a", Vector2I_to_a, 0);
  rb_define_method(rb_cVector3I, "to_a", Vector3I_to_a, 0);
  rb_define_method(rb_cVector2F, "to_a", Vector2F_to_a, 0);
  rb_define_method(rb_cVector3F, "to_a", Vector3F_to_a, 0);

  rb_define_method(rb_cVector2I, "to_s", Vector2I_to_s, 0);
  rb_define_method(rb_cVector3I, "to_s", Vector3I_to_s, 0);
  rb_define_method(rb_cVector2F, "to_s", Vector2F_to_s, 0);
  rb_define_method(rb_cVector3F, "to_s", Vector3F_to_s, 0);

  rb_define_method(rb_cVector2I, "marshal_dump", Vector2I_marshal_dump, 0);
  rb_define_method(rb_cVector3I, "marshal_dump", Vector3I_marshal_dump, 0);
  rb_define_method(rb_cVector2F, "marshal_dump", Vector2F_marshal_dump, 0);
  rb_define_method(rb_cVector3F, "marshal_dump", Vector3F_marshal_dump, 0);

  rb_define_method(rb_cVector2I, "marshal_load", Vector2I_marshal_load, 1);
  rb_define_method(rb_cVector3I, "marshal_load", Vector3I_marshal_load, 1);
  rb_define_method(rb_cVector2F, "marshal_load", Vector2F_marshal_load, 1);
  rb_define_method(rb_cVector3F, "marshal_load", Vector3F_marshal_load, 1);

  return Qtrue;
}
