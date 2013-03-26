/*
  StarRuby Texture Pixel Blend Functions
  */

#define Lum(pixel) ((6969 * pixel->color.red + 23434 * pixel->color.green + 2365 * pixel->color.blue) / 32768)

#define ClipPlus(_c_, l, n) (l + (((_c_ - l) * l) / (l - n)))
#define ClipSub(_c_, l, n) (l + (((_c_ - l) * (1 - l)) / (n - l)))

static inline void ClipColor(Pixel *pixel)
{
  int16_t l = Lum(pixel);
  int16_t n = MIN(MIN(pixel->color.red, pixel->color.green), pixel->color.blue);
  int16_t x = MAX(MAX(pixel->color.red, pixel->color.green), pixel->color.blue);
  if(n < 0)
  {
    pixel->color.red   = ClipPlus(pixel->color.red,   l, n);
    pixel->color.green = ClipPlus(pixel->color.green, l, n);
    pixel->color.blue  = ClipPlus(pixel->color.blue,  l, n);
  }
  if(x > 255)
  {
    pixel->color.red   = ClipSub(pixel->color.red,   l, x);
    pixel->color.green = ClipSub(pixel->color.green, l, x);
    pixel->color.blue  = ClipSub(pixel->color.blue,  l, x);
  }
}

static inline void SetLum(Pixel *pixel, const uint8_t l)
{
  uint8_t d = l - Lum(pixel);
  pixel->color.red   = CLAMPU255(pixel->color.red + d);
  pixel->color.green = CLAMPU255(pixel->color.green + d);
  pixel->color.blue  = CLAMPU255(pixel->color.blue + d);
  //ClipColor(pixel);
}

static void Pixel_blend_none(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  *dst = *src;
}

static void Pixel_blend_mask(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  dst->color.alpha = src->color.alpha;
}

static void Pixel_blend_alpha(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  const uint8_t dstAlpha = dst->color.alpha;
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  if ((beta == 255) | (dstAlpha == 0)) {
    dst->color.alpha = beta;
    dst->color.red   = src->color.red;
    dst->color.green = src->color.green;
    dst->color.blue  = src->color.blue;
  } else if (beta) {
    if (dstAlpha < beta) {
      dst->color.alpha = beta;
    }
    dst->color.red   = ALPHA(src->color.red,   dst->color.red,   beta);
    dst->color.green = ALPHA(src->color.green, dst->color.green, beta);
    dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  beta);
  }
}

static void Pixel_blend_color(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  if(dst->color.alpha == 0 || src->color.alpha == 0) return;
  SetLum(src, Lum(dst));
}

static void Pixel_blend_add(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  const int addR = src->color.red   + DIV255(dst->color.red * alpha);
  const int addG = src->color.green + DIV255(dst->color.green * alpha);
  const int addB = src->color.blue  + DIV255(dst->color.blue * alpha);
  dst->color.red   = MIN(255, addR);
  dst->color.green = MIN(255, addG);
  dst->color.blue  = MIN(255, addB);
  dst->color.alpha = beta;
}

static void Pixel_blend_sub(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  const int subR = -src->color.red   + DIV255(dst->color.red * alpha);
  const int subG = -src->color.green + DIV255(dst->color.green * alpha);
  const int subB = -src->color.blue  + DIV255(dst->color.blue * alpha);
  dst->color.red   = MAX(0, subR);
  dst->color.green = MAX(0, subG);
  dst->color.blue  = MAX(0, subB);
  dst->color.alpha = beta;
}

static void Pixel_blend_mul(Pixel *dst, Pixel *src, const uint8_t alpha)
{
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  const int addR = DIV255(src->color.red   * DIV255(dst->color.red * alpha));
  const int addG = DIV255(src->color.green * DIV255(dst->color.green * alpha));
  const int addB = DIV255(src->color.blue  * DIV255(dst->color.blue * alpha));
  dst->color.red   = CLAMPU255(addR);
  dst->color.green = CLAMPU255(addG);
  dst->color.blue  = CLAMPU255(addB);
  dst->color.alpha = beta;
}

