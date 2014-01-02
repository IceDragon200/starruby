/*
 * StarRuby Bytemap
 */

#ifndef STARRUBY_BYTEMAP_H
#define STARRUBY_BYTEMAP_H

typedef struct strb_bytemap
{
  uint16_t width;
  uint16_t height;
  uint32_t stride; /* Bytes per row */
  uint32_t size;   /* width * height */
  uint8_t* data;
} Bytemap;

#define BYTEMAP_OOR(bytemap, x, y) (x < 0 || y < 0 || x >= (int32_t)bytemap->width || y >= (int32_t)bytemap->height)

#endif
