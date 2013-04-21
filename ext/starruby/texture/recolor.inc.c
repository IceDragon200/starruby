Void strb_TextureRecolor(Texture* src_texture, Rect* rect,
                         const Color* src_color, Color* dst_color)
{
  Integer dst_x      = rect->x;
  Integer dst_y      = rect->y;
  Integer dst_width  = rect->width;
  Integer dst_height = rect->height;
  if (!ModifyRectInTexture(src_texture,
                           &(dst_x), &(dst_y), &(dst_width), &(dst_height))) {
    return;
  }
  const Integer tex_width = src_texture->width;
  const Integer padding = tex_width - dst_width;
  const Pixel src_pixel = (Pixel)(*src_color);
  const Pixel dst_pixel = (Pixel)(*dst_color);
  Pixel *pixels = &(src_texture->pixels[dst_x + dst_y * tex_width]);
  //const Integer tex_height = src_texture->height;
  for (Integer y = 0; y < dst_height; y++, pixels += padding) {
    for (Integer x = 0; x < dst_width; x++, pixels++) {
      if (pixels->value == src_pixel.value) {
        pixels->value = dst_pixel.value;
      }
    }
  }
}

static VALUE Texture_recolor(VALUE self, VALUE rbRect,
                             VALUE rbSrcColor, VALUE rbRepColor)
{
  Color* color1;
  Color* color2;
  Texture* texture;
  Rect* rect;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  Data_Get_Struct(rbRect, Rect, rect);
  Data_Get_Struct(rbSrcColor, Color, color1);
  Data_Get_Struct(rbRepColor, Color, color2);
  strb_TextureRecolor(texture, rect, color1, color2);
  return Qnil;
}
