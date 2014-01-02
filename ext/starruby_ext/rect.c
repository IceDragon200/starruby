/*
  StarRuby
    Rect
 */
#include "starruby.prv.h"
#include "rect.h"

volatile VALUE rb_cRect = Qundef;

#define PACK_l4 (rb_str_new2("l4\0"))

/* Rect_get_* */
static VALUE
Rect_get_x(VALUE self)
{
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  return INT2NUM(rect->x);
}

static VALUE
Rect_get_y(VALUE self)
{
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  return INT2NUM(rect->y);
}

static VALUE
Rect_get_width(VALUE self)
{
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  return INT2NUM(rect->width);
}

static VALUE
Rect_get_height(VALUE self)
{
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  return INT2NUM(rect->height);
}

/* Rect_set_* */
static VALUE
Rect_set_x(VALUE self, VALUE rbParam)
{
  rb_check_frozen(self);
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  rect->x = NUM2INT(rbParam);
  return Qnil;
}

static VALUE
Rect_set_y(VALUE self, VALUE rbParam)
{
  rb_check_frozen(self);
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  rect->y = NUM2INT(rbParam);
  return Qnil;
}

static VALUE
Rect_set_width(VALUE self, VALUE rbParam)
{
  rb_check_frozen(self);
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  rect->width = NUM2INT(rbParam);
  return Qnil;
}

static VALUE
Rect_set_height(VALUE self, VALUE rbParam)
{
  rb_check_frozen(self);
  Rect* rect;
  Data_Get_Struct(self, Rect, rect);
  rect->height = NUM2INT(rbParam);
  return Qnil;
}

inline void
strb_RubyToRect(VALUE rbObj, Rect* rect)
{
  switch (TYPE(rbObj)) {
    case T_DATA: {
      strb_AssertObjIsKindOf(rbObj, rb_cRect);
      Rect* src_rect;
      Data_Get_Struct(rbObj, Rect, src_rect);
      *rect = *src_rect;
      break;
    }
    case T_ARRAY: {
      if (hrbCheckArraySize(rbObj, ==, 4)) {
        rect->x      = NUM2INT(rb_ary_entry(rbObj, 0));
        rect->y      = NUM2INT(rb_ary_entry(rbObj, 1));
        rect->width  = NUM2INT(rb_ary_entry(rbObj, 2));
        rect->height = NUM2INT(rb_ary_entry(rbObj, 3));
      } else {
        rb_raise(rb_eArgError, "Expected %s of size 4",
                 rb_class2name(rb_cArray));
      }
      break;
    }
    default: {
      rb_raise(rb_eTypeError, "Cannot convert %s into %s",
               rb_obj_classname(rbObj), rb_class2name(rb_cRect));
    }
  }
}

static VALUE
Rect_set(int argc, VALUE *argv, VALUE self)
{
  rb_check_frozen(self);
  VALUE rbRect, rbX, rbY, rbWidth, rbHeight;
  Rect *rect;

  Data_Get_Struct(self, Rect, rect);

  if(argc == 0)
  {
    rect->x      = 0;
    rect->y      = 0;
    rect->width  = 0;
    rect->height = 0;
  }
  else if(argc == 1)
  {
    rb_scan_args(argc, argv, "10", &rbRect);
    strb_RubyToRect(rbRect, rect);
  }
  else
  {
    rb_scan_args(argc, argv, "40", &rbX, &rbY, &rbWidth, &rbHeight);
    rect->x = NUM2INT(rbX);
    rect->y = NUM2INT(rbY);
    rect->width = NUM2INT(rbWidth);
    rect->height = NUM2INT(rbHeight);
  }

  return self;
}

static VALUE
Rect_init_copy(VALUE self, VALUE rbOrgRect)
{
  volatile Rect *org_rect, *rect;
  Data_Get_Struct(self, Rect, rect);
  Data_Get_Struct(rbOrgRect, Rect, org_rect);

  rect->x = org_rect->x;
  rect->y = org_rect->y;
  rect->width = org_rect->width;
  rect->height = org_rect->height;

  return self;
}

static VALUE
Rect_to_a(VALUE self)
{
  Rect *rect;
  volatile VALUE ary;

  Data_Get_Struct(self, Rect, rect);

  ary = rb_ary_new();
  rb_ary_push(ary, INT2NUM(rect->x));
  rb_ary_push(ary, INT2NUM(rect->y));
  rb_ary_push(ary, INT2NUM(rect->width));
  rb_ary_push(ary, INT2NUM(rect->height));

  return ary;
}

static VALUE
Rect_empty(VALUE self)
{
  rb_check_frozen(self);
  Rect *rect;
  Data_Get_Struct(self, Rect, rect);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  return self;
}

static VALUE
Rect_is_empty(VALUE self)
{
  Rect *rect;
  Data_Get_Struct(self, Rect, rect);
  return CBOOL2RVAL(rect->width == 0 || rect->height == 0);
}

