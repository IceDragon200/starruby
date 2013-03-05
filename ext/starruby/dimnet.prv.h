/*

 */
#ifndef STARRUBY_DIMNET_PRV_H
  #define STARRUBY_DIMNET_PRV_H

  typedef struct
  {
    uint32_t size;
    ArrayI *dim_sizes, *dstart, *dend;
  } DimensionNet;

  void strb_DimensionNet_calc_sizes(DimensionNet *dimnet)
  {
    const uint32_t size = dimnet->size;
    for(uint32_t i = 0; i < size; i++)
    {
      dimnet->dim_sizes->data[i] = dimnet->dend->data[i] - dimnet->dstart->data[i];
    }
  }

  DimensionNet* strb_AllocDimensionNet(uint32_t size)
  {
    DimensionNet *dimnet = ALLOC(DimensionNet);
    dimnet->size = size;
    dimnet->dim_sizes = strb_AllocArrayI(size, 0);
    dimnet->dstart = strb_AllocArrayI(size, 0);
    dimnet->dend = strb_AllocArrayI(size, 0);
    return dimnet;
  }

  DimensionNet* strb_MakeDimensionNet(ArrayI *dstart, ArrayI *dend)
  {
    const uint32_t size = dstart->size;
    DimensionNet *dimnet = strb_AllocDimensionNet(size);
    strb_ArrayI_copy(dimnet->dstart, dstart);
    strb_ArrayI_copy(dimnet->dend, dend);
    strb_DimensionNet_calc_sizes(dimnet);
    return dimnet;
  }

  void strb_DimensionNet_free(DimensionNet *dimnet)
  {
    free(dimnet->dim_sizes);
    free(dimnet->dstart);
    free(dimnet->dend);
    free(dimnet);
  }

#endif
