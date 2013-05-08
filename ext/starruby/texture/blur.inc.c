/*
  StarRuby Texture
    blur
  vr 1.0.0
  */
#define NORMALIZE_COLORS3(target_color, color1, color2, color3)            \
  target_color.red   = ((color1.red   + color2.red   + color3.red)   / 3); \
  target_color.green = ((color1.green + color2.green + color3.green) / 3); \
  target_color.blue  = ((color1.blue  + color2.blue  + color3.blue)  / 3); \
  target_color.alpha = ((color1.alpha + color2.alpha + color3.alpha) / 3);

static VALUE
Texture_blur(VALUE self)
{
  Texture* texture;
  Texture* src_texture;
  Pixel* src_pixels;
  Pixel* dst_pixels;
  VALUE src_rbTexture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  src_rbTexture = rb_obj_dup(self);
  Data_Get_Struct(src_rbTexture, Texture, src_texture);

  src_pixels = src_texture->pixels;
  dst_pixels = texture->pixels;

  rb_check_frozen(self);

  for(int y = 1; y < texture->height; y++) {
    for(int x = 1; x < texture->width; x++) {
      Pixel cpx, hpx, vpx;
      Color rscol;

      const int cpx_index = x + (y * texture->width);

      cpx = (src_pixels[cpx_index]);
      hpx = (src_pixels[x - 1 + (y * texture->width)]);
      vpx = (src_pixels[x + ((y - 1) * texture->width)]);

      NORMALIZE_COLORS3(rscol, cpx.color, hpx.color, vpx.color);

      dst_pixels[cpx_index].color = rscol;
    }
  }

  for(int y = texture->height - 2; y > 0; y--) {
    for(int x = texture->width - 2; x > 0; x--) {
      Pixel cpx, hpx, vpx;
      Color rscol;

      const int cpx_index = x + (y * texture->width);

      cpx = (src_pixels[cpx_index]);
      hpx = (src_pixels[x + 1 + (y * texture->width)]);
      vpx = (src_pixels[x + ((y + 1) * texture->width)]);

      NORMALIZE_COLORS3(rscol, cpx.color, hpx.color, vpx.color);

      dst_pixels[cpx_index].color = rscol;
    }
  }

  rb_funcall(src_rbTexture, ID_dispose, 0);

  return self;
}