// Marshalling
static VALUE
Rect_dump(VALUE self, VALUE rbDepth)
{
  return rb_funcall(Rect_to_a(self), ID_pack, 1, PACK_l4);
}

static VALUE
Rect_load(VALUE klass, VALUE rbDStr)
{
  volatile VALUE rbUAry = rb_funcall(rbDStr, ID_unpack, 1, PACK_l4);

  VALUE rbArgv[4] = {
    rb_ary_entry(rbUAry, 0), rb_ary_entry(rbUAry, 1), // x, y
    rb_ary_entry(rbUAry, 2), rb_ary_entry(rbUAry, 3)  // width, height
  };

  return rb_class_new_instance(4, rbArgv, klass);
}

// Alloc / DeAlloc
static void
Rect_free(Rect *rect)
{
  free(rect);
}

static VALUE
Rect_alloc(VALUE klass)
{
  Rect* rect = ALLOC(Rect);
  rect->x      = 0;
  rect->y      = 0;
  rect->width  = 0;
  rect->height = 0;
  return Data_Wrap_Struct(klass, 0, Rect_free, rect);
}

static VALUE
Rect_is_equal(VALUE self, VALUE rbOther)
{
  if(self == rbOther) {
    return Qtrue;
  }
  if(!rb_obj_is_kind_of(rbOther, rb_cRect)) {
    return Qfalse;
  }

  Rect *rect1, *rect2;
  Data_Get_Struct(self, Rect, rect1);
  Data_Get_Struct(rbOther, Rect, rect2);

  return CBOOL2RVAL(rect1->x == rect2->x &&
                    rect1->y == rect2->y &&
                    rect1->width == rect2->width &&
                    rect1->height == rect2->height);
}

static VALUE
Rect_to_s(VALUE self)
{
  Rect *rect;
  Data_Get_Struct(self, Rect, rect);
  char str[256];
  snprintf(str, sizeof(str),
           "%d, %d, %d, %d", rect->x, rect->y, rect->width, rect->height);
  return rb_str_new2(str);
}

static VALUE
Rect_inspect(VALUE self)
{
  Rect *rect;
  Data_Get_Struct(self, Rect, rect);
  char str[256];
  snprintf(str, sizeof(str),
           "#<%s x=%d, y=%d, width=%d, height=%d>",
           rb_obj_classname(self), rect->x, rect->y, rect->width, rect->height);
  return rb_str_new2(str);
}

static VALUE
Rect_s_cast(VALUE klass, VALUE rbObj)
{
  Rect* rect;
  VALUE rbRect;
  rbRect = rb_class_new_instance(0, (VALUE[]){}, klass);
  Data_Get_Struct(rbRect, Rect, rect);
  strb_RubyToRect(rbObj, rect);
  return rbRect;
}

VALUE strb_InitializeRect(VALUE rb_mSub)
{
  rb_cRect = rb_define_class_under(rb_mSub, "Rect", rb_cObject);
  rb_define_alloc_func(rb_cRect, Rect_alloc);

  rb_define_singleton_method(rb_cRect, "cast", Rect_s_cast, 1);
  rb_define_singleton_method(rb_cRect, "[]", Rect_s_cast, 1);

  rb_define_private_method(rb_cRect, "initialize", Rect_set, -1);
  rb_define_private_method(rb_cRect, "initialize_copy", Rect_init_copy, 1);
  rb_define_method(rb_cRect, "set", Rect_set, -1);

  rb_define_method(rb_cRect, "x", Rect_get_x, 0);
  rb_define_method(rb_cRect, "y", Rect_get_y, 0);
  rb_define_method(rb_cRect, "width", Rect_get_width, 0);
  rb_define_method(rb_cRect, "height", Rect_get_height, 0);

  rb_define_method(rb_cRect, "x=", Rect_set_x, 1);
  rb_define_method(rb_cRect, "y=", Rect_set_y, 1);
  rb_define_method(rb_cRect, "width=", Rect_set_width, 1);
  rb_define_method(rb_cRect, "height=", Rect_set_height, 1);

  rb_define_method(rb_cRect, "empty", Rect_empty, 0);
  rb_define_method(rb_cRect, "empty?", Rect_is_empty, 0);

  rb_define_method(rb_cRect, "to_a", Rect_to_a, 0);
  rb_define_method(rb_cRect, "to_s", Rect_to_s, 0);
  rb_define_method(rb_cRect, "inspect", Rect_inspect, 0);

  rb_define_method(rb_cRect, "_dump", Rect_dump, 1);
  rb_define_singleton_method(rb_cRect, "_load", Rect_load, 1);

  rb_define_method(rb_cRect, "==", Rect_is_equal, 1);
  rb_define_method(rb_cRect, "eql?", Rect_is_equal, 1);

  return rb_cRect;
}
