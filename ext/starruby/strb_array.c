/*
  strb_array.c
  StarRuby
    by IceDragon
    dc 26/02/2013
    dm 26/02/2013
 */
#include "starruby_private.h"
#include "strb_array.prv.h"

#define ARRAY_ALLOCATOR(strct, datatype) \
strct* strb_Alloc ## strct(uint32_t size, datatype default_val) \
{ \
  strct *array = ALLOC(strct); \
  array->size = size; \
  array->data = ALLOC_N(datatype, size); \
  for(uint32_t n = 0; n < size; n++) \
    array->data[n] = default_val; \
  return array; \
}

ARRAY_ALLOCATOR(ArrayI, int32_t);
ARRAY_ALLOCATOR(ArrayF, double);

#undef ARRAY_ALLOCATOR

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
  for(uint32_t i = 0; i < a->size; i++)
  {
    if(a->data[i] != b->data[i])
    {
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
