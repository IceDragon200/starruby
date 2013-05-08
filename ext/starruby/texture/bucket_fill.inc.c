#define ROW_CHECKED 0x10
#define ROW_USED 0x01

typedef Byte** Bitmark;

Boolean strb_TextureBucketFillRow(Texture* texture, Integer x, Integer y,
                                  const Color* color, const Pixel* lookup_pixel,
                                  const Bitmark checked)
{
  Boolean filled_row = False;
  Integer rx = x, ry = y;
  Pixel* pixels = &(TextureGetPixel(texture, rx, ry));
  for (;rx < texture->width; rx++, pixels++) {
    if (TexturePixelRGBAMatch(pixels, lookup_pixel)) {
      filled_row = True;
      pixels->color.red   = color->red;
      pixels->color.green = color->green;
      pixels->color.blue  = color->blue;
      pixels->color.alpha = color->alpha;
      checked[ry][rx] = ROW_CHECKED | ROW_USED;
    } else {
      checked[ry][rx] = ROW_CHECKED;
      break;
    }
  }
  rx = x - 1;
  if (rx >= 0) {
    pixels = &(TextureGetPixel(texture, rx, ry));
    for (;rx >= 0; rx--, pixels--) {
      if (TexturePixelRGBAMatch(pixels, lookup_pixel)) {
        filled_row = True;
        pixels->color.red   = color->red;
        pixels->color.green = color->green;
        pixels->color.blue  = color->blue;
        pixels->color.alpha = color->alpha;
        checked[ry][rx] = ROW_CHECKED | ROW_USED;
      } else {
        checked[ry][rx] = ROW_CHECKED;
        break;
      }
    }
  }
  return filled_row;
}

#define ROW_FILL \
  filled_row = False; \
  for (Integer j = 0; j < texture->width; j++) { \
    if (rows[oy][j] & ROW_CHECKED) { \
      if (TexturePosInBound(texture, j, ty)) { \
        if (!(rows[ty][j] & ROW_CHECKED)) { \
          filled_row = strb_TextureBucketFillRow(texture, j, ty, color, \
                                                 &(lookup_pixel), (Bitmark)rows); \
        } \
      } \
    } \
  } \
  oy = ty; \
  if (!(filled_row)) { break; }

Void strb_TextureBucketFill(Texture* texture, Integer x, Integer y,
                            Color* color)
{
  const Pixel lookup_pixel = TextureGetPixel(texture, x, y);
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (!(x < texture->width)) x = texture->width-1;
  if (!(y < texture->height)) y = texture->height-1;
  Byte** rows = malloc(sizeof(Byte*) * texture->height);
  for (Integer rry = 0; rry < texture->height; rry++) {
    rows[rry] = malloc(sizeof(Byte) * texture->width);
    for (Integer rrx = 0; rrx < texture->width; rrx++) {
      rows[rry][rrx] = 0;
    }
  }
  strb_TextureBucketFillRow(texture, x, y, color,
                            &(lookup_pixel), (Bitmark)rows);
  Integer oy = y;
  Integer ty = y + 1;
  Boolean filled_row = False;
  for (;ty < texture->height; ty++) {
    ROW_FILL
  }
  oy = y;
  ty = y - 1;
  for (;ty >= 0; ty--) {
    ROW_FILL
  }
  /* Cleanup */
  for (Integer rry = 0; rry < texture->height; rry++) {
    free(rows[rry]);
  }
  free(rows);
}

static VALUE Texture_bucket_fill(VALUE self,
                                 VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  Color* color;
  Integer x, y;
  Texture* texture;

  x = NUM2INT(rbX);
  y = NUM2INT(rbY);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  strb_CheckObjIsKindOf(rbColor, rb_cColor);
  Data_Get_Struct(self, Color, color);
  strb_TextureBucketFill(texture, x, y, color);
  return Qnil;
}
