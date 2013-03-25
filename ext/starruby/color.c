#include "starruby.prv.h"

static volatile VALUE rb_cColor = Qundef;

VALUE
strb_GetColorClass(void)
{
  return rb_cColor;
}

static void Color_free(Color*);
STRUCT_CHECK_TYPE_FUNC(Color, Color);

inline void
strb_GetColorFromRubyValue(Color* color, VALUE rbColor)
{
  strb_CheckColor(rbColor);
  Color *src_color;
  Data_Get_Struct(rbColor, Color, src_color);
  ((Pixel*)color)->value = ((Pixel*)src_color)->value;
}

// attr_reader
static VALUE
Color_alpha(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  return INT2FIX(color.alpha);
}

static VALUE
Color_blue(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  return INT2FIX(color.blue);
}

static VALUE
Color_red(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  return INT2FIX(color.red);
}

static VALUE
Color_green(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
  return INT2FIX(color.green);
}

// attr_writer
static VALUE
Color_alpha_set(VALUE self, VALUE rbVal)
{
  Color *color;
  Data_Get_Struct(self, Color, color);
  color->alpha = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_red_set(VALUE self, VALUE rbVal)
{
  Color *color;
  Data_Get_Struct(self, Color, color);
  color->red = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_green_set(VALUE self, VALUE rbVal)
{
  Color *color;
  Data_Get_Struct(self, Color, color);
  color->green = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Color_blue_set(VALUE self, VALUE rbVal)
{
  Color *color;
  Data_Get_Struct(self, Color, color);
  color->blue = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static void
Color_free(Color* color)
{
  free(color);
}

static VALUE
Color_alloc(VALUE klass)
{
  Color *color = ALLOC(Color);
  color->red = 0;
  color->green = 0;
  color->blue = 0;
  color->alpha = 0;
  return Data_Wrap_Struct(klass, NULL, Color_free, color);
}

static VALUE
Color_set(int argc, VALUE *argv, VALUE self)
{
  volatile VALUE rbRed, rbGreen, rbBlue, rbAlpha;
  if(argc == 0) {
    VALUE v255 = INT2FIX(255);
    rbRed   = v255;
    rbBlue  = v255;
    rbGreen = v255;
    rbAlpha = v255;
  }
  else if(argc == 1) {
    volatile VALUE rbColor;
    rb_scan_args(argc, argv, "10", &rbColor);
    strb_CheckColor(rbColor);

    Color *color;
    Data_Get_Struct(rbColor, Color, color);

    rbRed   = INT2FIX(color->red);
    rbGreen = INT2FIX(color->green);
    rbBlue  = INT2FIX(color->blue);
    rbAlpha = INT2FIX(color->alpha);
  }
  else {
    rb_scan_args(argc, argv, "31",
                 &rbRed, &rbGreen, &rbBlue, &rbAlpha);
    if(NIL_P(rbAlpha)) rbAlpha = INT2FIX(255);
  }

  const int red   = MINMAXU255(NUM2INT(rbRed));
  const int green = MINMAXU255(NUM2INT(rbGreen));
  const int blue  = MINMAXU255(NUM2INT(rbBlue));
  const int alpha = MINMAXU255(NUM2INT(rbAlpha));

  if (red < 0 || 256 <= red || green < 0 || 256 <= green ||
      blue < 0 || 256 <= blue || alpha < 0 || 256 <= alpha) {
    rb_raise(rb_eArgError, "invalid color value: (r:%d, g:%d, b:%d, a:%d)",
             red, green, blue, alpha);
  }

  Color *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  trg_color->red   = red;
  trg_color->green = green;
  trg_color->blue  = blue;
  trg_color->alpha = alpha;

  return self;
}

static VALUE
Color_initialize_copy(VALUE self, VALUE rbOther)
{
  Color *src_color, *trg_color;
  Data_Get_Struct(self, Color, trg_color);
  Data_Get_Struct(rbOther, Color, src_color);
  trg_color->red   = src_color->red;
  trg_color->green = src_color->green;
  trg_color->blue  = src_color->blue;
  trg_color->alpha = src_color->alpha;

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
  return CBOOL2RUBY(color1.red   == color2.red &&
                    color1.green == color2.green &&
                    color1.blue  == color2.blue &&
                    color1.alpha == color2.alpha);
}

static VALUE
Color_hash(VALUE self)
{
  Color color;
  strb_GetColorFromRubyValue(&color, self);
#if POSFIXABLE(0xffffffff)
  const uint32_t hash = (color.alpha << 24) |
    (color.red << 16) |
    (color.green << 8) |
    color.blue;
#else
  const uint32_t hash = ((color.alpha >> 6) << 24) |
    ((color.red ^ ((color.alpha >> 4) & 0x3)) << 16) |
    ((color.green ^ ((color.alpha >> 2) & 0x3)) << 8) |
    (color.blue ^ (color.alpha & 0x3));
#endif
  return INT2FIX(hash);
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

  return rb_funcall(ary, rb_intern("pack"), 1, rb_str_new2("D4\0"));
}

static VALUE
Color_load(VALUE klass, VALUE rbDStr)
{
  volatile VALUE rbUAry = rb_funcall(
    rbDStr, rb_intern("unpack"), 1, rb_str_new2("D4\0"));

  VALUE rbArgv[4] = {
    rb_ary_entry(rbUAry, 0), // red
    rb_ary_entry(rbUAry, 1), // green
    rb_ary_entry(rbUAry, 2), // blue
    rb_ary_entry(rbUAry, 3)  // alpha
  };

  return rb_class_new_instance(4, rbArgv, klass);
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
