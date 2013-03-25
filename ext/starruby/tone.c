#include "starruby.prv.h"

static volatile VALUE rb_cTone = Qundef;

VALUE
strb_GetToneClass(void)
{
  return rb_cTone;
}

static void Tone_free(Tone*);
STRUCT_CHECK_TYPE_FUNC(Tone, Tone);

// attr_reader
static VALUE
Tone_saturation(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return INT2FIX(tone->saturation);
}

static VALUE
Tone_grey(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return INT2FIX(255 - tone->saturation);
}

static VALUE
Tone_blue(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return INT2FIX(tone->blue);
}

static VALUE
Tone_red(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return INT2FIX(tone->red);
}

static VALUE
Tone_green(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return INT2FIX(tone->green);
}

// attr_writer
static VALUE
Tone_saturation_set(VALUE self, VALUE rbVal)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->saturation = MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_grey_set(VALUE self, VALUE rbVal)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->saturation = 255 - MINMAXU255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_red_set(VALUE self, VALUE rbVal)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->red = MINMAX255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_green_set(VALUE self, VALUE rbVal)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->green = MINMAX255(FIX2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_blue_set(VALUE self, VALUE rbVal)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->blue = MINMAX255(FIX2INT(rbVal));
  return Qnil;
}

static void
Tone_free(Tone* tone)
{
  free(tone);
}

static VALUE
Tone_alloc(VALUE klass)
{
  Tone *tone = ALLOC(Tone);
  tone->red = 0;
  tone->green = 0;
  tone->blue = 0;
  tone->saturation = 255;
  return Data_Wrap_Struct(klass, NULL, Tone_free, tone);
}

static VALUE
Tone_set(int argc, VALUE *argv, VALUE self)
{
  volatile VALUE rbRed, rbGreen, rbBlue, rbSaturation;
  if(argc == 0) {
    rbRed   = INT2FIX(0);
    rbBlue  = INT2FIX(0);
    rbGreen = INT2FIX(0);
    rbSaturation = INT2FIX(255);
  }
  else if(argc == 1) {
    volatile VALUE rbTone;
    rb_scan_args(argc, argv, "10", &rbTone);
    strb_CheckTone(rbTone);

    Tone *tone;
    Data_Get_Struct(rbTone, Tone, tone);

    rbRed   = INT2FIX(tone->red);
    rbGreen = INT2FIX(tone->green);
    rbBlue  = INT2FIX(tone->blue);
    rbSaturation = INT2FIX(tone->saturation);
  }
  else {
    rb_scan_args(argc, argv, "31",
                 &rbRed, &rbGreen, &rbBlue, &rbSaturation);
    if(NIL_P(rbSaturation))
      rbSaturation = INT2FIX(255);
    else
      rbSaturation = INT2FIX((255 - FIX2INT(rbSaturation)));
  }

  const int red   = MINMAX255(FIX2INT(rbRed));
  const int green = MINMAX255(FIX2INT(rbGreen));
  const int blue  = MINMAX255(FIX2INT(rbBlue));
  const int saturation = MINMAXU255(FIX2INT(rbSaturation));

  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->red = red;
  tone->green = green;
  tone->blue = blue;
  tone->saturation = saturation;

  return self;
}

static VALUE
Tone_initialize_copy(VALUE self, VALUE rbOther)
{
  Tone *tone1, *tone2;
  Data_Get_Struct(self, Tone, tone1);
  Data_Get_Struct(rbOther, Tone, tone2);
  tone1->red = tone2->red;
  tone1->green = tone2->green;
  tone1->blue = tone2->blue;
  tone1->saturation = tone2->saturation;

  return Qnil;
}

static VALUE
Tone_equal(VALUE self, VALUE rbOther)
{
  if (self == rbOther) {
    return Qtrue;
  }
  if (!rb_obj_is_kind_of(rbOther, rb_cTone)) {
    return Qfalse;
  }
  Tone *tone1, *tone2;
  Data_Get_Struct(self, Tone, tone1);
  Data_Get_Struct(rbOther, Tone, tone2);
  return CBOOL2RUBY((tone1->red == tone2->red) &&
         (tone1->green == tone2->green) &&
         (tone1->blue == tone2->blue) &&
         (tone1->saturation == tone2->saturation));
}