// http://www.poynton.com/ColorFAQ.html
// static inline void Pixel_tone(Pixel *dst, Tone *tone, uint8_t beta)
#define Pixel_tone(dst, tone, beta) \
{ \
  if (tone->saturation < 255) { \
    const uint8_t y = Lum(dst); \
    dst->color.red   = ALPHA(dst->color.red,   y, tone->saturation); \
    dst->color.green = ALPHA(dst->color.green, y, tone->saturation); \
    dst->color.blue  = ALPHA(dst->color.blue,  y, tone->saturation); \
  } \
  if(tone->red != 0) { \
    if (0 < tone->red) { \
      dst->color.red = ALPHA(255, dst->color.red, DIV255(tone->red * beta)); \
    } else { \
      dst->color.red = ALPHA(0,   dst->color.red, -DIV255(tone->red * beta)); \
    } \
  } \
  if(tone->green != 0) { \
    if (0 < tone->green) { \
      dst->color.green = ALPHA(255, dst->color.green, DIV255(tone->green * beta));\
    } else { \
      dst->color.green = ALPHA(0,   dst->color.green, -DIV255(tone->green * beta)); \
    } \
  } \
  if(tone->blue != 0) { \
    if (0 < tone->blue) { \
      dst->color.blue = ALPHA(255, dst->color.blue, DIV255(tone->blue * beta)); \
    } else { \
      dst->color.blue = ALPHA(0,   dst->color.blue, -DIV255(tone->blue * beta)); \
    } \
  } \
}

#define is_valid_tone(tone_p) (tone_p && (tone_p->saturation < 255 || tone_p->red != 0 || tone_p->green != 0 || tone_p->blue != 0))
#define is_valid_color(color_p) (color_p && (color_p->alpha != 0))
#define IMGLOOP(content) \
  for(int j = 0; j < height; j++, src += srcPadding, dst += dstPadding) { \
    for(int k = 0; k < width; k++, src++, dst++) { \
      content; \
    } \
  }

static void
RenderTexture_blend(const Texture* srcTexture, const Texture* dstTexture,
              int srcX, int srcY, int srcWidth, int srcHeight,
              int dstX, int dstY,
              const uint8_t alpha, const Tone *tone, const Color *color,
              const BlendFunc blendFunc)
{
  const int srcTextureWidth  = srcTexture->width;
  const int srcTextureHeight = srcTexture->height;
  const int dstTextureWidth  = dstTexture->width;
  const int dstTextureHeight = dstTexture->height;
  if (dstX < 0) {
    srcX -= dstX;
    srcWidth += dstX;
    if (srcTextureWidth <= srcX || srcWidth <= 0) {
      return;
    }
    dstX = 0;
  } else if (dstTextureWidth <= dstX) {
    return;
  }
  if (dstY < 0) {
    srcY -= dstY;
    srcHeight += dstY;
    if (srcTextureHeight <= srcY || srcHeight <= 0) {
      return;
    }
    dstY = 0;
  } else if (dstTextureHeight <= dstY) {
    return;
  }

  const int width  = MIN(srcWidth,  dstTextureWidth - dstX);
  const int height = MIN(srcHeight, dstTextureHeight - dstY);
  Pixel* src = &(srcTexture->pixels[srcX + srcY * srcTextureWidth]);
  Pixel* dst = &(dstTexture->pixels[dstX + dstY * dstTextureWidth]);
  const int srcPadding = srcTextureWidth - width;
  const int dstPadding = dstTextureWidth - width;

  const bool use_tone  = is_valid_tone(tone);
  const bool use_color = is_valid_color(color);

  if(use_tone && use_color) {
    IMGLOOP({
      Pixel pixel = *src;
      Pixel_tone((&pixel), tone, alpha);
      (*blendFunc)(dst, &pixel, alpha);
      Pixel_blend_color(dst, (Pixel*)color, 255);
    })
  } else if(use_tone) {
    IMGLOOP({
      Pixel pixel = *src;
      Pixel_tone((&pixel), tone, alpha);
      (*blendFunc)(dst, &pixel, alpha);
    })
  } else if(use_color) {
    IMGLOOP({
      (*blendFunc)(dst, src, alpha);
      Pixel_blend_color(dst, (Pixel*)color, 255);
    })
  } else {
    IMGLOOP({ (*blendFunc)(dst, src, alpha); })
  }
}
