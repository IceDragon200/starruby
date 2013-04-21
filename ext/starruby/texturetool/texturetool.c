
// Verbosity
// 0 - Ignore and fix all errors internally
// 1 - Warn I
// 2 - Warn II
// 3 - Strict
#define VERBOSE_TEXTURE_TOOL 3

VALUE rb_mTextureTool = Qnil;

static VALUE
TextureTool_render_texture_fast(VALUE klass,
  VALUE rbDstTexture, VALUE rbDstX, VALUE rbDstY,
  VALUE rbSrcTexture,
  VALUE rbSrcX, VALUE rbSrcY, VALUE rbSrcWidth, VALUE rbSrcHeight,
  VALUE rbAlpha, VALUE rbTone, VALUE rbColor, VALUE rbBlendType)
{
  strb_CheckObjIsKindOf(rbDstTexture, rb_cTexture);
  strb_CheckObjIsKindOf(rbSrcTexture, rb_cTexture);

  rb_check_frozen(rbDstTexture);

  const Texture *dstTexture;
  Data_Get_Struct(rbDstTexture, Texture, dstTexture);
  strb_TextureCheckDisposed(dstTexture);

  const Texture *srcTexture;
  Data_Get_Struct(rbSrcTexture, Texture, srcTexture);
  strb_TextureCheckDisposed(srcTexture);

  const Tone *tone = NULL;
  const Color *color = NULL;

  if(!NIL_P(rbTone))
    Data_Get_Struct(rbTone, Tone, tone);

  if(!NIL_P(rbColor))
    Data_Get_Struct(rbColor, Color, color);

  int dstX = NUM2INT(rbDstX);
  int dstY = NUM2INT(rbDstY);

  int srcX      = NUM2INT(rbSrcX);
  int srcY      = NUM2INT(rbSrcY);
  int srcWidth  = NUM2INT(rbSrcWidth);
  int srcHeight = NUM2INT(rbSrcHeight);

  const uint8_t alpha = CLAMPU255(NUM2INT(rbAlpha));
  const int blendType = NUM2INT(rbBlendType);

  if (!ModifyRectInTexture(srcTexture,
                           &srcX, &srcY, &srcWidth, &srcHeight)) {
    return Qnil;
  }

  strb_TextureRender(srcTexture, dstTexture,
                    srcX, srcY, srcWidth, srcHeight, dstX, dstY,
                    alpha, tone, color, blendType);

  return Qnil;
}

static VALUE TextureTool_color_blend(
  VALUE klass, VALUE rbTexture, VALUE rbColor)
{
  strb_CheckObjIsKindOf(rbTexture, rb_cTexture);
  rb_check_frozen(rbTexture);

  const Texture* texture;
  Color color;

  Data_Get_Struct(rbTexture, Texture, texture);
  strb_TextureCheckDisposed(texture);

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
  strb_CheckObjIsKindOf(rbDstTexture, rb_cTexture);
  strb_CheckObjIsKindOf(rbSrcTexture, rb_cTexture);

  rb_check_frozen(rbDstTexture);

  const Texture* dstTexture;
  const Texture* srcTexture;
  Data_Get_Struct(rbDstTexture, Texture, dstTexture);
  Data_Get_Struct(rbSrcTexture, Texture, srcTexture);
  strb_TextureCheckDisposed(dstTexture);
  strb_TextureCheckDisposed(srcTexture);

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

/* 24/03/2013 TextureTool::noise */
#define RAND2 (rand() % 2)
#define COINFLIP (RAND2 == 0)
#define RELAY(condition, nc, no) (condition ? no : nc)

static VALUE TextureTool_noise(VALUE module, VALUE rbTexture, VALUE rbRect,
                               VALUE rbR, VALUE rbBipolar, VALUE rbSubtract)
{
  strb_CheckObjIsKindOf(rbRect, rb_cRect);
  strb_CheckObjIsKindOf(rbTexture, rb_cTexture);

  Texture *texture;
  Rect *rect;
  Data_Get_Struct(rbTexture, Texture, texture);
  Data_Get_Struct(rbRect, Rect, rect);

  const bool bipolar = RTEST(rbBipolar);
  const bool subtract = RTEST(rbSubtract);
  const uint8_t delta = (uint8_t)(MIN(MAX(NUM2DBL(rbR), 0.0), 1.0) * 255);
  int32_t dstX      = rect->x;
  int32_t dstY      = rect->y;
  int32_t dstWidth  = rect->width;
  int32_t dstHeight = rect->height;

  if(!ModifyRectInTexture(texture,
                          &dstX, &dstY, &dstWidth, &dstHeight)) {
    return Qnil;
  }

  int32_t padding = texture->width - dstWidth;

  Pixel *pixels = &(texture->pixels[dstX + dstY * texture->width]);

  srand(NUM2INT(rb_iv_get(rb_mTextureTool, "@noise_seed")));

  for(int32_t y = 0; y < dstHeight; y++, pixels += padding) {
    for(int32_t x = 0; x < dstWidth; x++, pixels++) {
      int16_t redd   = rand() % MAX(1, DIV255(RELAY(subtract, pixels->color.red, 255 - pixels->color.red) * delta));
      int16_t greend = rand() % MAX(1, DIV255(RELAY(subtract, pixels->color.green, 255 - pixels->color.green) * delta));
      int16_t blued  = rand() % MAX(1, DIV255(RELAY(subtract, pixels->color.blue, 255 - pixels->color.blue) * delta));

      if(bipolar) {
        if(COINFLIP) redd   = -redd;
        if(COINFLIP) greend = -greend;
        if(COINFLIP) blued  = -blued;
      }

      pixels->color.red   = CLAMPU255(pixels->color.red + redd);
      pixels->color.green = CLAMPU255(pixels->color.green + greend);
      pixels->color.blue  = CLAMPU255(pixels->color.blue + blued);
    }
  }

  return Qnil;
}

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
  rb_define_singleton_method(rb_mTextureTool, "noise",
                             TextureTool_noise, 5);
  rb_iv_set(rb_mTextureTool, "@noise_seed", INT2NUM(0));

  return rb_mTextureTool;
}
