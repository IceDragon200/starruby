/*
  StarRuby
    matrix.h
 */
#ifndef STARRUBY_MATRIX_H
  #define STARRUBY_MATRIX_H

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
