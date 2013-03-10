/*
  StarRuby
    Vector

    by IceDragon
    vr 0.90
    dc 24/02/2013
    dm 24/02/2013
 */
#include "starruby.prv.h"
#include "vector.h"
#include "vector.prv.h"

static void Vector2I_free(Vector2I*);
static void Vector3I_free(Vector3I*);
static void Vector2F_free(Vector2F*);
static void Vector3F_free(Vector3F*);

STRUCT_CHECK_TYPE_FUNC(Vector2I, Vector2I);
STRUCT_CHECK_TYPE_FUNC(Vector3I, Vector3I);
STRUCT_CHECK_TYPE_FUNC(Vector2F, Vector2F);
STRUCT_CHECK_TYPE_FUNC(Vector3F, Vector3F);

// ATTR_ACCESSOR(namespace, struct, attribute, reader, writer)
VECTOR_ACCESSOR2(Vector2I, Vector2I, INT2NUM, NUM2INT);
VECTOR_ACCESSOR2(Vector2F, Vector2F, DBL2NUM, NUM2DBL);
VECTOR_ACCESSOR3(Vector3I, Vector3I, INT2NUM, NUM2INT);
VECTOR_ACCESSOR3(Vector3F, Vector3F, DBL2NUM, NUM2DBL);

VECTOR_ALLOC2(Vector2I, Vector2I, 0);
VECTOR_ALLOC3(Vector3I, Vector3I, 0);
VECTOR_ALLOC2(Vector2F, Vector2F, 0.0);
VECTOR_ALLOC3(Vector3F, Vector3F, 0.0);

VECTOR_SET2(Vector2I, Vector2I, 0, INT2NUM, NUM2INT);
VECTOR_SET3(Vector3I, Vector3I, 0, INT2NUM, NUM2INT);
VECTOR_SET2(Vector2F, Vector2F, 0.0, DBL2NUM, NUM2DBL);
VECTOR_SET3(Vector3F, Vector3F, 0.0, DBL2NUM, NUM2DBL);

VECTOR_MATH_FUNCS2(Vector2I, Vector2I, INT2NUM, NUM2INT);
VECTOR_MATH_FUNCS2(Vector2F, Vector2F, DBL2NUM, NUM2DBL);
VECTOR_MATH_FUNCS3(Vector3I, Vector3I, INT2NUM, NUM2INT);
VECTOR_MATH_FUNCS3(Vector3F, Vector3F, DBL2NUM, NUM2DBL);

VECTOR_TO_A2(Vector2I, Vector2I, INT2NUM);
VECTOR_TO_A3(Vector3I, Vector3I, INT2NUM);
VECTOR_TO_A2(Vector2F, Vector2F, DBL2NUM);
VECTOR_TO_A3(Vector3F, Vector3F, DBL2NUM);

VECTOR_MARSHAL2(Vector2I, Vector2I, INT2NUM, NUM2INT);
VECTOR_MARSHAL3(Vector3I, Vector3I, INT2NUM, NUM2INT);
VECTOR_MARSHAL2(Vector2F, Vector2F, DBL2NUM, NUM2DBL);
VECTOR_MARSHAL3(Vector3F, Vector3F, DBL2NUM, NUM2DBL);

static VALUE rb_cVector = Qundef;
static VALUE rb_cVector2I = Qundef;
static VALUE rb_cVector2F = Qundef;
static VALUE rb_cVector3I = Qundef;
static VALUE rb_cVector3F = Qundef;

VALUE strb_InitializeVector(VALUE rb_mStarRuby)
{
  rb_cVector   = rb_define_class_under(rb_mStarRuby, "Vector", rb_cObject);
  rb_cVector2I = rb_define_class_under(rb_mStarRuby, "Vector2I", rb_cVector);
  rb_cVector2F = rb_define_class_under(rb_mStarRuby, "Vector2F", rb_cVector);
  rb_cVector3I = rb_define_class_under(rb_mStarRuby, "Vector3I", rb_cVector);
  rb_cVector3F = rb_define_class_under(rb_mStarRuby, "Vector3F", rb_cVector);

  rb_define_alloc_func(rb_cVector2I, Vector2I_alloc);
  rb_define_alloc_func(rb_cVector3I, Vector3I_alloc);
  rb_define_alloc_func(rb_cVector2F, Vector2F_alloc);
  rb_define_alloc_func(rb_cVector3F, Vector3F_alloc);

  rb_define_method(rb_cVector2I, "initialize", Vector2I_set, -1);
  rb_define_method(rb_cVector2F, "initialize", Vector2F_set, -1);
  rb_define_method(rb_cVector3I, "initialize", Vector3I_set, -1);
  rb_define_method(rb_cVector3F, "initialize", Vector3F_set, -1);

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
