Void strb_TextureMask(Texture *dst_texture, Texture *src_texture,
                      Integer dst_x, Integer dst_y, Rect* src_rect,
                      MaskType dst_mask, MaskType src_mask)
{
  Integer src_x      = src_rect->x;
  Integer src_y      = src_rect->y;
  Integer src_width  = src_rect->width;
  Integer src_height = src_rect->height;
  if (!ModifyRectInTexture(src_texture,
                           &(src_x), &(src_y), &(src_width), &(src_height))) {
    return;
  }

  const int srcTextureWidth  = src_texture->width;
  const int srcTextureHeight = src_texture->height;
  const int dstTextureWidth  = dst_texture->width;
  const int dstTextureHeight = dst_texture->height;

  if (dst_x < 0) {
    src_x -= dst_x;
    src_width += dst_x;
    if (srcTextureWidth <= src_x || src_width <= 0) {
      return;
    }
    dst_x = 0;
  } else if (dstTextureWidth <= dst_x) {
    return;
  }
  if (dst_y < 0) {
    src_y -= dst_y;
    src_height += dst_y;
    if (srcTextureHeight <= src_y || src_height <= 0) {
      return;
    }
    dst_y = 0;
  } else if (dstTextureHeight <= dst_y) {
    return;
  }

  const int width  = MIN(src_width,  dstTextureWidth - dst_x);
  const int height = MIN(src_height, dstTextureHeight - dst_y);
  Pixel* src = &(src_texture->pixels[src_x + src_y * srcTextureWidth]);
  Pixel* dst = &(dst_texture->pixels[dst_x + dst_y * dstTextureWidth]);

  const int srcPadding = srcTextureWidth - width;
  const int dstPadding = dstTextureWidth - width;

  Byte mask_value;

  for(int j = 0; j < height; j++, src += srcPadding, dst += dstPadding) {
    for(int k = 0; k < width; k++, src++, dst++) {
      switch (src_mask) {
        case MASK_ALPHA:
          mask_value = src->color.alpha;
          break;
        case MASK_GRAY:
          mask_value = (src->color.red + src->color.green + src->color.blue) / 3;
          break;
        case MASK_RED:
          mask_value = src->color.red;
          break;
        case MASK_GREEN:
          mask_value = src->color.green;
          break;
        case MASK_BLUE:
          mask_value = src->color.blue;
          break;
      }

      switch (dst_mask) {
        case MASK_ALPHA:
          dst->color.alpha = mask_value;
          break;
        case MASK_GRAY:
          dst->color.red   = DIV255(dst->color.red * mask_value);
          dst->color.green = DIV255(dst->color.green * mask_value);
          dst->color.blue  = DIV255(dst->color.blue * mask_value);
          break;
        case MASK_RED:
          dst->color.red = mask_value;
          break;
        case MASK_GREEN:
          dst->color.green = mask_value;
          break;
        case MASK_BLUE:
          dst->color.blue = mask_value;
          break;
      }
    }
  }
}

static VALUE Texture_mask(VALUE self, VALUE rbX, VALUE rbY, VALUE rbDstMask,
                          VALUE rbSrcTexture, VALUE rbSrcRect, VALUE rbSrcMask)
{
  rb_check_frozen(self);
  Texture* src_texture;
  Texture* dst_texture;
  Rect* src_rect;

  Integer x = NUM2INT(rbX);
  Integer y = NUM2INT(rbY);
  MaskType dst_mask = (MaskType)FIX2INT(rbDstMask);
  MaskType src_mask = (MaskType)FIX2INT(rbSrcMask);

  Data_Get_Struct(rbSrcTexture, Texture, src_texture);
  Data_Get_Struct(self, Texture, dst_texture);
  Data_Get_Struct(self, Rect, src_rect);

  strb_TextureCheckDisposed(src_texture);
  strb_TextureCheckDisposed(dst_texture);

  strb_TextureMask(dst_texture, src_texture,
                   x, y, src_rect,
                   dst_mask, src_mask);
  return Qnil;
}
