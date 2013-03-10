/*
  matrix.prv.h

 */
#ifndef STARRUBY_MATRIX_PRV_H
  #define STARRUBY_MATRIX_PRV_H

  #include "matrix.h"
  #include "dimnet.h"

  static int32_t MatrixI_entry_dims_to_index2(MatrixI *matrix, ArrayI *argv);

  #define MATRIX_ITERATE_DO_FUNC(name, strct, datatype, process) \
    static VALUE                                                 \
    name(VALUE self)                                             \
    {                                                            \
      strct *matrix;                                             \
      Data_Get_Struct(self, strct, matrix);                      \
      const uint32_t size = matrix->data->size;                  \
      volatile datatype *data = matrix->data->data;              \
      for(uint32_t i = 0; i < size; i++)                         \
      {                                                          \
        process;                                                 \
      }                                                          \
      return self;                                               \
    }

  #define MATRIX_UNARY_FUNC(name, strct, datatype, word, sym) \
    MATRIX_ITERATE_DO_FUNC(name, strct, datatype, \
      { \
        data[i] = sym(data[i]); \
      } \
    )

  #define MATRIX_ALLOCATOR_FUNC(type, datatype) \
  Matrix ## type* \
  strb_AllocMatrix ## type(int num_dimensions, uint32_t *sizes, datatype default_value) \
  { \
    volatile int size = 1; \
    Matrix ## type *matrix = ALLOC(Matrix ## type); \
    matrix->dimensions = strb_AllocArrayI(num_dimensions, 0); \
    for(int n = 0; n < num_dimensions; n++) \
    { \
      matrix->dimensions->data[n] = sizes[n]; \
      size *= sizes[n]; \
    } \
    matrix->offsets = strb_AllocArrayI(num_dimensions, 0); \
    matrix->data = strb_AllocArray ## type(size, default_value); \
    return matrix; \
  }

  #define MATRIX_CALC_OFFSETS_FUNC(name, strct) \
    static void \
    name(VALUE self) \
    { \
      strct *matrix; \
      Data_Get_Struct(self, strct, matrix); \
      ArrayI *dims = matrix->dimensions; \
      ArrayI *offsets = matrix->offsets; \
      for(uint32_t i = 0; i < dims->size; i++) \
      { \
        offsets->data[i] = 1; \
        for(uint32_t x = 0; x < i; x++) \
        { \
          offsets->data[i] *= dims->data[x]; \
        } \
      } \
    }

#define MATRIX_MATH_RECURSION_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect) \
  static void namespace ## _RangeRecursion_ ## word ## _matrix_bang( \
    uint32_t index, uint32_t stack_size, \
    ArrayI **range_stack, ArrayI *args, \
    strct *src_matrix, strct *trg_matrix, \
    DimensionNet *dimnet, ArrayI *trg_vec) \
  { \
    const ArrayI *rng = range_stack[index]; \
    const int32_t irng_start = rng->data[0]; \
    const int32_t irng_end   = rng->data[1]; \
    if(index < (stack_size - 1)) \
    { \
      for(int32_t i = irng_start; i < irng_end; i++) \
      { \
        strb_ArrayI_entry_set(args, index, i); \
        namespace ## _RangeRecursion_ ## word ## _matrix_bang(index + 1, stack_size, range_stack, args, src_matrix, trg_matrix, dimnet, trg_vec); \
      } \
    } \
    else \
    { \
      ArrayI *nargs = strb_AllocArrayI(stack_size, 0); \
      for(int32_t i = irng_start; i < irng_end; i++) \
      { \
        strb_ArrayI_entry_set(args, index, i); \
        for(uint32_t x = 0; x < stack_size; x++) \
        { \
          nargs->data[x] = trg_vec->data[x] + args->data[x] - dimnet->dstart->data[x]; \
        } \
        const int32_t src_index = namespace ## _entry_dims_to_index2(src_matrix, args); \
        const int32_t trg_index = namespace ## _entry_dims_to_index2(trg_matrix, nargs); \
        if(!(src_index < 0) && !(trg_index < 0)) \
        { \
          datatype num = src_matrix->data->data[src_index]; \
          zero_protect; \
          trg_matrix->data->data[trg_index]  sym ## = num;  \
        } \
      } \
      strb_ArrayI_free(nargs); \
    } \
  } \
  static void namespace ## _RangeRecursion_ ## word ## _numeric_bang( \
    uint32_t index, uint32_t stack_size, \
    ArrayI **range_stack, ArrayI *args, \
    datatype src_num, strct *trg_matrix, \
    DimensionNet *dimnet, ArrayI *trg_vec) \
  { \
    datatype num = src_num; \
    zero_protect; \
    const ArrayI *rng = range_stack[index]; \
    const int32_t irng_start = rng->data[0]; \
    const int32_t irng_end   = rng->data[1]; \
    if(index < (stack_size - 1)) \
    { \
      for(int32_t i = irng_start; i < irng_end; i++) \
      { \
        strb_ArrayI_entry_set(args, index, i); \
        namespace ## _RangeRecursion_ ## word ## _numeric_bang( \
            index + 1, stack_size, range_stack, args, \
            num, trg_matrix, dimnet, trg_vec); \
      } \
    } \
    else \
    { \
      ArrayI *nargs = strb_AllocArrayI(stack_size, 0); \
      for(int32_t i = irng_start; i < irng_end; i++) \
      { \
        strb_ArrayI_entry_set(args, index, i); \
        for(uint32_t x = 0; x < stack_size; x++) \
        { \
          nargs->data[x] = trg_vec->data[x] + args->data[x] - dimnet->dstart->data[x]; \
        } \
        const int32_t trg_index = namespace ## _entry_dims_to_index2(trg_matrix, nargs); \
        if(!(trg_index < 0)) \
        { \
          trg_matrix->data->data[trg_index] sym ## = num; \
        } \
      } \
      strb_ArrayI_free(nargs); \
    } \
  }

