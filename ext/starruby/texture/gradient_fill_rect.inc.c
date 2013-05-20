#define BIG_DELTA(x, m, n) (x * m / n)

#define CALC_DST_COLOR(m, n)                             \
  const int8_t dstRed   = CLAMPU255(baseRed   + BIG_DELTA(diffRed, m, n)); \
  const int8_t dstGreen = CLAMPU255(baseGreen + BIG_DELTA(diffGreen, m, n)); \
  const int8_t dstBlue  = CLAMPU255(baseBlue  + BIG_DELTA(diffBlue, m, n)); \
  const int8_t dstAlpha = CLAMPU255(baseAlpha + BIG_DELTA(diffAlpha, m, n));

#define SET_DST_COLOR(px)     \
  px->color.red   = dstRed;   \
  px->color.green = dstGreen; \
  px->color.blue  = dstBlue;  \
  px->color.alpha = dstAlpha;

static VALUE
Texture_gradient_fill_rect(int argc, VALUE* argv, VALUE self)
{
  bool vertical;
  Color color1;
  Color color2;
  int32_t padding;   /* Used to advance by 1 row in a texture */
  int32_t rows_back; /* Used by vertical to return to next column */
  Pixel* pixels;     /* Pointer to the Texture pixels data */
  Rect rect;
  int16_t baseAlpha;
  int16_t baseBlue;
  int16_t baseGreen;
  int16_t baseRed;
  int16_t diffAlpha;
  int16_t diffBlue;
  int16_t diffGreen;
  int16_t diffRed;
  Texture* texture;

  rb_check_frozen(self);

  if (argc == 3 || argc == 4)
  {
    VALUE rbRect, rbColor1, rbColor2, rbVertical;
    rb_scan_args(argc, argv, "31", &rbRect, &rbColor1, &rbColor2, &rbVertical);
    strb_RubyToRect(rbRect, &(rect));
    strb_RubyToColor(rbColor1, &(color1));
    strb_RubyToColor(rbColor2, &(color2));
    vertical = RTEST(rbVertical);
  } else if (argc == 6 || argc == 7)  {
    VALUE rbX, rbY, rbWidth, rbHeight, rbColor1, rbColor2, rbVertical;
    rb_scan_args(argc, argv, "61", &rbX, &rbY, &rbWidth, &rbHeight,
                                   &rbColor1, &rbColor2, &rbVertical);
    rect.x      = NUM2INT(rbX);
    rect.y      = NUM2INT(rbY);
    rect.width  = NUM2INT(rbWidth);
    rect.height = NUM2INT(rbHeight);
    strb_RubyToColor(rbColor1, &(color1));
    strb_RubyToColor(rbColor2, &(color2));
    vertical = RTEST(rbVertical);
  } else {
    rb_raise(rb_eArgError, "expected 3, 4, 6 or 7 arguments but recieved %d", argc);
  }

  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  if (!ModifyRectInTexture_Rect(texture, &(rect))) {
    return Qnil;
  }

  pixels = &(texture->pixels[rect.x + rect.y * texture->width]);

  baseRed   = color1.red;
  baseGreen = color1.green;
  baseBlue  = color1.blue;
  baseAlpha = color1.alpha;
  diffRed   = color2.red   - baseRed;
  diffGreen = color2.green - baseGreen;
  diffBlue  = color2.blue  - baseBlue;
  diffAlpha = color2.alpha - baseAlpha;

  if (vertical) {
    padding = texture->width - rect.width;
    for(int32_t y = 0; y < rect.height; y++, pixels += padding) {
      CALC_DST_COLOR(y, rect.height);
      for(int32_t x = 0; x < rect.width; x++, pixels++) {
        SET_DST_COLOR(pixels);
      }
    }
  } else {
    padding = texture->width;
    rows_back = padding * rect.height;
    for(int32_t x = 0; x < rect.width; x++, pixels -= rows_back, pixels++) {
      CALC_DST_COLOR(x, rect.width);
      for(int32_t y = 0; y < rect.height; y++, pixels += padding) {
        SET_DST_COLOR(pixels);
      }
    }
  }

  return Qnil;
}
