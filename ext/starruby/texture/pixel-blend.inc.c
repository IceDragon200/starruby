/*
  StarRuby Texture Pixel Blend Functions
  */

//#define Lum(pixel) ((6969 * pixel->color.red + 23434 * pixel->color.green + 2365 * pixel->color.blue) / 32768)
#define Lum(pixel) ((pixel->color.red + pixel->color.green + pixel->color.blue) / 3)
#define Chroma(pixel) (MAX(MAX(pixel->color.red, pixel->color.green), pixel->color.blue) - MIN(MIN(pixel->color.red, pixel->color.green), pixel->color.blue))

#define ClipPlus(_c_, l, n) (l + (((_c_ - l) * l) / (l - n)))
#define ClipSub(_c_, l, n) (l + (((_c_ - l) * (1 - l)) / (n - l)))

static inline void ClipColor(Pixel *pixel)
{
  int l = Lum(pixel);
  int n = MIN(MIN(pixel->color.red, pixel->color.green), pixel->color.blue);
  int x = MAX(MAX(pixel->color.red, pixel->color.green), pixel->color.blue);
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
  const uint8_t d = DIV255((l - Lum(pixel)) * pixel->color.alpha);
  pixel->color.red   = CLAMPU255(pixel->color.red + d);
  pixel->color.green = CLAMPU255(pixel->color.green + d);
  pixel->color.blue  = CLAMPU255(pixel->color.blue + d);
  //ClipColor(pixel);
}

static void Pixel_blend_none(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  *dst = *src;
  dst->color.alpha = DIV255(src->color.alpha * alpha);
}

static void Pixel_blend_mask(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  dst->color.alpha = src->color.alpha;
}

static inline void Pixel_blend_alpha(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  if ((src->color.alpha == 0) || alpha == 0) return;
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  if ((beta == 255) || (dst->color.alpha == 0)) {
    dst->value = src->value;
    dst->color.alpha = beta;
  } else if (beta) {
    if (dst->color.alpha < beta) {
      dst->color.alpha = beta;
    }
    dst->color.red   = ALPHA(src->color.red,   dst->color.red,   beta);
    dst->color.green = ALPHA(src->color.green, dst->color.green, beta);
    dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  beta);
  }
}

static inline void Pixel_blend_add(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  const int addR = dst->color.red   + DIV255(src->color.red * beta);
  const int addG = dst->color.green + DIV255(src->color.green * beta);
  const int addB = dst->color.blue  + DIV255(src->color.blue * beta);
  dst->color.red   = MIN(255, addR);
  dst->color.green = MIN(255, addG);
  dst->color.blue  = MIN(255, addB);
  //dst->color.alpha = beta;
}

static inline void Pixel_blend_sub(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  const int subR = dst->color.red   - DIV255(src->color.red * beta);
  const int subG = dst->color.green - DIV255(src->color.green * beta);
  const int subB = dst->color.blue  - DIV255(src->color.blue * beta);
  dst->color.red   = MAX(0, subR);
  dst->color.green = MAX(0, subG);
  dst->color.blue  = MAX(0, subB);
  //dst->color.alpha = beta;
}

static inline void Pixel_blend_mul(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  const uint8_t beta = DIV255(src->color.alpha * alpha);
  const int addR = DIV255(dst->color.red   * DIV255(src->color.red * beta));
  const int addG = DIV255(dst->color.green * DIV255(src->color.green * beta));
  const int addB = DIV255(dst->color.blue  * DIV255(src->color.blue * beta));
  dst->color.red   = CLAMPU255(addR);
  dst->color.green = CLAMPU255(addG);
  dst->color.blue  = CLAMPU255(addB);
  //dst->color.alpha = beta;
}

// http://www.poynton.com/ColorFAQ.html
static inline void Pixel_tone(Pixel *dst, const Tone *tone, const uint8_t beta)
{
  if (tone->saturation < 255) {
    const uint8_t l = Lum(dst);
    dst->color.red   = ALPHA(dst->color.red,   l, tone->saturation);
    dst->color.green = ALPHA(dst->color.green, l, tone->saturation);
    dst->color.blue  = ALPHA(dst->color.blue,  l, tone->saturation);
  }
  if(tone->red != 0) {
    if (0 < tone->red) {
      dst->color.red = ALPHA(255, dst->color.red, DIV255(tone->red * beta));
    } else {
      dst->color.red = ALPHA(0,   dst->color.red, -DIV255(tone->red * beta));
    }
  }
  if(tone->green != 0) {
    if (0 < tone->green) {
      dst->color.green = ALPHA(255, dst->color.green, DIV255(tone->green * beta));
    } else {
      dst->color.green = ALPHA(0,   dst->color.green, -DIV255(tone->green * beta));
    }
  }
  if(tone->blue != 0) {
    if (0 < tone->blue) {
      dst->color.blue = ALPHA(255, dst->color.blue, DIV255(tone->blue * beta));
    } else {
      dst->color.blue = ALPHA(0,   dst->color.blue, -DIV255(tone->blue * beta));
    }
  }
}

//#define Pixel_blend_color(dst, src, alpha)
/* TODO
 */
static inline void Pixel_blend_color(Pixel *dst, const Pixel *src, const uint8_t alpha)
{
  //
}