static VALUE
Tone_hash(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  int64_t hash =((255 - tone->saturation) << 24) +
                (tone->red << 16) +
                (tone->green << 8) +
                (tone->blue);
  return INT2FIX(hash);
}

static VALUE
Tone_to_s(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  char str[256];
  snprintf(str, sizeof(str),
           "#<%s saturation=%d, red=%d, green=%d, blue=%d>",
           rb_obj_classname(self),
           tone->saturation, tone->red, tone->green, tone->blue);
  return rb_str_new2(str);
}

static VALUE
Tone_to_a(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return rb_ary_new4(4, (VALUE[]){INT2FIX(tone->red),
                                  INT2FIX(tone->green),
                                  INT2FIX(tone->blue),
                                  INT2FIX(255 - tone->saturation)});
}

static VALUE
Tone_to_a2(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return rb_ary_new4(4, (VALUE[]){INT2FIX(tone->red),
                                  INT2FIX(tone->green),
                                  INT2FIX(tone->blue),
                                  INT2FIX(255 - tone->saturation)});
}

static VALUE
Tone_dump(VALUE self, VALUE rbDepth)
{
  return rb_funcall(Tone_to_a(self),
    rb_intern("pack"), 1, rb_str_new2("D4\0"));
}

static VALUE
Tone_load(VALUE klass, VALUE rbDStr)
{
  volatile VALUE rbUAry = rb_funcall(
    rbDStr, rb_intern("unpack"), 1, rb_str_new2("D4\0"));

  VALUE rbArgv[4] = {
    rb_ary_entry(rbUAry, 0), rb_ary_entry(rbUAry, 1), // red, green
    rb_ary_entry(rbUAry, 2), rb_ary_entry(rbUAry, 3)  // blue, alpha
  };

  return rb_class_new_instance(4, rbArgv, klass);
}

VALUE strb_InitializeTone(VALUE rb_mStarRuby)
{
  rb_cTone = rb_define_class_under(rb_mStarRuby, "Tone", rb_cObject);
  rb_define_alloc_func(rb_cTone, Tone_alloc);
  rb_define_private_method(rb_cTone, "initialize", Tone_set, -1);
  rb_define_method(rb_cTone, "initialize_copy", Tone_initialize_copy, 1);

  rb_define_singleton_method(rb_cTone, "_load", Tone_load, 1);
  rb_define_method(rb_cTone, "_dump", Tone_dump, 1);

  rb_define_method(rb_cTone, "set", Tone_set, -1);

  rb_define_method(rb_cTone, "red",        Tone_red, 0);
  rb_define_method(rb_cTone, "green",      Tone_green, 0);
  rb_define_method(rb_cTone, "blue",       Tone_blue, 0);
  rb_define_method(rb_cTone, "grey",       Tone_grey, 0);
  rb_define_method(rb_cTone, "saturation", Tone_saturation, 0);

  rb_define_method(rb_cTone, "red=",        Tone_red_set, 1);
  rb_define_method(rb_cTone, "green=",      Tone_green_set, 1);
  rb_define_method(rb_cTone, "blue=",       Tone_blue_set, 1);
  rb_define_method(rb_cTone, "grey=",       Tone_grey_set, 1);
  rb_define_method(rb_cTone, "saturation=", Tone_saturation_set, 1);

  rb_define_method(rb_cTone, "to_s",  Tone_to_s, 0);
  rb_define_method(rb_cTone, "to_a",  Tone_to_a, 0);
  rb_define_method(rb_cTone, "to_a2", Tone_to_a2, 0);
  rb_define_method(rb_cTone, "hash", Tone_hash, 0);

  rb_define_alias(rb_cTone, "gray",  "grey");
  rb_define_alias(rb_cTone, "gray=", "grey=");

  rb_define_method(rb_cTone, "==", Tone_equal, 1);
  rb_define_method(rb_cTone, "eql?", Tone_equal, 1);

  return rb_cTone;
}
