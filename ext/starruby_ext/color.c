/*
 * StarRuby Color
 */
#include "starruby.prv.h"
#include "color.h"

volatile VALUE rb_cColor = Qundef;

static void
Color_free(Color* color_ptr)
{
  free(color_ptr);
}

static VALUE
Color_alloc(VALUE klass)
{
  Color* color = ALLOC(Color);
  ((Pixel*)color)->value = (uint32_t)0x00000000;
  return Data_Wrap_Struct(klass, NULL, Color_free, color);
}

void
strb_RubyToColor(VALUE rbObj, Color* color)
{
  switch (TYPE(rbObj)) {
    case T_FIXNUM:
    case T_FLOAT: {
      color->red   = MINMAXU255(NUM2INT(rbObj));
      color->green = color->red;
      color->blue  = color->red;
      color->alpha = 255;
      break;
    }
    case T_DATA: {
      Color* src_color;
      strb_AssertObjIsKindOf(rbObj, rb_cColor);
      Data_Get_Struct(rbObj, Color, src_color);
      *color = *src_color;
      break;
    }
    case T_ARRAY: {
      if (hrbCheckArraySize(rbObj, ==, 3)) {
        color->red   = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 0)));
        color->green = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 1)));
        color->blue  = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 2)));
        color->alpha = 255;
      } else if (hrbCheckArraySize(rbObj, ==, 4)) {
        color->red   = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 0)));
        color->green = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 1)));
        color->blue  = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 2)));
        color->alpha = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 3)));
      } else {
        rb_raise(rb_eArgError, "Expected %s of size 3 or 4",
                 rb_class2name(rb_cArray));
      }
      break;
    }
    default: {
      rb_raise(rb_eTypeError, "Can't convert %s into %s",
               rb_obj_classname(rbObj), rb_class2name(rb_cColor));
    }
  }
}

// attr_reader
static VALUE
Color_alpha(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  return INT2FIX(color->alpha);
}

static VALUE
Color_red(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  return INT2FIX(color->red);
}

static VALUE
Color_green(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  return INT2FIX(color->green);
}

static VALUE
Color_blue(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  return INT2FIX(color->blue);
}

// attr_writer
static VALUE
Color_alpha_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->alpha = MINMAXU255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Color_red_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->red = MINMAXU255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Color_green_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->green = MINMAXU255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Color_blue_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->blue = MINMAXU255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Color_set(int argc, VALUE *argv, VALUE self)
{
  rb_check_frozen(self);
  Color *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  if(argc == 0) {
    trg_color->red   = 255;
    trg_color->green = 255;
    trg_color->blue  = 255;
    trg_color->alpha = 255;
  } else if (argc == 1) {
    Color color;
    strb_RubyToColor(argv[0], &(color));
    *trg_color = color;
  } else {
    VALUE rbRed, rbGreen, rbBlue, rbAlpha;
    rb_scan_args(argc, argv, "31",
                 &rbRed, &rbGreen, &rbBlue, &rbAlpha);
    if(NIL_P(rbAlpha)) rbAlpha = INT2FIX(255);
    trg_color->red   = (uint8_t)MINMAXU255(NUM2INT(rbRed));
    trg_color->green = (uint8_t)MINMAXU255(NUM2INT(rbGreen));
    trg_color->blue  = (uint8_t)MINMAXU255(NUM2INT(rbBlue));
    trg_color->alpha = (uint8_t)MINMAXU255(NUM2INT(rbAlpha));
  }
  return self;
}

static VALUE
Color_initialize(int argc, VALUE *argv, VALUE self)
{
  Color_set(argc, argv, self);
  return Qnil;
}

static VALUE
Color_initialize_copy(VALUE self, VALUE rbOther)
{
  Color *src_color, *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  Data_Get_Struct(rbOther, Color, src_color);
  *trg_color = *src_color;
  return Qnil;
}

static VALUE
Color_is_equal(VALUE self, VALUE rbOther)
{
  if (self == rbOther) {
    return Qtrue;
  }
  if (!rb_obj_is_kind_of(rbOther, rb_cColor)) {
    return Qfalse;
  }
  Color *src_color, *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  Data_Get_Struct(rbOther, Color, src_color);
  return CBOOL2RVAL(((Pixel*)src_color)->value == ((Pixel*)trg_color)->value);
}

