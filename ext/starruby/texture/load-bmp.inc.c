/*
typedef enum {
  BI_RGB,
  BI_RLE8,
  BI_RLE4,
  BI_BITFIELDS,
  BI_JPEG,
  BI_PNG,
  BI_ALPHABITFIELDS
} CompressionMethod;

typedef struct {
  Byte type[2];
  Size size;
  Short res1;
  Short res2;
  SInteger offset;
} BitmapFileHeader;

typedef struct {
  UInteger size;
  SInteger width;
  SInteger height;
  UShort color_planes;
  UShort bits_per_pixel;
  UInteger compression_meth;
  SInteger image_size;
  SInteger hres;
  SInteger vres;
  UInteger pallete_size;
  UInteger important_color_count;
} BitmapV5Header;

Void strb_TextureLoadBMP(String filename)
{

}

static VALUE Texture_s_load_bmp(VALUE self, VALUE rbFilename)
{

}
*/
//#define STRB_TEXTURE_LOAD_BMP
