/*
  StarRuby
    matrix.c
    dc 26/02/2013
    dm 26/02/2013
 */

#include "starruby_private.h"
#include "matrix.prv.h"

static ID sym_size = Qundef;
static VALUE rb_cMatrixI = Qundef;

void strb_MatrixI_free(MatrixI *matrix)
{
  strb_ArrayI_free(matrix->dimensions);
  strb_ArrayI_free(matrix->offsets);
  strb_ArrayI_free(matrix->data);
  free(matrix);
}

static void MatrixI_free(MatrixI *matrix)
{
  strb_MatrixI_free(matrix);
}

STRUCT_CHECK_TYPE_FUNC(MatrixI, MatrixI);

static VALUE
MatrixI_alloc(VALUE klass)
{
  MatrixI *matrix = ALLOC(MatrixI);
  return Data_Wrap_Struct(klass, NULL, MatrixI_free, matrix);
}

MATRIX_ALLOCATOR_FUNC(I, int32_t);

MATRIX_UNARY_FUNC(MatrixI_negate_bang, MatrixI, int32_t, negate, -);
MATRIX_UNARY_FUNC(MatrixI_affirm_bang, MatrixI, int32_t, affirm, +);

static VALUE
MatrixI_negate(VALUE self)
{
  VALUE newObj = rb_obj_dup(self);
  MatrixI_negate_bang(newObj);
  return newObj;
}

static VALUE
MatrixI_affirm(VALUE self)
{
  VALUE newObj = rb_obj_dup(self);
  MatrixI_affirm_bang(newObj);
  return newObj;
}

MATRIX_ITERATE_DO_FUNC(MatrixI_inc_bang, MatrixI, int32_t, {data[i] += 1;});
MATRIX_ITERATE_DO_FUNC(MatrixI_dec_bang, MatrixI, int32_t, {data[i] -= 1;});

MATRIX_CALC_OFFSETS_FUNC(Matrix_calc_offsets, IMatrix);

static VALUE
MatrixI_default_bang(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  const uint32_t size = matrix->data->size;
  volatile int32_t *data = matrix->data->data;
  int32_t default_val = FIX2INT(rb_iv_get(self, "default"));
  for(uint32_t i = 0; i < size; i++)
  {
    data[i] = default_val;
  }
  return self;
}

static VALUE
MatrixI_reset_bang(VALUE self)
{
  MatrixI_default_bang(self);
  Matrix_calc_offsets(self);
  return self;
}

static int32_t
MatrixI_dims_to_index(MatrixI *matrix, ArrayI *dims)
{
  volatile int32_t n = 0;
  const uint32_t dim_count = dims->size;
  ArrayI *dimensions = matrix->dimensions;
  ArrayI *offsets = matrix->offsets;

  if (dimensions->size != dim_count) return -1;

  for(uint i = 0; i < offsets->size; i++)
  {
    if(dims->data[i] < 0 || !(dims->data[i] < dimensions->data[i])) return -1;
    n += dims->data[i] * offsets->data[i];
  }

  return n;
}

static int32_t
MatrixI_entry_dims_to_index(MatrixI *matrix, int argc, VALUE *argv)
{
  const uint32_t dim_count = matrix->dimensions->size;

  if(dim_count != (uint)argc)
    rb_raise(rb_eArgError, "expected %d args but recived %d", dim_count, argc);

  return MatrixI_dims_to_index(
    matrix, strb_ArrayI_from_ruby(rb_ary_new4(argc, argv)));
}

static int32_t
MatrixI_entry_dims_to_index2(MatrixI *matrix, ArrayI *argv)
{
  const uint32_t dim_count = matrix->dimensions->size;

  if(dim_count != argv->size)
  {
    rb_raise(rb_eArgError, "expected %d args but recived %d",
      dim_count, argv->size);
  }

  return MatrixI_dims_to_index(matrix, argv);
}

static VALUE
MatrixI_entry_get(int argc, VALUE *argv, VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  const int32_t index = MatrixI_entry_dims_to_index(matrix, argc, argv);

  volatile VALUE defaultv = rb_iv_get(self, "default");
  volatile VALUE val;
  if(index < 0)
    val = defaultv;
  else
    val = INT2FIX(matrix->data->data[index]);

  return val;
}

static VALUE
MatrixI_entry_set(int argc, VALUE *argv, VALUE self)
{
  rb_check_frozen(self);
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  const int32_t index = MatrixI_entry_dims_to_index(matrix, argc-1, argv);
  volatile VALUE value = argv[argc-1];

  if(!(index < 0))
    matrix->data->data[index] = FIX2INT(value);

  return self;
}

