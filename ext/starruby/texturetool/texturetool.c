
// Verbosity
// 0 - Ignore and fix all errors internally
// 1 - Warn I
// 2 - Warn II
// 3 - Strict
#define VERBOSE_TEXTURE_TOOL 3

static VALUE
TextureTool_render_texture_fast(VALUE klass,
  VALUE rbDstTexture, VALUE rbDstX, VALUE rbDstY,
  VALUE rbSrcTexture,
  VALUE rbSrcX, VALUE rbSrcY, VALUE rbSrcWidth, VALUE rbSrcHeight,
  VALUE rbAlpha, VALUE rbTone, VALUE rbColor, VALUE rbBlendType)
{
  strb_CheckTexture(rbDstTexture);
  strb_CheckTexture(rbSrcTexture);

  rb_check_frozen(rbDstTexture);

  const Texture* dstTexture;
  const Texture* srcTexture;
  const Tone *tone = NULL;
  const Color *color = NULL;
  Data_Get_Struct(rbDstTexture, Texture, dstTexture);
  Data_Get_Struct(rbSrcTexture, Texture, srcTexture);
  strb_CheckDisposedTexture(dstTexture);
  strb_CheckDisposedTexture(srcTexture);

  if(!NIL_P(rbTone))
    Data_Get_Struct(rbTone, Tone, tone);

  if(!NIL_P(rbColor))
    Data_Get_Struct(rbColor, Color, color);

  int dstX = NUM2INT(rbDstX);
  int dstY = NUM2INT(rbDstY);

  int srcX = NUM2INT(rbSrcX);
  int srcY = NUM2INT(rbSrcY);
  int srcWidth = NUM2INT(rbSrcWidth);
  int srcHeight = NUM2INT(rbSrcHeight);

  const uint8_t alpha = CLAMPU255(NUM2INT(rbAlpha));
  const int blendType = NUM2INT(rbBlendType);

  if (!ModifyRectInTexture(srcTexture,
                           &(srcX), &(srcY), &(srcWidth), &(srcHeight))) {
    return Qnil;
  }

  RenderTexture(srcTexture, dstTexture,
                srcX, srcY, srcWidth, srcHeight, dstX, dstY,
                alpha, tone, color, blendType);

  return Qnil;
}

static VALUE TextureTool_color_blend(
  VALUE klass, VALUE rbTexture, VALUE rbColor)
{
  strb_CheckTexture(rbTexture);
  rb_check_frozen(rbTexture);

  const Texture* texture;
  Color color;

  Data_Get_Struct(rbTexture, Texture, texture);
  strb_CheckDisposedTexture(texture);

  strb_GetColorFromRubyValue(&color, rbColor);

  Pixel* src_pixels = texture->pixels;

  for(int y=0; y < texture->height; y++) {
    for(int x=0; x < texture->width; x++) {
      Pixel *dst;

      const int cpx_index = x + (y * texture->width);
      dst = &(src_pixels[cpx_index]);

      const uint8_t beta = DIV255(color.alpha * dst->color.alpha);
      if (dst->color.alpha < beta) {
        dst->color.alpha = beta;
      }
      dst->color.red   = ALPHA(color.red,   dst->color.red,   beta);
      dst->color.green = ALPHA(color.green, dst->color.green, beta);
      dst->color.blue  = ALPHA(color.blue,  dst->color.blue,  beta);
    }
  }

  return Qnil;
}

static VALUE TextureTool_clipping_mask(
  VALUE klass,
  VALUE rbDstTexture, VALUE rbDstX, VALUE rbDstY,
  VALUE rbSrcTexture,
  VALUE rbSrcX, VALUE rbSrcY, VALUE rbSrcWidth, VALUE rbSrcHeight)
{
  strb_CheckTexture(rbDstTexture);
  strb_CheckTexture(rbSrcTexture);

  rb_check_frozen(rbDstTexture);

  const Texture* dstTexture;
  const Texture* srcTexture;
  Data_Get_Struct(rbDstTexture, Texture, dstTexture);
  Data_Get_Struct(rbSrcTexture, Texture, srcTexture);
  strb_CheckDisposedTexture(dstTexture);
  strb_CheckDisposedTexture(srcTexture);

  Pixel* src_pixels = srcTexture->pixels;
  Pixel* dst_pixels = dstTexture->pixels;

  int dstX = NUM2INT(rbDstX);
  int dstY = NUM2INT(rbDstY);

  int srcX = NUM2INT(rbSrcX);
  int srcY = NUM2INT(rbSrcY);
  int srcWidth = NUM2INT(rbSrcWidth);
  int srcHeight = NUM2INT(rbSrcHeight);

  if (!ModifyRectInTexture(srcTexture,
                           &(srcX), &(srcY), &(srcWidth), &(srcHeight))) {
    return Qnil;
  }

  for(int y=0; y < srcHeight; y++) {
    for(int x=0; x < srcWidth; x++) {
      Pixel *dst, *src;
      int sx = srcX + x;
      int sy = srcY + y;
      int dx = dstX + x;
      int dy = dstY + y;

      dst = &(dst_pixels[(dx + (dy * dstTexture->width))]);
      src = &(src_pixels[(sx + (sy * srcTexture->width))]);

      dst->color.red   = src->color.red;
      dst->color.green = src->color.green;
      dst->color.blue  = src->color.blue;
      dst->color.alpha = DIV255(dst->color.alpha * src->color.alpha);
    };
  };

  return Qnil;
}

VALUE rb_mTextureTool = Qnil;

VALUE strb_InitializeTextureTool(VALUE rb_mStarRuby)
{
  rb_mTextureTool = rb_define_module("TextureTool");
  rb_define_singleton_method(
    rb_mTextureTool, "render_texture_fast",
    TextureTool_render_texture_fast, 12);
  rb_define_singleton_method(rb_mTextureTool, "color_blend",
                   TextureTool_color_blend, 2);
  rb_define_singleton_method(rb_mTextureTool, "clipping_mask",
                   TextureTool_clipping_mask, 8);

  return rb_mTextureTool;
}
