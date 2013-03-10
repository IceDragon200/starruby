/*
  StarRuby Dimension Net
  vr 1.0.0
  */
#ifndef STARRUBY_DIMNET_PRV_H
  #define STARRUBY_DIMNET_PRV_H

  #include "strb_array.h"

  typedef struct
  {
    uint32_t size;
    ArrayI *dim_sizes, *dstart, *dend;
  } DimensionNet;

  inline void strb_DimensionNet_calc_sizes(DimensionNet *dimnet)
  {
    const uint32_t size = dimnet->size;
    for(uint32_t i = 0; i < size; i++)
    {
      dimnet->dim_sizes->data[i] = dimnet->dend->data[i] - dimnet->dstart->data[i];
    }
  }

  DimensionNet* strb_AllocDimensionNet(uint32_t size);
  DimensionNet* strb_MakeDimensionNet(ArrayI *dstart, ArrayI *dend);
  void strb_DimensionNet_free(DimensionNet *dimnet);

#endif

