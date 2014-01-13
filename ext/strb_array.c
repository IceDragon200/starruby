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

void
strb_ArrayI_free(ArrayI *array)
{
  free(array->data);
  free(array);
}

void
strb_ArrayF_free(ArrayF *array)
{
  free(array->data);
  free(array);
}

ARRAY_FROM_RUBY_FUNC(ArrayI, 0, NUM2LONG);
ARRAY_FROM_RUBY_FUNC(ArrayF, 0.0, NUM2DBL);

/*
  ArrayI Comparison
 */
bool
strb_ArrayI_comp(ArrayI *a, ArrayI *b)
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

/* Stratos Array */
bool
stratos_Array_free(StratosArray* array)
{
  if(array) {
    if(array->data) {
      free(array->data);
    }
    free(array);
    return true;
  }
  return false;
}

bool
stratos_Array_alloc_data(StratosArray* array)
{
  switch(array->datatype) {
    case DATATYPE_UINT8:
      array->data = ALLOC_N(uint8_t, array->size);
      break;
    case DATATYPE_INT8:
      array->data = ALLOC_N(int8_t, array->size);
      break;
    case DATATYPE_UINT16:
      array->data = ALLOC_N(uint16_t, array->size);
      break;
    case DATATYPE_INT16:
      array->data = ALLOC_N(int16_t, array->size);
      break;
    case DATATYPE_UINT32:
      array->data = ALLOC_N(uint32_t, array->size);
      break;
    case DATATYPE_INT32:
      array->data = ALLOC_N(int32_t, array->size);
      break;
    case DATATYPE_UINT64:
      array->data = ALLOC_N(uint64_t, array->size);
      break;
    case DATATYPE_INT64:
      array->data = ALLOC_N(int64_t, array->size);
      break;
    default:
      return false;
  }
  return true;
}

StratosArray*
stratos_Array_alloc(ArrayDatatype type, uint64_t size)
{
  StratosArray* array = ALLOC(StratosArray);
  return array;
}

bool
stratos_Array_init(StratosArray* array, ArrayDatatype type, uint64_t size)
{
  array->datatype = type;
  array->size = size;
  return stratos_Array_alloc_data(array);
}

uint64_t
stratos_Array_size(StratosArray* array)
{
  return array->size;
}

ArrayDatatype
stratos_Array_datatype(StratosArray* array)
{
  return array->datatype;
}

void*
stratos_Array_data(StratosArray* array)
{
  return array->data;
}