void
strb_ModifyInMatrixI(MatrixI *matrix, DimensionNet *dimnet)
{
  ArrayI *dims = matrix->dimensions;
  for(uint32_t i = 0; i < dims->size; i++)
  {
    int32_t bottom = 0;
    int32_t top = dims->data[i];
    if(dimnet->dstart->data[i] < bottom)
      dimnet->dstart->data[i] = bottom;

    if(dimnet->dend->data[i] > top)
      dimnet->dend->data[i] = top;
  }
  strb_DimensionNet_calc_sizes(dimnet);
}

MATRIX_MATH_FUNCS(MatrixI, MatrixI, ArrayI, int32_t, FIX2INT);

static VALUE
MatrixI_initialize(VALUE self, VALUE dimensions, VALUE defaul_val)
{
  volatile MatrixI *matrix;
  const uint dim_size = NUM2INT(rb_funcall(dimensions, rb_intern("size"), 0));

  Data_Get_Struct(self, MatrixI, matrix);
  matrix->dimensions = strb_AllocArrayI(dim_size, 0);
  matrix->offsets = strb_AllocArrayI(dim_size, 0);

  uint32_t datasize = 1;
  for(uint32_t i = 0; i < dim_size; i++)
  {
    uint32_t size = NUM2INT(rb_ary_entry(dimensions, i));
    matrix->dimensions->data[i] = size;
    datasize *= size;
  }
  rb_iv_set(self, "default", rb_funcall(defaul_val, rb_intern("to_i"), 0));

  matrix->data = strb_AllocArrayI(datasize, 0);

  MatrixI_reset_bang(self);
  return self;
}

static VALUE
MatrixI_initialize_copy(VALUE self, VALUE rbOrgMatrix)
{
  MatrixI *trg_matrix, *src_matrix;
  Data_Get_Struct(self, MatrixI, trg_matrix);
  Data_Get_Struct(rbOrgMatrix, MatrixI, src_matrix);

  trg_matrix->dimensions = strb_AllocArrayI(src_matrix->dimensions->size, 0);
  trg_matrix->offsets = strb_AllocArrayI(src_matrix->offsets->size, 0);
  trg_matrix->data = strb_AllocArrayI(src_matrix->data->size, 0);

  strb_ArrayI_copy(trg_matrix->dimensions, src_matrix->dimensions);
  strb_ArrayI_copy(trg_matrix->offsets, src_matrix->offsets);
  strb_ArrayI_copy(trg_matrix->data, src_matrix->data);

  return self;
}

static VALUE
MatrixI_datasize(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  ArrayI *data = matrix->data;
  return INT2FIX(data->size);
}

static VALUE
MatrixI_dimsize(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  ArrayI *dimensions = matrix->dimensions;
  return INT2FIX(dimensions->size);
}

static VALUE
MatrixI_dimensions_ary(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  ArrayI *dimensions = matrix->dimensions;
  VALUE ary = rb_ary_new();
  for(uint32_t i = 0; i < dimensions->size; i++)
  {
    rb_ary_push(ary, INT2FIX(dimensions->data[i]));
  }
  return ary;
}

static VALUE
MatrixI_offsets_ary(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  ArrayI *offsets = matrix->offsets;
  VALUE ary = rb_ary_new();
  for(uint32_t i = 0; i < offsets->size; i++)
  {
    rb_ary_push(ary, INT2FIX(offsets->data[i]));
  }
  return ary;
}

static VALUE
MatrixI_dimnet_ary(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  ArrayI *dimensions = matrix->dimensions;
  VALUE ary = rb_ary_new();
  VALUE ary2 = rb_ary_new();
  VALUE result_ary = rb_ary_new();
  for(uint32_t i = 0; i < dimensions->size; i++)
  {
    rb_ary_push(ary, INT2FIX(0));
    rb_ary_push(ary2, INT2FIX(dimensions->data[i]));
  }
  rb_ary_push(result_ary, ary);
  rb_ary_push(result_ary, ary2);
  return result_ary;
}

static VALUE
MatrixI_data_ary(VALUE self)
{
  MatrixI *matrix;
  Data_Get_Struct(self, MatrixI, matrix);
  ArrayI *data = matrix->data;
  VALUE ary = rb_ary_new();
  for(uint32_t i = 0; i < data->size; i++)
  {
    rb_ary_push(ary, INT2FIX(data->data[i]));
  }
  return ary;
}

