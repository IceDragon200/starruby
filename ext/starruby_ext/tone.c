#include "tone.h"
#include "starruby.prv.h"

volatile VALUE rb_cTone = Qundef;

// attr_reader
static VALUE
Tone_saturation(VALUE self)
{
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  return INT2FIX(tone->saturation);
}

static VALUE
Tone_gray(VALUE self)
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
  rb_check_frozen(self);
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->saturation = MINMAXU255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_gray_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->saturation = 255 - MINMAXU255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_red_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->red = MINMAX255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_green_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->green = MINMAX255(NUM2INT(rbVal));
  return Qnil;
}

static VALUE
Tone_blue_set(VALUE self, VALUE rbVal)
{
  rb_check_frozen(self);
  Tone *tone;
  Data_Get_Struct(self, Tone, tone);
  tone->blue = MINMAX255(NUM2INT(rbVal));
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
  rb_check_frozen(self);
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
    strb_AssertObjIsKindOf(rbTone, rb_cTone);

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
      rbSaturation = INT2FIX((255 - NUM2INT(rbSaturation)));
  }

  const int red   = MINMAX255(NUM2INT(rbRed));
  const int green = MINMAX255(NUM2INT(rbGreen));
  const int blue  = MINMAX255(NUM2INT(rbBlue));
  const int saturation = MINMAXU255(NUM2INT(rbSaturation));

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
Tone_is_equal(VALUE self, VALUE rbOther)
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
  return CBOOL2RVAL((tone1->red == tone2->red) &&
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
           "#<%d, %d, %d, %d>",
           tone->red, tone->green, tone->blue, tone->saturation);
  return rb_str_new2(str);
}

static VALUE
Tone_inspect(VALUE self)
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
  return rb_funcall(Tone_to_a(self), ID_pack, 1, rb_str_new2("D4"));
}

static VALUE
Tone_load(VALUE klass, VALUE rbDStr)
{
  volatile VALUE rbUAry = rb_funcall(rbDStr, ID_unpack, 1, rb_str_new2("D4"));

  VALUE rbArgv[4] = {
    rb_ary_entry(rbUAry, 0), rb_ary_entry(rbUAry, 1), // red, green
    rb_ary_entry(rbUAry, 2), rb_ary_entry(rbUAry, 3)  // blue, saturation
  };

  return rb_class_new_instance(4, rbArgv, klass);
}

static inline void
strb_ToneSet4b(Tone* dst_tone,
               uint8_t saturation, uint8_t red, uint8_t green, uint8_t blue)
{
  dst_tone->saturation = saturation;
  dst_tone->red   = red;
  dst_tone->green = green;
  dst_tone->blue  = blue;
}

static inline void
strb_CopyToneTo(Tone* src_tone, Tone* dst_tone)
{
  dst_tone->saturation = src_tone->saturation;
  dst_tone->red   = src_tone->red;
  dst_tone->green = src_tone->green;
  dst_tone->blue  = src_tone->blue;
}

void
strb_RubyToTone(VALUE rbObj, Tone* tone)
{
  switch (TYPE(rbObj)) {
    case T_FIXNUM:
    case T_FLOAT: {
      tone->red   = MINMAX255(NUM2INT(rbObj));
      tone->green = tone->red;
      tone->blue  = tone->red;
      tone->saturation = 255;
      break;
    }
    case T_DATA: {
      Tone* src_tone;
      strb_AssertObjIsKindOf(rbObj, rb_cTone);
      Data_Get_Struct(rbObj, Tone, src_tone);
      *tone = *src_tone;
      break;
    }
    case T_ARRAY: {
      if (hrbCheckArraySize(rbObj, ==, 3)) {
        tone->red   = MINMAX255(NUM2INT(rb_ary_entry(rbObj, 0)));
        tone->green = MINMAX255(NUM2INT(rb_ary_entry(rbObj, 1)));
        tone->blue  = MINMAX255(NUM2INT(rb_ary_entry(rbObj, 2)));
        tone->saturation = 255;
      } else if (hrbCheckArraySize(rbObj, ==, 4)) {
        tone->red        = MINMAX255(NUM2INT(rb_ary_entry(rbObj, 0)));
        tone->green      = MINMAX255(NUM2INT(rb_ary_entry(rbObj, 1)));
        tone->blue       = MINMAX255(NUM2INT(rb_ary_entry(rbObj, 2)));
        tone->saturation = MINMAXU255(NUM2INT(rb_ary_entry(rbObj, 3))) ^ 255;
      } else {
        rb_raise(rb_eArgError, "Expected %s of size 3 or 4",
                 rb_class2name(rb_cArray));
      }
      break;
    }
    default: {
      rb_raise(rb_eTypeError, "Can't convert %s into %s",
               rb_obj_classname(rbObj), rb_class2name(rb_cTone));
    }
  }
}

static VALUE
Tone_s_cast(VALUE klass, VALUE rbObj)
{
  Tone* tone;
  VALUE rbTone;
  rbTone = rb_class_new_instance(0, (VALUE[]){}, klass);
  Data_Get_Struct(rbTone, Tone, tone);
  strb_RubyToTone(rbObj, tone);
  return rbTone;
}

VALUE strb_InitializeTone(VALUE rb_mStarRuby)
{
  rb_cTone = rb_define_class_under(rb_mStarRuby, "Tone", rb_cObject);
  rb_define_alloc_func(rb_cTone, Tone_alloc);

  rb_define_singleton_method(rb_cTone, "_load", Tone_load, 1);
  rb_define_method(rb_cTone, "_dump", Tone_dump, 1);

  rb_define_private_method(rb_cTone, "initialize", Tone_set, -1);
  rb_define_method(rb_cTone, "initialize_copy", Tone_initialize_copy, 1);

  rb_define_singleton_method(rb_cTone, "cast", Tone_s_cast, 1);

  rb_define_method(rb_cTone, "set", Tone_set, -1);

  rb_define_method(rb_cTone, "red",        Tone_red, 0);
  rb_define_method(rb_cTone, "green",      Tone_green, 0);
  rb_define_method(rb_cTone, "blue",       Tone_blue, 0);
  rb_define_method(rb_cTone, "gray",       Tone_gray, 0);
  rb_define_method(rb_cTone, "saturation", Tone_saturation, 0);

  rb_define_method(rb_cTone, "red=",        Tone_red_set, 1);
  rb_define_method(rb_cTone, "green=",      Tone_green_set, 1);
  rb_define_method(rb_cTone, "blue=",       Tone_blue_set, 1);
  rb_define_method(rb_cTone, "gray=",       Tone_gray_set, 1);
  rb_define_method(rb_cTone, "saturation=", Tone_saturation_set, 1);

  rb_define_method(rb_cTone, "to_s",  Tone_to_s, 0);
  rb_define_method(rb_cTone, "to_a",  Tone_to_a, 0);
  rb_define_method(rb_cTone, "to_a2", Tone_to_a2, 0);

  rb_define_method(rb_cTone, "hash",  Tone_hash, 0);

  rb_define_method(rb_cTone, "==", Tone_is_equal, 1);
  rb_define_alias(rb_cTone, "==", "eql?");

  rb_define_method(rb_cTone, "inspect",  Tone_inspect, 0);

  return rb_cTone;
}
