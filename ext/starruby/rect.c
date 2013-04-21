/*
  StarRuby
    Rect
 */
#include "starruby.prv.h"
#include "rect.h"

volatile VALUE rb_cRect = Qundef;

#define PACK_l4 (rb_str_new2("l4\0"))

STRUCT_ATTR_ACCESSOR(Rect, Rect, x, INT2NUM, NUM2INT);
STRUCT_ATTR_ACCESSOR(Rect, Rect, y, INT2NUM, NUM2INT);
STRUCT_ATTR_ACCESSOR(Rect, Rect, width, INT2NUM, NUM2INT);
STRUCT_ATTR_ACCESSOR(Rect, Rect, height, INT2NUM, NUM2INT);

static VALUE
Rect_set(int argc, VALUE *argv, VALUE self)
{
  const VALUE zero = INT2NUM(0);
  volatile VALUE rbRect, rbX, rbY, rbWidth, rbHeight;
  volatile Rect *rect, *src_rect;

  Data_Get_Struct(self, Rect, rect);

  if(argc == 0)
  {
    rbX      = zero;
    rbY      = zero;
    rbWidth  = zero;
    rbHeight = zero;
  }
  else if(argc == 1)
  {
    rb_scan_args(argc, argv, "10", &rbRect);
    strb_CheckObjIsKindOf(rbRect, rb_cRect);
    Data_Get_Struct(rbRect, Rect, src_rect);

    rbX = INT2NUM(src_rect->x);
    rbY = INT2NUM(src_rect->y);
    rbWidth = INT2NUM(src_rect->width);
    rbHeight = INT2NUM(src_rect->height);
  }
  else
  {
    rb_scan_args(argc, argv, "40", &rbX, &rbY, &rbWidth, &rbHeight);
  }
  rect->x = NUM2INT(rbX);
  rect->y = NUM2INT(rbY);
  rect->width = NUM2INT(rbWidth);
  rect->height = NUM2INT(rbHeight);

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
  return CBOOL2RUBY(rect->width == 0 || rect->height == 0);
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

  return CBOOL2RUBY(rect1->x == rect2->x &&
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
           "#<%s x=%d, y=%d, width=%d, height=%d>",
           rb_obj_classname(self), rect->x, rect->y, rect->width, rect->height);
  return rb_str_new2(str);
}

VALUE strb_InitializeRect(VALUE rb_mStarRuby)
{
  rb_cRect = rb_define_class_under(rb_mStarRuby, "Rect", rb_cObject);
  rb_define_alloc_func(rb_cRect, Rect_alloc);

  rb_define_method(rb_cRect, "set", Rect_set, -1);
  rb_define_alias(rb_cRect, "initialize", "set");

  rb_define_method(rb_cRect, "initialize_copy", Rect_init_copy, 1);

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

  rb_define_method(rb_cRect, "_dump", Rect_dump, 1);
  rb_define_singleton_method(rb_cRect, "_load", Rect_load, 1);

  rb_define_method(rb_cRect, "==", Rect_is_equal, 1);
  rb_define_method(rb_cRect, "eql?", Rect_is_equal, 1);

  return rb_cRect;
}