VALUE strb_InitializeMatrix(VALUE rb_mStarRuby)
{
  sym_size = rb_intern("size");
  rb_cMatrixI = rb_define_class_under(rb_mStarRuby, "MatrixI", rb_cObject);

  rb_define_alloc_func(rb_cMatrixI, MatrixI_alloc);

  rb_define_method(rb_cMatrixI, "initialize", MatrixI_initialize, 2);
  rb_define_method(rb_cMatrixI, "initialize_copy", MatrixI_initialize_copy, 1);
  rb_define_method(rb_cMatrixI, "negate!", MatrixI_negate_bang, 0);
  rb_define_method(rb_cMatrixI, "affirm!", MatrixI_affirm_bang, 0);
  rb_define_method(rb_cMatrixI, "negate", MatrixI_negate, 0);
  rb_define_method(rb_cMatrixI, "affirm", MatrixI_affirm, 0);
  rb_define_alias(rb_cMatrixI, "-@", "negate");
  rb_define_alias(rb_cMatrixI, "+@", "affirm");

  rb_define_method(rb_cMatrixI, "dimsize", MatrixI_dimsize, 0);
  rb_define_method(rb_cMatrixI, "datasize", MatrixI_datasize, 0);
  rb_define_alias(rb_cMatrixI, "size", "datasize");

  rb_define_method(rb_cMatrixI, "dimensions", MatrixI_dimensions_ary, 0);
  rb_define_method(rb_cMatrixI, "offsets", MatrixI_offsets_ary, 0);
  rb_define_method(rb_cMatrixI, "data", MatrixI_data_ary, 0);
  rb_define_method(rb_cMatrixI, "dimnet", MatrixI_dimnet_ary, 0);
  rb_define_alias(rb_cMatrixI, "to_a", "data");

  rb_define_method(rb_cMatrixI, "[]", MatrixI_entry_get, -1);
  rb_define_method(rb_cMatrixI, "[]=", MatrixI_entry_set, -1);
  rb_define_method(rb_cMatrixI, "inc!", MatrixI_inc_bang, 0);
  rb_define_method(rb_cMatrixI, "dec!", MatrixI_dec_bang, 0);
  rb_define_method(rb_cMatrixI, "reset!", MatrixI_reset_bang, 0);

  rb_define_method(rb_cMatrixI, "add!", MatrixI_add_bang, 1);
  rb_define_method(rb_cMatrixI, "sub!", MatrixI_sub_bang, 1);
  rb_define_method(rb_cMatrixI, "mul!", MatrixI_mul_bang, 1);
  rb_define_method(rb_cMatrixI, "div!", MatrixI_div_bang, 1);
  rb_define_method(rb_cMatrixI, "replace!", MatrixI_replace_bang, 1);

  rb_define_method(rb_cMatrixI, "add", MatrixI_add, 1);
  rb_define_method(rb_cMatrixI, "sub", MatrixI_sub, 1);
  rb_define_method(rb_cMatrixI, "mul", MatrixI_mul, 1);
  rb_define_method(rb_cMatrixI, "div", MatrixI_div, 1);
  rb_define_method(rb_cMatrixI, "replace", MatrixI_replace, 1);

  rb_define_alias(rb_cMatrixI, "+", "add");
  rb_define_alias(rb_cMatrixI, "-", "sub");
  rb_define_alias(rb_cMatrixI, "*", "mul");
  rb_define_alias(rb_cMatrixI, "/", "div");

  rb_define_method(rb_cMatrixI, "add_at!", MatrixI_add_at_bang, 4);
  rb_define_method(rb_cMatrixI, "sub_at!", MatrixI_sub_at_bang, 4);
  rb_define_method(rb_cMatrixI, "mul_at!", MatrixI_mul_at_bang, 4);
  rb_define_method(rb_cMatrixI, "div_at!", MatrixI_div_at_bang, 4);
  rb_define_method(rb_cMatrixI, "replace_at!", MatrixI_replace_at_bang, 4);

  rb_define_method(rb_cMatrixI, "add_at", MatrixI_add_at, 4);
  rb_define_method(rb_cMatrixI, "sub_at", MatrixI_sub_at, 4);
  rb_define_method(rb_cMatrixI, "mul_at", MatrixI_mul_at, 4);
  rb_define_method(rb_cMatrixI, "div_at", MatrixI_div_at, 4);
  rb_define_method(rb_cMatrixI, "replace_at", MatrixI_replace_at, 4);

  return Qtrue;
}
