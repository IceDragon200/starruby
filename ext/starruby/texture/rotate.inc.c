
Void strb_TextureRotate(Texture *dst_texture, Texture *src_texture,
                        RotateType rotate_direction)
{
  const Integer width  = src_texture->width;
  const Integer height = src_texture->height;
  Pixel* dst_data = dst_texture->pixels;
  Pixel* src_data = src_texture->pixels;

  if (rotate_direction == ROTATE_NONE) {
    /* Nothing here folks, please move on */
  } else if (rotate_direction == ROTATE_HORZ) {
    for (Integer y = 0; y < height; y++) {
      Integer row = y * width;
      for (Integer x = 0; x < width; x++) {
        dst_data[(width - 1) - x + row] = src_data[x + row];
      }
    }
  } else if (rotate_direction == ROTATE_VERT) {
    for (Integer y = 0; y < height; y++) {
      Integer row = y * width;
      Integer dst_row = (height - 1 - y) * width;
      for (Integer x = 0; x < width; x++) {
        dst_data[x + dst_row] = src_data[x + row];
      }
    }
  /* Clockwise Rotation by 90*
      Width becomes Height and Height becomes Width
      (x) becomes (+y)
      (y) becomes (-x)
   */
  } else if (rotate_direction == ROTATE_CW) {
    /* HACK Width is swapped with Height, because the datasize remains the same
       we don't really have to worry about it too much */
    dst_texture->width = src_texture->height;
    dst_texture->height = src_texture->width;

    for (Integer y = 0; y < height; y++) {
      Integer row = y * width;
      for (Integer x = 0; x < width; x++) {
        dst_data[(height - 1) - y + x * height] = src_data[x + row];
      }
    }
  /* Counter Clockwise Rotation by 90*
      Width becomes Height and Height becomes Width
      (x) becomes (-y)
      (y) becomes (+x)
   */
  } else if (rotate_direction == ROTATE_CCW) {
    /* HACK Width is swapped with Height, because the datasize remains the same
       we don't really have to worry about it too much */
    dst_texture->width = src_texture->height;
    dst_texture->height = src_texture->width;

    for (Integer y = 0; y < height; y++) {
      Integer row = y * width;
      for (Integer x = 0; x < width; x++) {
        dst_data[y + ((width - 1 - x) * height)] = src_data[x + row];
      }
    }
  /* 180* Rotation
      Width and Height remain the same
      (x) becomes (-y)
      (y) becomes (-x)
   */
  } else if (rotate_direction == ROTATE_180) {
    for (Integer y = 0; y < height; y++) {
      Integer row = y * width;
      Integer dst_row = (height - 1 - y) * width;
      for (Integer x = 0; x < width; x++) {
        dst_data[(width - 1) - x + dst_row] = src_data[x + row];
      }
    }
  }
}

static VALUE Texture_rotate(VALUE self, VALUE rbRotateType)
{
  rb_check_frozen(self);
  VALUE rbSrcTexture = rb_obj_dup(self);
  RotateType rotatetype = (RotateType)FIX2INT(rbRotateType);
  if (rotatetype != ROTATE_NONE &&
      rotatetype != ROTATE_CW &&
      rotatetype != ROTATE_CCW &&
      rotatetype != ROTATE_180 &&
      rotatetype != ROTATE_HORZ &&
      rotatetype != ROTATE_VERT) {
    rb_raise(rb_eArgError, "invalid RotateType %d", rotatetype);
  }
  Texture* src_texture;
  Texture* dst_texture;
  Data_Get_Struct(rbSrcTexture, Texture, src_texture);
  Data_Get_Struct(self, Texture, dst_texture);
  strb_TextureCheckDisposed(src_texture);
  strb_TextureCheckDisposed(dst_texture);
  strb_TextureRotate(dst_texture, src_texture, rotatetype);
  Texture_dispose(rbSrcTexture);
  return Qnil;
}
