#include "starruby.prv.h"

volatile VALUE rb_cColor = Qundef;

#define NULL_COLOR (Color){ 0, 0, 0, 0 }

static Void Color_free(Color* color_ptr)
{
  free(color_ptr);
}

static VALUE Color_alloc(VALUE klass)
{
  Color* color = ALLOC(Color);
  ((Pixel*)color)->value = 0x00000000;
  return Data_Wrap_Struct(klass, Null, Color_free, color);
}

Void strb_ColorSet4b(Color* dst_color,
                     UByte alpha, UByte red, UByte green, UByte blue)
{
  dst_color->alpha = alpha;
  dst_color->red   = red;
  dst_color->green = green;
  dst_color->blue  = blue;
}

Void strb_CopyColorTo(Color* src_color, Color* dst_color)
{
  ((Pixel*)dst_color)->value = ((Pixel*)src_color)->value;
}

Color* strb_RubyColorPtr(VALUE rbColor)
{
  Color* color_ptr;
  Data_Get_Struct(rbColor, Color, color_ptr);
  return color_ptr;
}

VALUE strb_RubyWrapColorPtr(VALUE klass, Color* color)
{
  return Data_Wrap_Struct(klass, Null, Color_free, color);
}

VALUE strb_ColorToRuby(Color color)
{
  Color *wrap_color = ALLOC(Color);
  strb_CopyColorTo(&(color), wrap_color);
  return strb_RubyWrapColorPtr(rb_cColor, wrap_color);
}

Color strb_RubyToColor(VALUE rbObj)
{
  switch (TYPE(rbObj)) {
    case T_DATA: {
      if (rb_obj_is_kind_of(rbObj, rb_cColor)) {
        return *strb_RubyColorPtr(rbObj);
      }
    }
    case T_ARRAY: {
      if (hrbCheckArraySize(rbObj, ==, 3)) {
        Color color;
        color.red   = hrbArrayEntryAsByte(rbObj, 0);
        color.green = hrbArrayEntryAsByte(rbObj, 1);
        color.blue  = hrbArrayEntryAsByte(rbObj, 2);
        color.alpha = 0xFF;
        return color;
      } else if (hrbCheckArraySize(rbObj, ==, 4)) {
        Color color;
        color.red   = hrbArrayEntryAsByte(rbObj, 0);
        color.green = hrbArrayEntryAsByte(rbObj, 1);
        color.blue  = hrbArrayEntryAsByte(rbObj, 2);
        color.alpha = hrbArrayEntryAsByte(rbObj, 3);
        return color;
      } else {
        rb_raise(rb_eArgError, "Expected Array of size 3 or 4");
      }
    }
    default: {
      rb_raise(rb_eTypeError, "Can't convert %s into StarRuby::Color",
               rb_obj_classname(rbObj));
    }
  }
  return NULL_COLOR;
}

Boolean strb_ObjIsColor(VALUE rbObj)
{
  return rb_obj_is_kind_of(rbObj, rb_cColor);
}

