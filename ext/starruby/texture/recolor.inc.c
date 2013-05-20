void strb_TextureRecolor(Texture* src_texture, Rect* rect,
                         const Color* src_color, Color* rep_color)
{
  int32_t dst_x      = rect->x;
  int32_t dst_y      = rect->y;
  int32_t dst_width  = rect->width;
  int32_t dst_height = rect->height;
  if (!ModifyRectInTexture(src_texture,
                           &(dst_x), &(dst_y), &(dst_width), &(dst_height))) {
    return;
  }
  const int32_t tex_width = src_texture->width;
  const int32_t padding = tex_width - dst_width;
  const Pixel* src_pixel = (Pixel*)(src_color);
  const Pixel* rep_pixel = (Pixel*)(rep_color);
  Pixel *pixels = &(src_texture->pixels[dst_x + dst_y * tex_width]);
  //const int32_t tex_height = src_texture->height;
  for (int32_t y = 0; y < dst_height; y++, pixels += padding) {
    for (int32_t x = 0; x < dst_width; x++, pixels++) {
      if (TexturePixelRGBAMatch(pixels, src_pixel)) {
        pixels->color.red   = rep_pixel->color.red;
        pixels->color.green = rep_pixel->color.green;
        pixels->color.blue  = rep_pixel->color.blue;
        pixels->color.alpha = rep_pixel->color.alpha;
      }
    }
  }
}

static VALUE Texture_recolor(VALUE self, VALUE rbRect,
                             VALUE rbSrcColor, VALUE rbRepColor)
{
  Color color1;
  Color color2;
  Texture* texture;
  Rect rect;

  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  strb_RubyToRect(rbRect, &rect);
  strb_RubyToColor(rbSrcColor, &color1);
  strb_RubyToColor(rbRepColor, &color2);
  strb_TextureRecolor(texture, &rect, &color1, &color2);
  return Qnil;
}
