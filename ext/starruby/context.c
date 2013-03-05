/*
  composite.c
  StarRuby
    by IceDragon
    dc 26/02/2013
    dm 26/02/2013
    vr 1.00
 */

#include "starruby_private.h"
#include "context.h"

static VALUE rb_cContext = Qundef;


VALUE
strb_InitializeContext(VALUE rb_mStarRuby)
{
  rb_cContext = rb_define_class_under(rb_mStarRuby, "Context", rb_cObject);
  return rb_cContext;
}
