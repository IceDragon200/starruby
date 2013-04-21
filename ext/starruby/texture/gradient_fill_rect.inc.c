static VALUE
Texture_gradient_fill_rect(VALUE self,
                  VALUE rbX, VALUE rbY,
                  VALUE rbWidth, VALUE rbHeight,
                  VALUE rbColor1, VALUE rbColor2, VALUE rbVertical)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  int32_t dstX      = NUM2INT(rbX);
  int32_t dstY      = NUM2INT(rbY);
  int32_t dstWidth  = NUM2INT(rbWidth);
  int32_t dstHeight = NUM2INT(rbHeight);

  if (!ModifyRectInTexture(texture,
                           &dstX, &dstY, &dstWidth, &dstHeight)) {
    return Qnil;
  }

  Color color1, color2;
  strb_GetColorFromRubyValue(&color1, rbColor1);
  strb_GetColorFromRubyValue(&color2, rbColor2);

  Pixel *pixels = &(texture->pixels[dstX + dstY * texture->width]);

  const int16_t baseRed   = color1.red;
  const int16_t baseGreen = color1.green;
  const int16_t baseBlue  = color1.blue;
  const int16_t baseAlpha = color1.alpha;

  const int16_t diffRed   = color2.red   - baseRed;
  const int16_t diffGreen = color2.green - baseGreen;
  const int16_t diffBlue  = color2.blue  - baseBlue;
  const int16_t diffAlpha = color2.alpha - baseAlpha;

#define BIG_DELTA(x, m, n) (x * m / n)

#define CALC_DST_COLOR(m, n)                             \
  const uint8_t dstRed   = CLAMPU255(baseRed   + BIG_DELTA(diffRed, m, n)); \
  const uint8_t dstGreen = CLAMPU255(baseGreen + BIG_DELTA(diffGreen, m, n)); \
  const uint8_t dstBlue  = CLAMPU255(baseBlue  + BIG_DELTA(diffBlue, m, n)); \
  const uint8_t dstAlpha = CLAMPU255(baseAlpha + BIG_DELTA(diffAlpha, m, n));

#define SET_DST_COLOR(px)     \
  px->color.red   = dstRed;   \
  px->color.green = dstGreen; \
  px->color.blue  = dstBlue;  \
  px->color.alpha = dstAlpha;

  if(RTEST(rbVertical)) {
    const int32_t padding = texture->width - dstWidth;
    for(int32_t y = 0; y < dstHeight; y++, pixels += padding) {
      CALC_DST_COLOR(y, dstHeight);
      for(int32_t x = 0; x < dstWidth; x++, pixels++) {
        SET_DST_COLOR(pixels);
      }
    }
  } else {
    const int32_t padding = texture->width;
    const int32_t rows_back = padding * dstHeight;
    for(int32_t x = 0; x < dstWidth; x++, pixels -= rows_back, pixels++) {
      CALC_DST_COLOR(x, dstWidth);
      for(int32_t y = 0; y < dstHeight; y++, pixels += padding) {
        SET_DST_COLOR(pixels);
      }
    }
  }

  return Qnil;
}