#define MATRIX_MATH_MATRIX_BANG_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect) \
  static VALUE namespace ## _ ## word ## _matrix_bang(VALUE self, VALUE rbMatrix) \
  { \
    strb_CheckMatrixI(self); \
    strct *trg_matrix, *src_matrix; \
    Data_Get_Struct(self, strct, trg_matrix); \
    Data_Get_Struct(rbMatrix, strct, src_matrix); \
    \
    if(!strb_ArrayI_comp(trg_matrix->dimensions, src_matrix->dimensions)) \
    { \
      rb_raise(rb_eArgError, \
        "Source and Target Matrix must be of the same dimensions"); \
    } \
    \
    arytype *src_data = src_matrix->data; \
    arytype *trg_data = trg_matrix->data; \
    const uint32_t size = src_data->size; \
    for(uint32_t i = 0; i < size; i++) \
    { \
      datatype num = src_data->data[i]; \
      zero_protect; \
      trg_data->data[i] sym ## = num; \
    } \
    return self; \
  }

#define MATRIX_MATH_BASIC_BANG_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect) \
  static VALUE namespace ## _ ## word ## _numeric_bang(VALUE self, VALUE rbObj) \
  { \
    MatrixI *matrix; \
    Data_Get_Struct(self, MatrixI, matrix); \
    volatile datatype num = rbconv(rbObj); \
    volatile arytype* mdata = matrix->data; \
    const uint32_t size = mdata->size; \
    zero_protect; \
    for(uint32_t i = 0; i < size; i++) \
    { \
      mdata->data[i] sym ## = num; \
    } \
    return self; \
  } \
  static VALUE namespace ## _ ## word ## _bang(VALUE self, VALUE rbObj) \
  { \
    if(NUMERIC_P(rbObj)) \
    { \
      namespace ## _ ## word ## _numeric_bang(self, rbObj); \
    } \
    else \
    { \
      namespace ## _ ## word ## _matrix_bang(self, rbObj); \
    } \
    return self; \
  }

