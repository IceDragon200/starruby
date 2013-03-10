/*
  strb_array.c
  StarRuby
    by IceDragon
    dc 26/02/2013
    dm 26/02/2013
 */
#include "starruby.prv.h"
#include "strb_array.prv.h"

ARRAY_ALLOCATOR(ArrayI, int32_t);
ARRAY_ALLOCATOR(ArrayF, double);

void strb_ArrayI_free(ArrayI *array)
{
  free(array->data);
  free(array);
}

void strb_ArrayF_free(ArrayF *array)
{
  free(array->data);
  free(array);
}

ARRAY_FROM_RUBY_FUNC(ArrayI, 0, NUM2LONG);
ARRAY_FROM_RUBY_FUNC(ArrayF, 0.0, NUM2DBL);

/*
  ArrayI Comparison
 */
bool strb_ArrayI_comp(ArrayI *a, ArrayI *b)
{
  if(a->size != b->size) return false;
  for(uint32_t i = 0; i < a->size; i++) {
    if(a->data[i] != b->data[i]) {
      return false;
    }
  }
  return true;
}

VALUE strb_ArrayI_to_ruby(ArrayI *array)
{
  VALUE rbArray = rb_ary_new();
  for(uint32_t i = 0; i < array->size; i++)
    rb_ary_push(rbArray, INT2FIX(array->data[i]));
  return rbArray;
}

VALUE strb_ArrayF_to_ruby(ArrayF *array)
{
  VALUE rbArray = rb_ary_new();
  for(uint32_t i = 0; i < array->size; i++)
    rb_ary_push(rbArray, DBL2NUM(array->data[i]));
  return rbArray;
}