inline Void strb_GetColorFromRubyValue(Color* color, VALUE rbColor)
{
  Color src_color = strb_RubyToColor(rbColor);
  ((Pixel*)color)->value = ((Pixel)src_color).value;
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
  color->alpha = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_red_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->red = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_green_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->green = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_blue_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Color* color;
  Data_Get_Struct(self, Color, color);
  color->blue = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_set(int argc, VALUE *argv, VALUE self)
{
  rb_check_frozen(self);
  volatile VALUE rbRed, rbGreen, rbBlue, rbAlpha;
  if(argc == 0) {
    VALUE v255 = INT2FIX(255);
    rbRed   = v255;
    rbBlue  = v255;
    rbGreen = v255;
    rbAlpha = v255;
  } else if (argc == 1) {
    Color color = strb_RubyToColor(argv[0]);
    rbRed   = INT2FIX(color.red);
    rbGreen = INT2FIX(color.green);
    rbBlue  = INT2FIX(color.blue);
    rbAlpha = INT2FIX(color.alpha);
  } else {
    rb_scan_args(argc, argv, "31",
                 &rbRed, &rbGreen, &rbBlue, &rbAlpha);
    if(NIL_P(rbAlpha)) rbAlpha = INT2FIX(255);
  }

  const UByte red   = MINMAXU255(NUM2INT(rbRed));
  const UByte green = MINMAXU255(NUM2INT(rbGreen));
  const UByte blue  = MINMAXU255(NUM2INT(rbBlue));
  const UByte alpha = MINMAXU255(NUM2INT(rbAlpha));

  Color *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  strb_ColorSet4b(trg_color, alpha, red, green, blue);

  return self;
}

static VALUE
Color_initialize_copy(VALUE self, VALUE rbOther)
{
  Color *src_color, *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  Data_Get_Struct(rbOther, Color, src_color);
  ((Pixel*)trg_color)->value = ((Pixel*)src_color)->value;
  return Qnil;
}

static VALUE
Color_equal(VALUE self, VALUE rbOther)
{
  if (self == rbOther) {
    return Qtrue;
  }
  if (!rb_obj_is_kind_of(rbOther, rb_cColor)) {
    return Qfalse;
  }
  Color color1, color2;
  strb_GetColorFromRubyValue(&color1, self);
  strb_GetColorFromRubyValue(&color2, rbOther);
  return CBOOL2RUBY(((Pixel)color1).value == ((Pixel)color2).value);
}

static VALUE
Color_hash(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  const Bignum hash = ((Pixel)color).value;
  return LONG2NUM(hash);
}

static VALUE
Color_to_s(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  char str[256];
  snprintf(str, sizeof(str),
           "#<%s alpha=%d, red=%d, green=%d, blue=%d>",
           rb_obj_classname(self),
           color.alpha, color.red, color.green, color.blue);
  return rb_str_new2(str);
}

static VALUE
Color_to_a(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  return rb_ary_new4(4, (VALUE[]){INT2FIX(color.red),
                                  INT2FIX(color.green),
                                  INT2FIX(color.blue),
                                  INT2FIX(color.alpha)});
}

// Marshalling
static VALUE
Color_dump(VALUE self, VALUE rbDepth)
{
  VALUE ary = Color_to_a(self);
  return rb_funcall(ary, ID_pack, 1, rb_str_new2("D4\0"));
}

static VALUE
Color_load(VALUE klass, VALUE rbDStr)
{
  volatile VALUE rbUAry = rb_funcall(
    rbDStr, ID_unpack, 1, rb_str_new2("D4\0"));

  VALUE argv[4] = {
    rb_ary_entry(rbUAry, 0), // red
    rb_ary_entry(rbUAry, 1), // green
    rb_ary_entry(rbUAry, 2), // blue
    rb_ary_entry(rbUAry, 3)  // alpha
  };

  return rb_class_new_instance(4, argv, klass);
}

VALUE
strb_InitializeColor(VALUE rb_mStarRuby)
{
  rb_cColor = rb_define_class_under(rb_mStarRuby, "Color", rb_cObject);
  rb_define_alloc_func(rb_cColor, Color_alloc);
  rb_define_private_method(rb_cColor, "initialize", Color_set, -1);
  rb_define_private_method(rb_cColor, "initialize_copy", Color_initialize_copy, 1);

  rb_define_singleton_method(rb_cColor, "_load", Color_load, 1);
  rb_define_method(rb_cColor, "_dump", Color_dump, 1);

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

  rb_define_method(rb_cColor, "to_s",  Color_to_s,  0);
  rb_define_method(rb_cColor, "to_a",  Color_to_a,  0);

  rb_define_method(rb_cColor, "eql?",  Color_equal, 1);
  rb_define_alias(rb_cColor, "==", "eql?");
  return rb_cColor;
}