#define MATRIX_MATH_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect) \
  MATRIX_MATH_RECURSION_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect); \
  MATRIX_MATH_MATRIX_BANG_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect); \
  MATRIX_MATH_BASIC_BANG_FUNC(namespace, strct, arytype, datatype, rbconv, word, sym, zero_protect); \
  static VALUE namespace ## _ ## word ## _at_bang( \
    VALUE self, VALUE rbTrgCoords, VALUE rbObj, \
    VALUE rbCoordStart, VALUE rbCoordEnd) \
  { \
    bool using_matrix = !(NUMERIC_P(rbObj)); \
    strct *trg_matrix = NULL; \
    strct *src_matrix = NULL; \
    VALUE rbTrgSize, rbStartSize, rbEndSize; \
    uint32_t trgSize, startSize, endSize, stackSize;\
    int32_t src_num = 0; \
    \
    strb_CheckMatrixI(self); \
    Data_Get_Struct(self, MatrixI, trg_matrix); \
    if(using_matrix) \
    { \
      strb_Check ## strct(rbObj); \
      Data_Get_Struct(rbObj, MatrixI, src_matrix); \
    } \
    else \
    { \
      src_num = FIX2INT(rbObj); \
    } \
    \
    rbTrgSize   = rb_funcall(rbTrgCoords, sym_size, 0); \
    rbStartSize = rb_funcall(rbCoordStart, sym_size, 0); \
    rbEndSize   = rb_funcall(rbCoordEnd, sym_size, 0); \
    trgSize   = FIX2INT(rbTrgSize); \
    startSize = FIX2INT(rbStartSize); \
    endSize   = FIX2INT(rbEndSize); \
    stackSize = trgSize; \
    \
    if(stackSize <= 0) \
    { \
      rb_raise(rb_eArgError, "Dimension stack size resulted in 0"); \
    } \
    if(trgSize != trg_matrix->dimensions->size) \
    { \
      rb_raise(rb_eArgError, \
        "Target Coords are not the same dimension as Matrix"); \
    } \
    if(using_matrix && src_matrix != NULL) \
    { \
      if(trg_matrix->dimensions->size != src_matrix->dimensions->size) \
      { \
        rb_raise(rb_eArgError, \
          "Source and Target Matrix must be of the same number of dimensions"); \
      } \
      if(startSize != src_matrix->dimensions->size) \
      { \
        rb_raise(rb_eArgError, \
          "Start Coords are not the same dimension as source Matrix"); \
      } \
      if(endSize != src_matrix->dimensions->size) \
      { \
        rb_raise(rb_eArgError, \
          "End Coords are not the same dimension as source Matrix"); \
      } \
    } \
    \
    ArrayI* *ranges = ALLOC_N(ArrayI*, stackSize); \
    ArrayI *args_array = strb_AllocArrayI(stackSize, 0); \
    ArrayI *trg_vec = strb_ArrayI_from_ruby(rbTrgCoords); \
    ArrayI *src_vec_start = strb_ArrayI_from_ruby(rbCoordStart); \
    ArrayI *src_vec_end = strb_ArrayI_from_ruby(rbCoordEnd); \
    DimensionNet *dimnet = strb_MakeDimensionNet(src_vec_start, src_vec_end); \
    \
    if(using_matrix && src_matrix != NULL) \
      strb_ModifyInMatrixI(src_matrix, dimnet); \
    for(uint32_t d = 0; d < stackSize; d++) \
    { \
      ranges[d] = strb_AllocArrayI(2, 0); \
      ranges[d]->data[0] = dimnet->dstart->data[d]; \
      ranges[d]->data[1] = dimnet->dend->data[d]; \
    } \
    if(using_matrix && src_matrix != NULL) \
    { \
      namespace ## _RangeRecursion_ ## word ## _matrix_bang( \
        0, stackSize, ranges, args_array, src_matrix, trg_matrix, dimnet, trg_vec); \
    } \
    else \
    { \
      namespace ## _RangeRecursion_ ## word ## _numeric_bang( \
        0, stackSize, ranges, args_array, src_num, trg_matrix, dimnet, trg_vec); \
    } \
    \
    for(uint32_t d = 0; d < stackSize; d++) \
    { \
      strb_ArrayI_free(ranges[d]); \
    } \
    strb_ArrayI_free(args_array); \
    strb_ArrayI_free(trg_vec); \
    strb_ArrayI_free(src_vec_start); \
    strb_ArrayI_free(src_vec_end); \
    strb_DimensionNet_free(dimnet); \
    \
    return self; \
  } \
  static VALUE namespace ## _ ## word(VALUE self, VALUE rbObj) \
  { \
    VALUE obj = rb_obj_dup(self); \
    namespace ## _ ## word ## _bang(obj, rbObj); \
    return obj; \
  } \
  static VALUE namespace ## _ ## word ## _at( \
    VALUE self, VALUE rbTrgCoords, VALUE rbObj, \
    VALUE rbCoordStart, VALUE rbCoordEnd) \
  { \
    VALUE obj = rb_obj_dup(self); \
    namespace ## _ ## word ## _at_bang( \
      obj, rbTrgCoords, rbObj, rbCoordStart, rbCoordEnd); \
    return obj; \
  }

  #define MATRIX_MATH_FUNCS(namespace, strct, arytype, datatype, rbconv) \
    MATRIX_MATH_FUNC(namespace, strct, arytype, datatype, rbconv, add, +, ); \
    MATRIX_MATH_FUNC(namespace, strct, arytype, datatype, rbconv, sub, -, ); \
    MATRIX_MATH_FUNC(namespace, strct, arytype, datatype, rbconv, mul, *, ); \
    MATRIX_MATH_FUNC(namespace, strct, arytype, datatype, rbconv, div, /, {if(num == 0) num = 1;}); \
    MATRIX_MATH_FUNC(namespace, strct, arytype, datatype, rbconv, replace, , );

#endif
