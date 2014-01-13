/*
  StarRuby
    matrix.h
 */
#ifndef STARRUBY_MATRIX_H_
#define STARRUBY_MATRIX_H_

#include "strb_array.h"

typedef struct matrixI
{
  ArrayI *dimensions;
  ArrayI *offsets;
  ArrayI *data;
} MatrixI;

typedef struct matrixF
{
  ArrayI *dimensions;
  ArrayI *offsets;
  ArrayF *data;
} MatrixF;

typedef struct
{
  ArrayI *dimensions;
  ArrayI *offsets;
} IMatrix;

#endif
