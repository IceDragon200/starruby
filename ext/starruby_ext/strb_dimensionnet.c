#include "starruby.prv.h"
#include "strb_dimensionnet.h"

DimensionNet* strb_AllocDimensionNet(uint32_t size)
{
  DimensionNet *dimnet = ALLOC(DimensionNet);
  dimnet->size         = size;
  dimnet->dim_sizes    = strb_AllocArrayI(size, 0);
  dimnet->dstart       = strb_AllocArrayI(size, 0);
  dimnet->dend         = strb_AllocArrayI(size, 0);
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
  strb_ArrayI_free(dimnet->dim_sizes);
  strb_ArrayI_free(dimnet->dstart);
  strb_ArrayI_free(dimnet->dend);
  free(dimnet);
}