static VALUE
Color_to_a(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  return rb_ary_new4(4, (VALUE[]){INT2FIX(color->red),
                                  INT2FIX(color->green),
                                  INT2FIX(color->blue),
                                  INT2FIX(color->alpha)});
}

static VALUE
Color_to_s(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  char str[256];
  snprintf(str, sizeof(str),
           "%d, %d, %d, %d", color->red, color->green, color->blue, color->alpha);
  return rb_str_new2(str);
}

static VALUE
Color_inspect(VALUE self)
{
  Color* color;
  Data_Get_Struct(self, Color, color);
  char str[256];
  snprintf(str, sizeof(str),
           "#<%s red=%d, green=%d, blue=%d, alpha=%d>",
           rb_obj_classname(self),
           color->red, color->green, color->blue, color->alpha);
  return rb_str_new2(str);
}

static VALUE
Color_hash(VALUE self)
{
  return rb_funcall(Color_to_a(self), ID_hash, 0);
}

// Marshalling
static VALUE
Color_dump(VALUE self, VALUE rbDepth)
{
  VALUE ary = Color_to_a(self);
  return rb_funcall(ary, ID_pack, 1, rb_str_new2("D4"));
}

static VALUE
Color_load(VALUE klass, VALUE rbDStr)
{
  volatile VALUE rbUAry = rb_funcall(
    rbDStr, ID_unpack, 1, rb_str_new2("D4"));

  VALUE argv[4] = {
    rb_ary_entry(rbUAry, 0), // red
    rb_ary_entry(rbUAry, 1), // green
    rb_ary_entry(rbUAry, 2), // blue
    rb_ary_entry(rbUAry, 3)  // alpha
  };

  return rb_class_new_instance(4, argv, klass);
}

static VALUE
Color_s_cast(VALUE klass, VALUE rbObj)
{
  Color* color;
  VALUE rbColor;
  rbColor = rb_class_new_instance(0, (VALUE[]){}, klass);
  Data_Get_Struct(rbColor, Color, color);
  strb_RubyToColor(rbObj, color);
  return rbColor;
}

VALUE
strb_InitializeColor(VALUE rb_mStarRuby)
{
  rb_cColor = rb_define_class_under(rb_mStarRuby, "Color", rb_cObject);
  rb_define_alloc_func(rb_cColor, Color_alloc);

  rb_define_singleton_method(rb_cColor, "_load", Color_load, 1);
  rb_define_private_method(rb_cColor, "_dump", Color_dump, 1);

  rb_define_singleton_method(rb_cColor, "cast", Color_s_cast, 1);

  rb_define_private_method(rb_cColor, "initialize", Color_initialize, -1);
  rb_define_private_method(rb_cColor, "initialize_copy", Color_initialize_copy, 1);

  rb_define_method(rb_cColor, "set", Color_set, -1);

  rb_define_method(rb_cColor, "alpha", Color_alpha, 0);
  rb_define_method(rb_cColor, "red",   Color_red,   0);
  rb_define_method(rb_cColor, "blue",  Color_blue,  0);
  rb_define_method(rb_cColor, "green", Color_green, 0);

  rb_define_method(rb_cColor, "alpha=", Color_alpha_set, 1);
  rb_define_method(rb_cColor, "red=",   Color_red_set,   1);
  rb_define_method(rb_cColor, "blue=",  Color_blue_set,  1);
  rb_define_method(rb_cColor, "green=", Color_green_set, 1);

  rb_define_method(rb_cColor, "hash",  Color_hash,  0);

  rb_define_method(rb_cColor, "to_a",  Color_to_a,  0);
  rb_define_method(rb_cColor, "to_s",  Color_to_s,  0);
  rb_define_method(rb_cColor, "inspect",  Color_inspect,  0);

  rb_define_method(rb_cColor, "eql?",  Color_is_equal, 1);
  rb_define_alias(rb_cColor, "==", "eql?");
  return rb_cColor;
}