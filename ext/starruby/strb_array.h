#ifndef STARRUBY_ARRAY_H
  #define STARRUBY_ARRAY_H

  typedef struct arrayI
  {
    uint32_t size;
    int32_t *data;
  } ArrayI;

  typedef struct arrayF
  {
    uint32_t size;
    double *data;
  } ArrayF;

  ArrayI* strb_AllocArrayI(uint32_t size, int32_t default_value);
  ArrayF* strb_AllocArrayF(uint32_t size, double default_value);
  ArrayI* strb_ArrayI_from_ruby(VALUE array);
  ArrayF* strb_ArrayF_from_ruby(VALUE array);
  void strb_ArrayI_free(ArrayI *array);
  void strb_ArrayF_free(ArrayF *array);
  bool strb_ArrayI_comp(ArrayI *a, ArrayI *b);
  VALUE strb_ArrayI_to_ruby(ArrayI *array);
  VALUE strb_ArrayF_to_ruby(ArrayF *array);

  /* Inline */
  inline void
  strb_ArrayI_copy(ArrayI *trg_array, ArrayI *src_array)
  {
    uint32_t size = trg_array->size = src_array->size;
    MEMCPY(trg_array->data, src_array->data, int32_t, size);
  }

  inline void
  strb_ArrayF_copy(ArrayF *trg_array, ArrayF *src_array)
  {
    uint32_t size = trg_array->size = src_array->size;
    MEMCPY(trg_array->data, src_array->data, double, size);
  }

  // Helpers
  inline uint32_t strb_ArrayI_size(ArrayI *array)
  {
    return array->size;
  }

  inline int32_t strb_ArrayI_entry(ArrayI *array, uint32_t index)
  {
    return array->data[index];
  }

  inline void strb_ArrayI_entry_set(ArrayI *array, uint32_t index, int32_t v)
  {
    array->data[index] = v;
  }

#endif
