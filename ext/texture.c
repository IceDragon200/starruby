/*
 * StarRuby Texture
 */
#include <png.h>
#include <ruby.h>
#include "starruby.prv.h"
#include "color.h"
#include "rect.h"
#include "tone.h"
#include "vector.h"
#include "font.h"
#include "texture.prv.h"

volatile VALUE rb_cTexture = Qundef;

static inline void
strb_RubyToBlendType(VALUE val, BlendType* blend_type)
{
  switch(TYPE(val)) {
    case T_FIXNUM: {
      *blend_type = NUM2INT(val);
      break;
    }
    case T_SYMBOL: {
      if (val == symbol_none) {
        *blend_type = BLEND_TYPE_NONE;
      } else if (val == symbol_alpha) {
        *blend_type = BLEND_TYPE_ALPHA;
      } else if (val == symbol_add) {
        *blend_type = BLEND_TYPE_ADD;
      } else if (val == symbol_sub) {
        *blend_type = BLEND_TYPE_SUBTRACT;
      } else if (val == symbol_multiply) {
        *blend_type = BLEND_TYPE_MULTIPLY;
      } else if (val == symbol_divide) {
        *blend_type = BLEND_TYPE_DIVIDE;
      } else if (val == symbol_src_mask) {
        *blend_type = BLEND_TYPE_SRC_MASK;
      } else if (val == symbol_dst_mask) {
        *blend_type = BLEND_TYPE_DST_MASK;
      } else if (val == symbol_clear) {
        *blend_type = BLEND_TYPE_CLEAR;
      }
      break;
    }
    default: {
      rb_raise(rb_eTypeError, "failed to convert %s to BlendType",
               rb_obj_classname(val));
      break;
    }
  }
}

inline bool
strb_Texture_is_disposed(const Texture* const texture)
{
  return !texture->pixels;
}

inline void
strb_TextureCheckDisposed(const Texture* const texture)
{
  if (!texture->pixels) {
    rb_raise(rb_eRuntimeError,
             "can't modify disposed StarRuby::Texture");
  }
}

/*
 */
inline static bool
ModifyRectInTexture(const Texture* texture,
                    int32_t* x, int32_t* y,
                    int32_t* width, int32_t* height)
{
  if (*x < 0) {
    *width += *x;
    *x = 0;
  }
  if (*y < 0) {
    *height += *y;
    *y = 0;
  }
  /* Is the X, Y outside the texture bounds? */
  if (texture->width <= *x || texture->height <= *y) {
    return false;
  }

  const int32_t x2 = *x + *width;
  const int32_t y2 = *y + *height;

  if (x2 > texture->width) {
    *width -= x2 - texture->width;
  }
  if (y2 > texture->height) {
    *height -= y2 - texture->height;
  }

  if (*width <= 0 || *height <= 0) {
    return false;
  }

  return true;
}

inline static bool ModifyRectInTexture_Rect(Texture* texture, Rect* rect)
{
  return ModifyRectInTexture(texture, &(rect->x), &(rect->y),
                                      &(rect->width), &(rect->height));
}

#define ASSIGN_MATRIX(options, val)                                \
  Check_Type(val, T_ARRAY);                                        \
  VALUE* values = RARRAY_PTR(val);                                 \
  switch (RARRAY_LEN(val)) {                                       \
  case 2:                                                          \
    {                                                              \
      VALUE row0 = values[0];                                      \
      VALUE row1 = values[1];                                      \
      Check_Type(row0, T_ARRAY);                                   \
      Check_Type(row1, T_ARRAY);                                   \
      if (RARRAY_LEN(row0) != 2) {                                 \
        rb_raise(rb_eArgError, "matrix array must be 2x2 or 4x1"); \
      }                                                            \
      if (RARRAY_LEN(row1) != 2) {                                 \
        rb_raise(rb_eArgError, "matrix array must be 2x2 or 4x1"); \
      }                                                            \
      options->matrix.a = NUM2DBL(RARRAY_PTR(row0)[0]);            \
      options->matrix.b = NUM2DBL(RARRAY_PTR(row0)[1]);            \
      options->matrix.c = NUM2DBL(RARRAY_PTR(row1)[0]);            \
      options->matrix.d = NUM2DBL(RARRAY_PTR(row1)[1]);            \
      options->matrix.tx = 0;                                      \
      options->matrix.ty = 0;                                      \
    }                                                              \
    break;                                                         \
  case 4:                                                          \
    {                                                              \
      options->matrix.a = NUM2DBL(values[0]);                      \
      options->matrix.b = NUM2DBL(values[1]);                      \
      options->matrix.c = NUM2DBL(values[2]);                      \
      options->matrix.d = NUM2DBL(values[3]);                      \
      options->matrix.tx = 0;                                      \
      options->matrix.ty = 0;                                      \
    }                                                              \
    break;                                                         \
  default:                                                         \
    rb_raise(rb_eArgError, "matrix array must be 2x2 or 4x1");     \
    break;                                                         \
  }

static void
AssignPerspectiveOptions(PerspectiveOptions* options, VALUE rbOptions,
                         const Texture* texture)
{
  volatile VALUE val;
  Check_Type(rbOptions, T_HASH);
  MEMZERO(options, PerspectiveOptions, 1);
  options->intersectionX = texture->width  >> 1;
  options->intersectionY = texture->height >> 1;
  options->viewAngle = PI / 4;
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_x))) {
    options->cameraX = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_y))) {
    options->cameraY = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_height))) {
    options->cameraHeight = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_yaw))) {
    options->cameraYaw = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_pitch))) {
    options->cameraPitch = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_roll))) {
    options->cameraRoll = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_view_angle))) {
    options->viewAngle = NUM2DBL(val);
    if (!isfinite(options->viewAngle) ||
        options->viewAngle <= 0 || PI <= options->viewAngle) {
      rb_raise(rb_eArgError, "invalid :view_angle value");
    }
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_intersection_x))) {
    options->intersectionX = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_intersection_y))) {
    options->intersectionY = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_loop))) {
    options->isLoop = RTEST(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_blur))) {
    switch (TYPE(val)) {
    case T_DATA:
      options->blurType = BLUR_TYPE_COLOR;
      Color color;
      strb_RubyToColor(val, &color);
      options->blurColor = color;
      break;
    case T_SYMBOL:
      if (val == symbol_background) {
        options->blurType = BLUR_TYPE_BACKGROUND;
      } else {
        options->blurType = BLUR_TYPE_NONE;
      }
      break;
    default:
      rb_raise(rb_eTypeError,
               "wrong argument type %s (expected Color or Symbol)",
               rb_obj_classname(val));
      break;
    }
  }
}

static inline void
AssignRenderingTextureOptions(VALUE rbOptions,
                              RenderingTextureOptions* options)
{
  volatile VALUE val;
  Check_Type(rbOptions, T_HASH);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_rect))) {
    Rect* rect;
    Data_Get_Struct(val, Rect, rect);
    options->srcX      = rect->x;
    options->srcY      = rect->y;
    options->srcWidth  = rect->width;
    options->srcHeight = rect->height;
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_x))) {
    options->srcX = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_y))) {
    options->srcY = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_width))) {
    options->srcWidth = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_height))) {
    options->srcHeight = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale))) {
    Vector2* vec2;
    Data_Get_Struct(val, Vector2, vec2);
    options->scaleX = vec2->x;
    options->scaleY = vec2->y;
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_vec2))) {
    Vector2* vec2;
    Data_Get_Struct(val, Vector2, vec2);
    options->scaleX = vec2->x;
    options->scaleY = vec2->y;
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_x))) {
    options->scaleX = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_y))) {
    options->scaleY = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_angle))) {
    options->angle = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_center_x))) {
    options->centerX = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_center_y))) {
    options->centerY = NUM2INT(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_matrix))) {
    ASSIGN_MATRIX(options, val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_alpha))) {
    options->alpha = NUM2DBL(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_blend_type))) {
    strb_RubyToBlendType(val, &options->blendType);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone))) {
    Tone *tone;
    Data_Get_Struct(val, Tone, tone);
    options->tone = *tone;
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_color))) {
    Color *color;
    Data_Get_Struct(val, Color, color);
    options->color = *color;
  }
}

static inline bool
stratos_Texture_init(Texture* texture)
{
  texture->pixels       = NULL;
  texture->width        = 0;
  texture->height       = 0;
  texture->size         = 0;
  texture->clip_rect    = NULL;
  texture->stride       = 0;
  texture->palette_size = 0;
  texture->palette      = NULL;
  texture->indexes      = NULL;
  /* Ruby bindings */
  texture->rb_clip_rect = Qnil;
#ifdef HAVE_CAIRO
  texture->cr_surface   = NULL;
  texture->cr_context   = NULL;
  texture->rb_cr_surface = Qnil;
  texture->rb_cr_context = Qnil;
#endif
  return true;
}

/* ruby GC mark
 */
static void
Texture_mark(Texture* texture)
{
  if (texture) {
    if(!NIL_P(texture->rb_clip_rect)) {
      rb_gc_mark(texture->rb_clip_rect);
    }
#ifdef HAVE_CAIRO
    if(!NIL_P(texture->rb_cr_surface)) {
      rb_gc_mark(texture->rb_cr_surface);
    }
    if(!NIL_P(texture->rb_cr_context)) {
      rb_gc_mark(texture->rb_cr_context);
    }
#endif
  }
}

#ifdef HAVE_CAIRO
static void
stratos_Texture_alloc_cairo_surface(Texture* texture)
{
  texture->stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                  texture->width);
  texture->cr_surface =
    cairo_image_surface_create_for_data((unsigned char*)texture->pixels,
                                        CAIRO_FORMAT_ARGB32,
                                        texture->width, texture->height,
                                        texture->stride);
  cairo_surface_mark_dirty(texture->cr_surface);
}

static void
stratos_Texture_alloc_cairo_context(Texture* texture)
{
  texture->cr_context = cairo_create(texture->cr_surface);
}

static void
stratos_Texture_alloc_cairo(Texture* texture)
{
  stratos_Texture_alloc_cairo_surface(texture);
  stratos_Texture_alloc_cairo_context(texture);
}
#endif

static void
stratos_Texture_alloc_data(Texture* texture, uint32_t width, uint32_t height)
{
  texture->width  = width;
  texture->height = height;
  texture->size = texture->width * texture->height;
  assert(texture->size > 0);
  texture->pixels = ALLOC_N(Pixel, texture->size);
  MEMZERO(texture->pixels, Pixel, texture->size);
#ifdef HAVE_CAIRO
  stratos_Texture_alloc_cairo(texture);
#endif
}

static Texture*
strb_TextureMakeNew(const int32_t width, const int32_t height)
{
  Texture* texture = ALLOC(Texture);
  stratos_Texture_init(texture);
  stratos_Texture_alloc_data(texture, width, height);
  return texture;
}

#ifdef HAVE_CAIRO
static void
strb_TextureFreeCairoContext(Texture* texture)
{
  if(texture->cr_context) {
    //if(!NIL_P(texture->rb_cr_context)) {
    //  rb_funcall(texture->rb_cr_context, ID_destroy, 0);
    //}
    texture->rb_cr_context = Qnil;
    cairo_destroy(texture->cr_context);
    texture->cr_context = NULL;
  }
}

static void
strb_TextureFreeCairoSurface(Texture* texture)
{
  if(texture->cr_surface) {
    //if(!NIL_P(texture->rb_cr_surface)) {
    //  rb_funcall(texture->rb_cr_surface, ID_destroy, 0);
    //}
    texture->rb_cr_surface = Qnil;
    cairo_surface_destroy(texture->cr_surface);
    texture->cr_surface = NULL;
  }
}

static void
strb_TextureFreeCairo(Texture* texture)
{
  strb_TextureFreeCairoContext(texture);
  strb_TextureFreeCairoSurface(texture);
}

static void
strb_TextureAllocRbCairoContext(Texture* texture)
{
  texture->rb_cr_context = rb_cairo_context_to_ruby_object(texture->cr_context);
}

static void
strb_TextureAllocRbCairoSurface(Texture* texture)
{
  texture->rb_cr_surface = rb_cairo_surface_to_ruby_object(texture->cr_surface);
}

static VALUE
Texture_recycle_cr_context(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureFreeCairoContext(texture);
  stratos_Texture_alloc_cairo_context(texture);
  strb_TextureAllocRbCairoContext(texture);
  return texture->rb_cr_context;
}
#endif

static void
strb_TextureFreePixels(Texture* texture)
{
  if(texture) {
#ifdef HAVE_CAIRO
    strb_TextureFreeCairo(texture);
#endif
    if(texture->pixels) {
      free(texture->pixels);
      texture->pixels = NULL;
    }
  }
}

static void
strb_TextureFree(Texture* texture)
{
  if(texture) {
    texture->clip_rect = NULL;
    strb_TextureFreePixels(texture);
    free(texture);
  }
}

static VALUE
Texture_disposed(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  if (!texture) {
    return Qfalse;
  }
  return CBOOL2RVAL(!texture->pixels);
}

static VALUE
Texture_alloc(VALUE klass)
{
  Texture* texture = ALLOC(Texture);
  stratos_Texture_init(texture);
  // Texture_mark
  return Data_Wrap_Struct(klass, Texture_mark, strb_TextureFree, texture);
}

static VALUE
Texture_dispose(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  if(texture) {
    strb_TextureFreePixels(texture);
    return Qtrue;
  }
  return Qfalse;
}

static VALUE
Texture_setup_extra(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
#ifdef HAVE_CAIRO
  strb_TextureAllocRbCairoSurface(texture);
  strb_TextureAllocRbCairoContext(texture);
#endif
  return Qtrue;
}

static VALUE
Texture_initialize(VALUE self, VALUE rbWidth, VALUE rbHeight)
{
  Texture* texture;
  int32_t width  = NUM2INT(rbWidth);
  int32_t height = NUM2INT(rbHeight);
  Data_Get_Struct(self, Texture, texture);
  if (width <= 0) {
    rb_raise(rb_eArgError, "Given width for Texture must be greater than 0");
  }
  if (height <= 0) {
    rb_raise(rb_eArgError, "Given height for Texture must be greater than 0");
  }
  stratos_Texture_alloc_data(texture, width, height);
  Texture_setup_extra(self);
  return Qtrue;
}

static VALUE
Texture_initialize_copy(VALUE self, VALUE rbTexture)
{
  Texture* texture;
  const Texture* origTexture;
  Data_Get_Struct(self, Texture, texture);
  Data_Get_Struct(rbTexture, Texture, origTexture);
  stratos_Texture_alloc_data(texture, origTexture->width, origTexture->height);
  MEMCPY(texture->pixels, origTexture->pixels, Pixel, texture->size);
  Texture_setup_extra(self);
  return Qtrue;
}

static void
strb_Rect_get_clip_rect(const Texture* texture, Rect* cclip_rect)
{
  if(texture->clip_rect) {
    *cclip_rect = *texture->clip_rect;
    /* clip_rect patch */
    if(cclip_rect->x < 0) {
      cclip_rect->width += cclip_rect->x;
      cclip_rect->x = 0;
    }
    if(cclip_rect->y < 0) {
      cclip_rect->height += cclip_rect->y;
      cclip_rect->y = 0;
    }
    int32_t x2 = cclip_rect->x + cclip_rect->width;
    int32_t y2 = cclip_rect->y + cclip_rect->height;
    if(x2 > texture->width) {
      cclip_rect->width -= x2 - texture->width;
    }
    if(y2 > texture->height) {
      cclip_rect->height -= y2 - texture->height;
    }
  } else {
    *cclip_rect = (Rect){ 0, 0, texture->width, texture->height };
  }
}

static VALUE
strb_Texture_setup_clip_rect(VALUE rb_vTexture, VALUE rb_vRect)
{
  Texture* texture;
  Rect* rect = NULL;
  Data_Get_Struct(rb_vTexture, Texture, texture);
  if(!NIL_P(rb_vRect)) {
    Data_Get_Struct(rb_vRect, Rect, rect);
  }
  texture->clip_rect = rect;
  texture->rb_clip_rect = rb_vRect;
  return Qtrue;
}

static VALUE
Texture_clip_rect(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  if(!texture->clip_rect) {
    VALUE rb_vRect;
    Rect* rect;
    rb_vRect = rb_class_new_instance(0, (VALUE[]){}, rb_cRect);
    Data_Get_Struct(rb_vRect, Rect, rect);
    rect->x      = 0;
    rect->y      = 0;
    rect->width  = texture->width;
    rect->height = texture->height;
    strb_Texture_setup_clip_rect(self, rb_vRect);
  }
  return texture->rb_clip_rect;
}

static VALUE
Texture_clip_rect_eq(VALUE self, VALUE rb_vRect)
{
  if(rb_obj_is_kind_of(rb_vRect, rb_cRect) || NIL_P(rb_vRect)) {
    strb_Texture_setup_clip_rect(self, rb_vRect);
  } else {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected %s or nil)",
             rb_obj_classname(rb_vRect), rb_class2name(rb_cRect));
  }
  return Qnil;
}

static VALUE
Texture_aref(VALUE self, VALUE rbX, VALUE rbY)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    rb_raise(rb_eArgError, "index out of range: (%d, %d)", x, y);
  }
  const Color color = texture->pixels[x + y * texture->width].color;
  VALUE argv[] = {INT2FIX(color.red),
                  INT2FIX(color.green),
                  INT2FIX(color.blue),
                  INT2FIX(color.alpha)};
  return rb_class_new_instance(4, argv, rb_cColor);
}

static VALUE
Texture_aset(VALUE self, VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    return Qnil;
  }
  Color color;
  strb_RubyToColor(rbColor, &color);
  texture->pixels[x + y * texture->width].color = color;
  return Qnil;
}


/*
  StarRuby Texture Pixel Blend Functions
  */

//#define Lum(pixel) ((6969 * pixel->color.red + 23434 * pixel->color.green + 2365 * pixel->color.blue) / 32768)
#define Lum(pixel) ((pixel->color.red + pixel->color.green + pixel->color.blue) / 3)
#define Chroma(pixel) (MAX(MAX(pixel->color.red, pixel->color.green), pixel->color.blue) - MIN(MIN(pixel->color.red, pixel->color.green), pixel->color.blue))
#define ClipPlus(_c_, l, n) (l + (((_c_ - l) * l) / (l - n)))
#define ClipSub(_c_, l, n) (l + (((_c_ - l) * (1 - l)) / (n - l)))

#define pixel_prog inline
//#define pixel_prog
//#define pixel_prog void

static pixel_prog void
ClipColor(Pixel *pixel)
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

static pixel_prog void
SetLum(Pixel *pixel, uint8_t l)
{
  const uint8_t d = FAST_DIV255((l - Lum(pixel)) * pixel->color.alpha);
  pixel->color.red   = CLAMPU255(pixel->color.red + d);
  pixel->color.green = CLAMPU255(pixel->color.green + d);
  pixel->color.blue  = CLAMPU255(pixel->color.blue + d);
  //ClipColor(pixel);
}

static pixel_prog void
Pixel_blend_none(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    dst->color.red   = src->color.red;
    dst->color.green = src->color.green;
    dst->color.blue  = src->color.blue;
    dst->color.alpha = FAST_DIV255(src->color.alpha * alpha);
  } else {
    *dst = *src;
  }
}

static pixel_prog void
Pixel_blend_src_mask(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    dst->color.alpha = FAST_DIV255(src->color.alpha * alpha);
  } else {
    dst->color.alpha = src->color.alpha;
  }
}

static pixel_prog void
Pixel_blend_dst_mask(Pixel *dst, Pixel *src, uint8_t alpha)
{
  dst->color.red   = src->color.red;
  dst->color.green = src->color.green;
  dst->color.blue  = src->color.blue;
  if (alpha != 255) {
    dst->color.alpha = FAST_DIV255(dst->color.alpha * alpha);
  }
}

static pixel_prog void
Pixel_blend_clear(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    uint8_t beta = FAST_DIV255(src->color.alpha * alpha);
    dst->color.alpha = MAX(dst->color.alpha - beta, 0);
  } else {
    dst->color.alpha = MAX(dst->color.alpha - src->color.alpha, 0);
  }
}

static pixel_prog void
Pixel_blend_alpha(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    uint8_t beta = DIV255(alpha * src->color.alpha);
    if ((beta == 255) || (dst->color.alpha == 0)) {
      *dst = *src;
      dst->color.alpha = beta;
    } else if (beta) {
      dst->color.red   = ALPHA(src->color.red,   dst->color.red,   beta);
      dst->color.green = ALPHA(src->color.green, dst->color.green, beta);
      dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  beta);
      if (dst->color.alpha < beta) {
        dst->color.alpha = beta;
      }
    }
  } else {
    if ((src->color.alpha == 255) || (dst->color.alpha == 0)) {
      *dst = *src;
      dst->color.alpha = src->color.alpha;
    } else if (src->color.alpha) {
      dst->color.red   = ALPHA(src->color.red,   dst->color.red,   src->color.alpha);
      dst->color.green = ALPHA(src->color.green, dst->color.green, src->color.alpha);
      dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  src->color.alpha);
      if (dst->color.alpha < src->color.alpha) {
        dst->color.alpha = src->color.alpha;
      }
    }
  }
  //uint8_t beta = DIV255(alpha * src->color.alpha);
  //if (!beta) return;
  //if ((beta == 255) || (dst->color.alpha == 0)) {
  //  *dst = *src;
  //  dst->color.alpha = beta;
  //} else if (beta) {
  //  dst->color.red   = ALPHA(src->color.red,   dst->color.red,   beta);
  //  dst->color.green = ALPHA(src->color.green, dst->color.green, beta);
  //  dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  beta);
  //  if (dst->color.alpha < beta) {
  //    dst->color.alpha = beta;
  //  }
  //}
}

static pixel_prog void
Pixel_blend_add(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    uint8_t beta = FAST_DIV255(src->color.alpha * alpha);
    int iR = dst->color.red   + FAST_DIV255(src->color.red   * beta);
    int iG = dst->color.green + FAST_DIV255(src->color.green * beta);
    int iB = dst->color.blue  + FAST_DIV255(src->color.blue  * beta);
    dst->color.red   = MIN(iR, 255);
    dst->color.green = MIN(iG, 255);
    dst->color.blue  = MIN(iB, 255);
    if (dst->color.alpha < beta) {
      dst->color.alpha = beta;
    }
  } else {
    int iR = dst->color.red   + FAST_DIV255(src->color.red   * src->color.alpha);
    int iG = dst->color.green + FAST_DIV255(src->color.green * src->color.alpha);
    int iB = dst->color.blue  + FAST_DIV255(src->color.blue  * src->color.alpha);
    dst->color.red   = MIN(iR, 255);
    dst->color.green = MIN(iG, 255);
    dst->color.blue  = MIN(iB, 255);
    if (dst->color.alpha < src->color.alpha) {
      dst->color.alpha = src->color.alpha;
    }
  }
}

static pixel_prog void
Pixel_blend_sub(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    uint8_t beta = DIV255(src->color.alpha * alpha);
    int subR = dst->color.red   - FAST_DIV255(src->color.red   * beta);
    int subG = dst->color.green - FAST_DIV255(src->color.green * beta);
    int subB = dst->color.blue  - FAST_DIV255(src->color.blue  * beta);
    dst->color.red   = MAX(0, subR);
    dst->color.green = MAX(0, subG);
    dst->color.blue  = MAX(0, subB);
    if (dst->color.alpha < beta) {
      dst->color.alpha = beta;
    }
  } else {
    int subR = dst->color.red   - FAST_DIV255(src->color.red   * src->color.alpha);
    int subG = dst->color.green - FAST_DIV255(src->color.green * src->color.alpha);
    int subB = dst->color.blue  - FAST_DIV255(src->color.blue  * src->color.alpha);
    dst->color.red   = MAX(0, subR);
    dst->color.green = MAX(0, subG);
    dst->color.blue  = MAX(0, subB);
    if (dst->color.alpha < src->color.alpha) {
      dst->color.alpha = src->color.alpha;
    }
  }
}

static pixel_prog void
Pixel_blend_multiply(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    uint8_t beta = FAST_DIV255(src->color.alpha * alpha);
    dst->color.red   = FAST_DIV255(dst->color.red   * FAST_DIV255(src->color.red   * beta));
    dst->color.green = FAST_DIV255(dst->color.green * FAST_DIV255(src->color.green * beta));
    dst->color.blue  = FAST_DIV255(dst->color.blue  * FAST_DIV255(src->color.blue  * beta));
    if (dst->color.alpha < beta) {
      dst->color.alpha = beta;
    }
  } else {
    dst->color.red   = FAST_DIV255(dst->color.red   * FAST_DIV255(src->color.red   * src->color.alpha));
    dst->color.green = FAST_DIV255(dst->color.green * FAST_DIV255(src->color.green * src->color.alpha));
    dst->color.blue  = FAST_DIV255(dst->color.blue  * FAST_DIV255(src->color.blue  * src->color.alpha));
    if (dst->color.alpha < src->color.alpha) {
      dst->color.alpha = src->color.alpha;
    }
  }
}

static pixel_prog void
Pixel_blend_divide(Pixel *dst, Pixel *src, uint8_t alpha)
{
  uint8_t beta = FAST_DIV255(src->color.alpha * alpha);
  if(!beta) return;
  int red   = FAST_DIV255(src->color.red   * beta);
  int green = FAST_DIV255(src->color.green * beta);
  int blue  = FAST_DIV255(src->color.blue  * beta);
  red   = FAST_DIV255(dst->color.red   / MAX(red, 1));
  green = FAST_DIV255(dst->color.green / MAX(green, 1));
  blue  = FAST_DIV255(dst->color.blue  / MAX(blue, 1));
  dst->color.red   = CLAMPU255(red);
  dst->color.green = CLAMPU255(green);
  dst->color.blue  = CLAMPU255(blue);
  if (dst->color.alpha < beta) {
    dst->color.alpha = beta;
  }
}

// http://www.poynton.com/ColorFAQ.html
static pixel_prog void
Pixel_blend_tone(Pixel *dst, Tone *tone, uint8_t beta)
{
  if (tone->saturation < 255) {
    uint8_t l = Lum(dst);
    dst->color.red   = TONE_ALPHA(dst->color.red,   l, tone->saturation);
    dst->color.green = TONE_ALPHA(dst->color.green, l, tone->saturation);
    dst->color.blue  = TONE_ALPHA(dst->color.blue,  l, tone->saturation);
  }
  if(tone->red != 0) {
    if (0 < tone->red) {
      if (beta != 255) {
        dst->color.red = TONE_ALPHA(255, dst->color.red, TONE_DIV255(tone->red * beta));
      } else {
        dst->color.red = TONE_ALPHA(255, dst->color.red, tone->red);
      }
    } else {
      if (beta != 255) {
        dst->color.red = TONE_ALPHA(0,   dst->color.red, -TONE_DIV255(tone->red * beta));
      } else {
        dst->color.red = TONE_ALPHA(0,   dst->color.red, -tone->red);
      }
    }
  }
  if(tone->green != 0) {
    if (0 < tone->green) {
      if (beta != 255) {
        dst->color.green = TONE_ALPHA(255, dst->color.green, TONE_DIV255(tone->green * beta));
      } else {
        dst->color.green = TONE_ALPHA(255, dst->color.green, tone->green * beta);
      }
    } else {
      if (beta != 255) {
        dst->color.green = TONE_ALPHA(0,   dst->color.green, -TONE_DIV255(tone->green * beta));
      } else {
        dst->color.green = TONE_ALPHA(0,   dst->color.green, -tone->green);
      }
    }
  }
  if(tone->blue != 0) {
    if (0 < tone->blue) {
      if (beta != 255) {
        dst->color.blue = TONE_ALPHA(255, dst->color.blue, TONE_DIV255(tone->blue * beta));
      } else {
        dst->color.blue = TONE_ALPHA(255, dst->color.blue, tone->blue);
      }
    } else {
      if (beta != 255) {
        dst->color.blue = TONE_ALPHA(0,   dst->color.blue, -TONE_DIV255(tone->blue * beta));
      } else {
        dst->color.blue = TONE_ALPHA(0,   dst->color.blue, -tone->blue);
      }
    }
  }
}

static pixel_prog void
Pixel_blend_color(Pixel *dst, Pixel *src, uint8_t alpha)
{
  if (alpha != 255) {
    uint8_t beta = FAST_DIV255(src->color.alpha * alpha);
    dst->color.red   = ALPHA(src->color.red,   dst->color.red,   beta);
    dst->color.green = ALPHA(src->color.green, dst->color.green, beta);
    dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  beta);
  } else {
    dst->color.red   = ALPHA(src->color.red,   dst->color.red,   src->color.alpha);
    dst->color.green = ALPHA(src->color.green, dst->color.green, src->color.alpha);
    dst->color.blue  = ALPHA(src->color.blue,  dst->color.blue,  src->color.alpha);
  }
}

#define PIXEL_BLEND(blendType, dst_px, src_px, alpha) \
switch(blendType)                                     \
{                                                     \
  case BLEND_TYPE_NONE:                               \
    Pixel_blend_none(dst_px, src_px, alpha);          \
    break;                                            \
  case BLEND_TYPE_ALPHA:                              \
    Pixel_blend_alpha(dst_px, src_px, alpha);         \
    break;                                            \
  case BLEND_TYPE_ADD:                                \
    Pixel_blend_add(dst_px, src_px, alpha);           \
    break;                                            \
  case BLEND_TYPE_SUBTRACT:                           \
    Pixel_blend_sub(dst_px, src_px, alpha);           \
    break;                                            \
  case BLEND_TYPE_DST_MASK:                           \
    Pixel_blend_dst_mask(dst_px, src_px, alpha);      \
    break;                                            \
  case BLEND_TYPE_SRC_MASK:                           \
    Pixel_blend_src_mask(dst_px, src_px, alpha);      \
    break;                                            \
  case BLEND_TYPE_CLEAR:                              \
    Pixel_blend_clear(dst_px, src_px, alpha);         \
    break;                                            \
  case BLEND_TYPE_MULTIPLY:                           \
    Pixel_blend_multiply(dst_px, src_px, alpha);           \
    break;                                            \
  case BLEND_TYPE_DIVIDE:                             \
    Pixel_blend_divide(dst_px, src_px, alpha);           \
    break;                                            \
}

static VALUE
Texture_render_in_perspective(int argc, VALUE* argv, VALUE self)
{
  /*
   * Space Coordinates
   *
   *     y
   *     |
   *     o-- x
   *    /
   *   z
   *
   * srcTexture (ground)
   *   mapped on the x-z plane
   *
   *     o-- x
   *    /
   *   y
   *
   * dstTexture (screen)
   *   o: screenO
   *
   *     o-- x
   *     |
   *     y
   *
   */
  rb_check_frozen(self);
  volatile VALUE rbTexture, rbOptions;
  rb_scan_args(argc, argv, "11", &rbTexture, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  }
  strb_AssertObjIsKindOf(rbTexture, rb_cTexture);
  const Texture* srcTexture;
  Data_Get_Struct(rbTexture, Texture, srcTexture);
  strb_TextureCheckDisposed(srcTexture);
  const Texture* dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);
  strb_TextureCheckDisposed(dstTexture);

  if (srcTexture == dstTexture) {
    rb_raise(rb_eRuntimeError, "can't render self in perspective");
  }
  PerspectiveOptions options;
  AssignPerspectiveOptions(&options, rbOptions, dstTexture);
  if (!options.cameraHeight) {
    return self;
  }
  const int srcWidth  = srcTexture->width;
  const int srcHeight = srcTexture->height;
  const int dstWidth  = dstTexture->width;
  const int dstHeight = dstTexture->height;
  const double cosYaw   = cos(options.cameraYaw);
  const double sinYaw   = sin(options.cameraYaw);
  const double cosPitch = cos(options.cameraPitch);
  const double sinPitch = sin(options.cameraPitch);
  const double cosRoll  = cos(options.cameraRoll);
  const double sinRoll  = sin(options.cameraRoll);
  const Vector3 screenDX = {
    cosRoll * cosYaw + sinRoll * sinPitch * sinYaw,
    sinRoll * -cosPitch,
    cosRoll * sinYaw - sinRoll * sinPitch * cosYaw,
  };
  const Vector3 screenDY = {
    -sinRoll * cosYaw + cosRoll * sinPitch * sinYaw,
    cosRoll * -cosPitch,
    -sinRoll * sinYaw - cosRoll * sinPitch * cosYaw,
  };
  const double distance = dstWidth / (2 * (tan(options.viewAngle / 2)));
  const Vector3 intersection = {
    distance * (cosPitch * sinYaw),
    distance * sinPitch + options.cameraHeight,
    distance * (-cosPitch * cosYaw),
  };
  const Vector3 screenO = {
    intersection.x
    - options.intersectionX * screenDX.x
    - options.intersectionY * screenDY.x,
    intersection.y
    - options.intersectionX * screenDX.y
    - options.intersectionY * screenDY.y,
    intersection.z
    - options.intersectionX * screenDX.z
    - options.intersectionY * screenDY.z
  };
  const int cameraHeight = (int)options.cameraHeight;
  const Pixel* src = srcTexture->pixels;
  Pixel* dst = dstTexture->pixels;
  Vector3 screenP;
  for (int j = 0; j < dstHeight; ++j) {
    screenP.x = screenO.x + j * screenDY.x;
    screenP.y = screenO.y + j * screenDY.y;
    screenP.z = screenO.z + j * screenDY.z;
    LOOP({
        if (cameraHeight != screenP.y &&
            ((0 < cameraHeight && screenP.y < cameraHeight) ||
             (cameraHeight < 0 && cameraHeight < screenP.y))) {
          const double scale = cameraHeight / (cameraHeight - screenP.y);
          int srcX = (int)((screenP.x) * scale + options.cameraX);
          int srcZ = (int)((screenP.z) * scale + options.cameraY);
          if (options.isLoop) {
            srcX %= srcWidth;
            if (srcX < 0) {
              srcX += srcWidth;
            }
            srcZ %= srcHeight;
            if (srcZ < 0) {
              srcZ += srcHeight;
            }
          }
          if (options.isLoop ||
              (0 <= srcX && srcX < srcWidth && 0 <= srcZ && srcZ < srcHeight)) {
            const Color* srcColor = &(src[srcX + srcZ * srcWidth].color);
            if (options.blurType == BLUR_TYPE_NONE || scale <= 1) {
              RENDER_PIXEL(dst->color, (*srcColor));
            } else {
              const int rate = (int)(255 * (1 / scale));
              if (options.blurType == BLUR_TYPE_BACKGROUND) {
                Color c;
                c.red   = srcColor->red;
                c.green = srcColor->green;
                c.blue  = srcColor->blue;
                c.alpha = DIV255(srcColor->alpha * rate);
                RENDER_PIXEL(dst->color, c);
              } else {
                Color c;
                c.red   = ALPHA(srcColor->red,   options.blurColor.red,   rate);
                c.green = ALPHA(srcColor->green, options.blurColor.green, rate);
                c.blue  = ALPHA(srcColor->blue,  options.blurColor.blue,  rate);
                c.alpha = ALPHA(srcColor->alpha, options.blurColor.alpha, rate);
                RENDER_PIXEL(dst->color, c);
              }
            }
          }
        }
        ++dst;
        screenP.x += screenDX.x;
        screenP.y += screenDX.y;
        screenP.z += screenDX.z;
      }, dstWidth);
  }
  return self;
}

static VALUE
Texture_render_line(VALUE self,
                    VALUE rbX1, VALUE rbY1, VALUE rbX2, VALUE rbY2,
                    VALUE rbColor)
{
  rb_check_frozen(self);
  const int x1 = NUM2INT(rbX1);
  const int y1 = NUM2INT(rbY1);
  const int x2 = NUM2INT(rbX2);
  const int y2 = NUM2INT(rbY2);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  Color color;
  strb_RubyToColor(rbColor, &color);
  int x = x1;
  int y = y1;
  const int dx = abs(x2 - x1);
  const int dy = abs(y2 - y1);
  const int signX = (x1 <= x2) ? 1 : -1;
  const int signY = (y1 <= y2) ? 1 : -1;
  if (dy <= dx) {
    int e = dx;
    const int eLimit = dx << 1;
    for (int i = 0; i <= dx; ++i) {
      if (0 <= x && x < texture->width && 0 <= y && y < texture->height) {
        Pixel* pixel = &(texture->pixels[x + y * texture->width]);
        RENDER_PIXEL(pixel->color, color);
      }
      x += signX;
      e += dy << 1;
      if (eLimit <= e) {
        e -= eLimit;
        y += signY;
      }
    }
  } else {
    int e = dy;
    const int eLimit = dy << 1;
    for (int i = 0; i <= dy; ++i) {
      if (0 <= x && x < texture->width && 0 <= y && y < texture->height) {
        Pixel* pixel = &(texture->pixels[x + y * texture->width]);
        RENDER_PIXEL(pixel->color, color);
      }
      y += signY;
      e += dx << 1;
      if (eLimit <= e) {
        e -= eLimit;
        x += signX;
      }
    }
  }
  return self;
}

static VALUE
Texture_render_pixel(VALUE self, VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    return self;
  }
  Color color;
  strb_RubyToColor(rbColor, &color);
  Pixel* pixel = &(texture->pixels[x + y * texture->width]);
  RENDER_PIXEL(pixel->color, color);
  return self;
}


/*
  StarRuby Texture
    render-texture
  vr 1.0.0
  */
//#define PARALLEL_RENDER

static void
strb_TextureRender(const Texture* src_texture, const Texture* dst_texture,
                   int32_t srcX, int32_t srcY,
                   int32_t srcWidth, int32_t srcHeight,
                   int32_t dstX, int32_t dstY,
                   const uint8_t alpha, const Tone *tone, const Color *color,
                   const BlendType blendType)
{
  Rect cclip_rect;
  //if(dstX < 0) {
  //  srcX += -dstX;
  //  srcWidth += dstX;
  //  dstX = 0;
  //}
  //if(dstY < 0) {
  //  srcY += -dstY;
  //  srcHeight += dstY;
  //  dstY = 0;
  //}
  if(srcWidth < 0 || srcHeight < 0 ||
    dstX > dst_texture->width || dstY > dst_texture->height || alpha == 0)
  {
    return;
  }
  if (!ModifyRectInTexture(src_texture,
                           &(srcX), &(srcY), &(srcWidth), &(srcHeight))) {
    return;
  }
  strb_Rect_get_clip_rect(dst_texture, &cclip_rect);

  const int width  = MIN(srcWidth,  dst_texture->width - dstX);
  const int height = MIN(srcHeight, dst_texture->height - dstY);

#ifndef PARALLEL_RENDER
  Pixel* src_px = &(src_texture->pixels[srcX + srcY * src_texture->width]);
  Pixel* dst_px = &(dst_texture->pixels[dstX + dstY * dst_texture->width]);

  const int32_t srcPadding = src_texture->width - width;
  const int32_t dstPadding = dst_texture->width - width;
#endif

  const bool use_tone  = is_valid_tone(tone);
  const bool use_color = is_valid_color(color);

  Pixel* px_color = (Pixel*)color;

  int32_t iy = dstY;
  for(int32_t j = 0; j < height; ++iy, ++j, src_px += srcPadding, dst_px += dstPadding) {
    int32_t ix = dstX;
    if(is_y_in_rect(cclip_rect, iy)) {
      for(int32_t k = 0; k < width; ++ix, ++k, ++src_px, ++dst_px) {
        if(is_x_in_rect(cclip_rect, ix)) {
          Pixel pixel = *src_px;
          if(use_tone) {
            Pixel_blend_tone(&pixel, tone, alpha);
          }
          if(use_color) {
            Pixel_blend_color(&pixel, px_color, alpha);
          }
          PIXEL_BLEND(blendType, dst_px, &pixel, alpha);
        }
      }
    } else {
      src_px += width;
      dst_px += width;
    }
  }
}

static void
strb_TextureRenderWithOptions(const Texture* src_texture, const Texture* dst_texture,
                              int srcX, int srcY, int srcWidth, int srcHeight,
                              int dstX, int dstY,
                              const RenderingTextureOptions* options)
{
  Rect cclip_rect;
  strb_Rect_get_clip_rect(dst_texture, &cclip_rect);
  const double angle  = options->angle;
  const int centerX   = options->centerX;
  const int centerY   = options->centerY;
  const double scaleX = options->scaleX;
  const double scaleY = options->scaleY;
  AffineMatrix mat;
  mat.a  = options->matrix.a;
  mat.b  = options->matrix.b;
  mat.c  = options->matrix.c;
  mat.d  = options->matrix.d;
  mat.tx = options->matrix.a * (options->matrix.tx - centerX)
    + options->matrix.b * (options->matrix.ty - centerY);
  mat.ty = options->matrix.c * (options->matrix.tx - centerX)
    + options->matrix.d * (options->matrix.ty - centerY);
  if (scaleX != 1) {
    mat.a  *= scaleX;
    mat.b  *= scaleX;
    mat.tx *= scaleX;
  }
  if (scaleY != 1) {
    mat.c  *= scaleY;
    mat.d  *= scaleY;
    mat.ty *= scaleY;
  }
  if (angle != 0) {
    const double a  = mat.a;
    const double b  = mat.b;
    const double c  = mat.c;
    const double d  = mat.d;
    const double tx = mat.tx;
    const double ty = mat.ty;
    const double cosAngle = cos(angle);
    const double sinAngle = sin(angle);
    mat.a  = cosAngle * a  - sinAngle * c;
    mat.b  = cosAngle * b  - sinAngle * d;
    mat.c  = sinAngle * a  + cosAngle * c;
    mat.d  = sinAngle * b  + cosAngle * d;
    mat.tx = cosAngle * tx - sinAngle * ty;
    mat.ty = sinAngle * tx + cosAngle * ty;
  }
  mat.tx += centerX + dstX;
  mat.ty += centerY + dstY;
  const double det = mat.a * mat.d - mat.b * mat.c;
  if (det == 0) {
    return;
  }
  const double dstX00 = mat.tx;
  const double dstY00 = mat.ty;
  const double dstX01 = mat.b * srcHeight + mat.tx;
  const double dstY01 = mat.d * srcHeight + mat.ty;
  const double dstX10 = mat.a * srcWidth  + mat.tx;
  const double dstY10 = mat.c * srcWidth  + mat.ty;
  const double dstX11 = mat.a * srcWidth  + mat.b * srcHeight + mat.tx;
  const double dstY11 = mat.c * srcWidth  + mat.d * srcHeight + mat.ty;
  double dstX0 = MIN(MIN(MIN(dstX00, dstX01), dstX10), dstX11);
  double dstY0 = MIN(MIN(MIN(dstY00, dstY01), dstY10), dstY11);
  double dstX1 = MAX(MAX(MAX(dstX00, dstX01), dstX10), dstX11);
  double dstY1 = MAX(MAX(MAX(dstY00, dstY01), dstY10), dstY11);

  if (dst_texture->width <= dstX0 || dst_texture->height <= dstY0 ||
      dstX1 < 0 || dstY1 < 0) {
    return;
  }
  AffineMatrix matInv = {
    .a = mat.d  / det,
    .b = -mat.b / det,
    .c = -mat.c / det,
    .d = mat.a  / det,
  };
  matInv.tx = -(matInv.a * mat.tx + matInv.b * mat.ty);
  matInv.ty = -(matInv.c * mat.tx + matInv.d * mat.ty);
  double srcOX = matInv.a * (dstX0 + 0.5) + matInv.b * (dstY0 + 0.5)
    + matInv.tx + srcX;
  double srcOY = matInv.c * (dstX0 + 0.5) + matInv.d * (dstY0 + 0.5)
    + matInv.ty + srcY;
  double srcDXX = matInv.a;
  double srcDXY = matInv.c;
  double srcDYX = matInv.b;
  double srcDYY = matInv.d;

  if (dstX0 < 0) {
    srcOX -= dstX0 * srcDXX;
    srcOY -= dstX0 * srcDXY;
    dstX0 = 0;
  }
  if (dstY0 < 0) {
    srcOX -= dstY0 * srcDYX;
    srcOY -= dstY0 * srcDYY;
    dstY0 = 0;
  }
  const int dstX0Int = (int)dstX0;
  const int dstY0Int = (int)dstY0;
  const int dstWidth  = MIN(dst_texture->width,  (int)dstX1) - dstX0Int;
  const int dstHeight = MIN(dst_texture->height, (int)dstY1) - dstY0Int;

  const int_fast32_t srcOX16  = (int_fast32_t)(srcOX  * (1 << 16));
  const int_fast32_t srcOY16  = (int_fast32_t)(srcOY  * (1 << 16));
  const int_fast32_t srcDXX16 = (int_fast32_t)(srcDXX * (1 << 16));
  const int_fast32_t srcDXY16 = (int_fast32_t)(srcDXY * (1 << 16));
  const int_fast32_t srcDYX16 = (int_fast32_t)(srcDYX * (1 << 16));
  const int_fast32_t srcDYY16 = (int_fast32_t)(srcDYY * (1 << 16));

  Texture* clonedTexture = NULL;
  if (src_texture == dst_texture) {
    clonedTexture = ALLOC(Texture);
    clonedTexture->pixels      = NULL;
    clonedTexture->width  = dst_texture->width;
    clonedTexture->height = dst_texture->height;
    const int length = dst_texture->width * dst_texture->height;
    clonedTexture->pixels = ALLOC_N(Pixel, length);
    MEMCPY(clonedTexture->pixels, dst_texture->pixels, Pixel, length);
    src_texture = clonedTexture;
  }

  const int srcX2 = srcX + srcWidth;
  const int srcY2 = srcY + srcHeight;
  const uint8_t alpha       = options->alpha;
  const BlendType blendType = options->blendType;
  //const int saturation      = options->tone.saturation;
  //const int toneRed         = options->tone.red;
  //const int toneGreen       = options->tone.green;
  //const int toneBlue        = options->tone.blue;

  for (int j = 0; j < dstHeight; ++j) {
    int_fast32_t srcI16 = srcOX16 + j * srcDYX16;
    int_fast32_t srcJ16 = srcOY16 + j * srcDYY16;
    //if(is_y_in_rect(cclip_rect, dstY0Int + j)) {
      Pixel* dst = &(dst_texture->pixels[dstX0Int + (dstY0Int + j) * dst_texture->width]);
      for (int i = 0; i < dstWidth; ++i, ++dst, srcI16 += srcDXX16, srcJ16 += srcDXY16) {
        const int_fast32_t srcI = srcI16 >> 16;
        const int_fast32_t srcJ = srcJ16 >> 16;
        //if(is_x_in_rect(cclip_rect, srcI)) {
          if (srcX <= srcI && srcI < srcX2 && srcY <= srcJ && srcJ < srcY2) {
            Color srcColor = src_texture->pixels[srcI + srcJ * src_texture->width].color;
            if (blendType == BLEND_TYPE_SRC_MASK) {
              dst->color.alpha = srcColor.alpha;
            } else {
              Pixel_blend_tone((Pixel*)(&srcColor), &options->tone, alpha);
              PIXEL_BLEND(blendType, dst, (Pixel*)(&srcColor), alpha);
            }
          } else if ((srcI < srcX && srcDXX <= 0) ||
                     (srcX2 <= srcI && 0 <= srcDXX) ||
                     (srcJ < srcY && srcDXY <= 0) ||
                     (srcY2 <= srcJ && 0 <= srcDXY)) {
            break;
          }
        //}
      }
    //}
  }
  if (clonedTexture) {
    strb_TextureFree(clonedTexture);
    clonedTexture = NULL;
  }
}

static VALUE
Texture_render_texture(int argc, VALUE* argv, VALUE self)
{
  AffineMatrix* matrix;
  RenderingTextureOptions options;
  Texture* dst_texture;
  Texture* src_texture;
  VALUE rbTexture, rbX, rbY, rbOptions;

  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, dst_texture);
  strb_TextureCheckDisposed(dst_texture);

  if (3 <= argc && argc <= 4) {
    rbTexture = argv[0];
    rbX       = argv[1];
    rbY       = argv[2];
    rbOptions = (argc == 4) ? argv[3] : Qnil;
  } else {
    rb_scan_args(argc, argv, "31", &rbTexture, &rbX, &rbY, &rbOptions);
  }

  strb_AssertObjIsKindOf(rbTexture, rb_cTexture);
  Data_Get_Struct(rbTexture, Texture, src_texture);
  strb_TextureCheckDisposed(src_texture);

  options = (RenderingTextureOptions){
    .srcX         = 0,
    .srcY         = 0,
    .srcWidth     = src_texture->width,
    .srcHeight    = src_texture->height,
    .scaleX       = 1.0,
    .scaleY       = 1.0,
    .angle        = 0,
    .centerX      = 0,
    .centerY      = 0,
    .matrix       = (AffineMatrix) {
      .a  = 1,
      .b  = 0,
      .c  = 0,
      .d  = 1,
      .tx = 0,
      .ty = 0,
    },
    .alpha        = 255,
    .blendType    = BLEND_TYPE_ALPHA,
    .tone         = (Tone) {
      .red = 0, .green = 0, .blue = 0, .saturation = 255
    },
    .color        = (Color) {
      .red = 0, .green = 0, .blue = 0, .alpha = 0
    }
  };

  if (!NIL_P(rbOptions) && (TYPE(rbOptions) != T_HASH)) {
    if (rb_obj_respond_to(rbOptions, ID_to_h, Qfalse)) {
      rbOptions = rb_funcall(rbOptions, ID_to_h, 0);
    } else {
      rb_raise(rb_eTypeError, "cannot convert argument type %s into Hash",
               rb_obj_classname(rbOptions));
    }
  }

  if (TYPE(rbOptions) == T_HASH) {
    AssignRenderingTextureOptions(rbOptions, &options);
  } else if (!NIL_P(rbOptions)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Hash)",
             rb_obj_classname(rbOptions));
  }

  matrix = &(options.matrix);

  if (src_texture != dst_texture &&
      (matrix->a == 1 && matrix->b == 0 && matrix->c == 0 && matrix->d == 1) &&
      (options.scaleX == 1.0 && options.scaleY == 1.0 && options.angle == 0)) {
    strb_TextureRender(src_texture, dst_texture,
                      options.srcX, options.srcY, options.srcWidth, options.srcHeight,
                      NUM2INT(rbX), NUM2INT(rbY),
                      options.alpha,
                      &(options.tone),
                      &(options.color),
                      options.blendType);
  } else {
    if (!ModifyRectInTexture(src_texture,
                           &(options.srcX), &(options.srcY), &(options.srcWidth), &(options.srcHeight))) {
      return self;
    }
    strb_TextureRenderWithOptions(src_texture, dst_texture,
                                  options.srcX, options.srcY, options.srcWidth, options.srcHeight,
                                  NUM2INT(rbX), NUM2INT(rbY),
                                  &options);
  }
  return self;
}


static SDL_PixelFormat px_format = {
  .palette = NULL, .BitsPerPixel = 32, .BytesPerPixel = 4,
  .Rmask = 0x00ff0000, .Gmask = 0x0000ff00,
  .Bmask = 0x000000ff, .Amask = 0xff000000,
  .colorkey = 0, .alpha = 255
};

static VALUE
Texture_render_text(int argc, VALUE* argv, VALUE self)
{
  bool anti_alias;
  char* text;
  Color color;
  Font* font;
  Pixel* dst;
  Pixel* src;
  BlendType blend_type;
  //SDL_PixelFormat px_format;
  SDL_Surface* text_surface;
  SDL_Surface* text_surface_raw;
  Texture *dst_texture;
  Texture* text_texture;
  //
  VALUE rb_vAntiAlias;
  VALUE rb_vBlendType;
  VALUE rb_vColor;
  VALUE rb_vFont;
  VALUE rb_vSize;
  VALUE rb_vText;
  VALUE rb_vTextTexture;
  VALUE rb_vX;
  VALUE rb_vY;

  rb_check_frozen(self); /* Is the texture frozen? */
  rb_scan_args(argc, argv, "61",
               &rb_vText, &rb_vX, &rb_vY, &rb_vFont, &rb_vColor, &rb_vBlendType, &rb_vAntiAlias);
  /* Is this a String? */
  if(!(TYPE(rb_vText) == T_STRING)) {
    rb_raise(rb_eArgError, "expected type String but recieved %s",
             rb_obj_classname(rb_vText));
  }
  if (!(RSTRING_LEN(rb_vText))) {
    return self;
  }

  strb_AssertObjIsKindOf(rb_vFont, rb_cFont); /* Are we working with a Font? */

  anti_alias = RTEST(rb_vAntiAlias); /* anti-aliasing? */
  text = StringValueCStr(rb_vText);
  rb_vSize = rb_funcall(rb_vFont, ID_get_size, 1, rb_vText);
  rb_vTextTexture = rb_class_new_instance(2, RARRAY_PTR(rb_vSize), rb_cTexture);

  Data_Get_Struct(rb_vFont, Font, font);
  Data_Get_Struct(rb_vTextTexture, Texture, text_texture);
  strb_RubyToColor(rb_vColor, &color);
  strb_RubyToBlendType(rb_vBlendType, &blend_type);

  /* Draw Text to a raw SDL Surface */
  if (anti_alias) {
    text_surface_raw =
      TTF_RenderUTF8_Shaded(font->sdlFont, text,
                            (SDL_Color){255, 255, 255, 255},
                            (SDL_Color){0, 0, 0, 0});
  } else {
    text_surface_raw =
      TTF_RenderUTF8_Solid(font->sdlFont, text,
                           (SDL_Color){255, 255, 255, 255});
  }

  /* Somehow, we failed to render the text properly! */
  if (!text_surface_raw) {
    rb_raise_sdl_ttf_error();
  }


  text_surface = SDL_ConvertSurface(text_surface_raw, &px_format, SDL_SWSURFACE);
  SDL_FreeSurface(text_surface_raw); text_surface_raw = NULL;
  /* and somehow we failed to convert the text_surface*/
  if (!text_surface) {
    rb_raise_sdl_error();
  }

  SDL_LockSurface(text_surface);
  src = (Pixel*)(text_surface->pixels);
  dst = text_texture->pixels;
  for(uint32_t i = 0; i < text_texture->size; ++i, ++src, ++dst) {
    if (src->value) {
      dst->color = color;
      if (color.alpha == 255) {
        dst->color.alpha = src->color.red;
      } else {
        dst->color.alpha = DIV255(src->color.red * color.alpha);
      }
    }
  }
  SDL_UnlockSurface(text_surface);
  SDL_FreeSurface(text_surface);
  Data_Get_Struct(self, Texture, dst_texture);
  strb_TextureRender(text_texture, dst_texture,
                     0, 0, text_texture->width, text_texture->height,
                     NUM2INT(rb_vX), NUM2INT(rb_vY), 255, NULL, NULL, blend_type);
  Texture_dispose(rb_vTextTexture);
  text_surface = NULL;
  return self;
}


static VALUE
Texture_clear(VALUE self)
{
  int32_t length;
  Pixel* pixels;
  Texture* texture;
  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  length = texture->width * texture->height;
  pixels = texture->pixels;
  for (int32_t i = 0; i < length; ++i, ++pixels) {
    pixels->value = 0;
  }
  //MEMZERO(texture->pixels, Pixel, length);
  return self;
}

static VALUE
Texture_clear_rect(int argc, VALUE* argv, VALUE self)
{
  int32_t padding;
  Pixel* pixels;
  Rect rect;
  Texture* texture;

  rb_check_frozen(self);

  if (argc == 1)
  {
    strb_RubyToRect(argv[0], &(rect));
  } else if (argc == 4)  {
    rect.x      = NUM2INT(argv[0]);
    rect.y      = NUM2INT(argv[1]);
    rect.width  = NUM2INT(argv[2]);
    rect.height = NUM2INT(argv[3]);
  } else {
    rb_raise(rb_eArgError, "expected 1 or 4 arguments but recieved %d", argc);
  }
  Data_Get_Struct(self, Texture, texture);
  if (!ModifyRectInTexture_Rect(texture, &(rect))) {
    return self;
  }
  strb_TextureCheckDisposed(texture);

  pixels = &(texture->pixels[rect.x + rect.y * texture->width]);
  padding = texture->width - rect.width;
  for (int32_t y = 0; y < rect.height; ++y, pixels += padding) {
    for (int32_t x = 0; x < rect.width; ++x, ++pixels) {
      pixels->value = 0;
    }
  }
  return self;
}

static VALUE
Texture_width(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  return INT2NUM(texture->width);
}

static VALUE
Texture_height(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  return INT2NUM(texture->height);
}

static VALUE
Texture_size(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  volatile VALUE rbSize =
    rb_assoc_new(INT2NUM(texture->width), INT2NUM(texture->height));
  OBJ_FREEZE(rbSize);
  return rbSize;
}

static VALUE
Texture_rect(VALUE self)
{
  Texture *texture;
  Data_Get_Struct(self, Texture, texture);
  VALUE rbArgv[4] = { INT2NUM(0), INT2NUM(0),
                      INT2NUM(texture->width), INT2NUM(texture->height) };
  return rb_class_new_instance(4, rbArgv, rb_cRect);
}


static VALUE
Texture_dump(VALUE self, VALUE rbFormat)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  const char* format = StringValuePtr(rbFormat);
  const int formatLength = RSTRING_LEN(rbFormat);
  const int pixelLength = texture->width * texture->height;
  volatile VALUE rbResult = rb_str_new(NULL, pixelLength * formatLength);
  uint8_t* strPtr = (uint8_t*)RSTRING_PTR(rbResult);
  const Pixel* pixels = texture->pixels;
  for (int i = 0; i < pixelLength; ++i, ++pixels) {
    for (int j = 0; j < formatLength; ++j, ++strPtr) {
      switch (format[j]) {
      case 'r': *strPtr = pixels->color.red;   break;
      case 'g': *strPtr = pixels->color.green; break;
      case 'b': *strPtr = pixels->color.blue;  break;
      case 'a': *strPtr = pixels->color.alpha; break;
      }
    }
  }
  return rbResult;
}

static VALUE
Texture_undump(VALUE self, VALUE rbData, VALUE rbFormat)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  const char* format = StringValuePtr(rbFormat);
  const int formatLength = RSTRING_LEN(rbFormat);
  const int pixelLength = texture->width * texture->height;
  Check_Type(rbData, T_STRING);
  if (pixelLength * formatLength != RSTRING_LEN(rbData)) {
    rb_raise(rb_eArgError, "invalid data size: %d expected but was %ld",
             pixelLength * formatLength, RSTRING_LEN(rbData));
  }
  const uint8_t* data = (uint8_t*)RSTRING_PTR(rbData);
  Pixel* pixels = texture->pixels;
  for (int i = 0; i < pixelLength; ++i, ++pixels) {
    for (int j = 0; j < formatLength; ++j, ++data) {
      switch (format[j]) {
      case 'r': pixels->color.red   = *data; break;
      case 'g': pixels->color.green = *data; break;
      case 'b': pixels->color.blue  = *data; break;
      case 'a': pixels->color.alpha = *data; break;
      }
    }
  }
  return Qnil;
}


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
  rb_check_frozen(self);
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

  for(uint32_t y = 1; y < texture->height; ++y) {
    for(uint32_t x = 1; x < texture->width; ++x) {
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

  for(uint32_t y = texture->height - 2; y > 0; --y) {
    for(uint32_t x = texture->width - 2; x > 0; --x) {
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


static void
strb_TextureRotate(Texture *dst_texture, Texture *src_texture,
                   RotateType rotate_direction)
{
  const int32_t width  = src_texture->width;
  const int32_t height = src_texture->height;
  Pixel* dst_data = dst_texture->pixels;
  Pixel* src_data = src_texture->pixels;

  if (rotate_direction == ROTATE_NONE) {
    /* Nothing here folks, please move on */
  } else if (rotate_direction == ROTATE_HORZ) {
    for (int32_t y = 0; y < height; ++y) {
      int32_t row = y * width;
      for (int32_t x = 0; x < width; ++x) {
        dst_data[(width - 1) - x + row] = src_data[x + row];
      }
    }
  } else if (rotate_direction == ROTATE_VERT) {
    for (int32_t y = 0; y < height; ++y) {
      int32_t row = y * width;
      int32_t dst_row = (height - 1 - y) * width;
      for (int32_t x = 0; x < width; ++x) {
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

    for (int32_t y = 0; y < height; ++y) {
      int32_t row = y * width;
      for (int32_t x = 0; x < width; ++x) {
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

    for (int32_t y = 0; y < height; ++y) {
      int32_t row = y * width;
      for (int32_t x = 0; x < width; ++x) {
        dst_data[y + ((width - 1 - x) * height)] = src_data[x + row];
      }
    }
  /* 180* Rotation
      Width and Height remain the same
      (x) becomes (-y)
      (y) becomes (-x)
   */
  } else if (rotate_direction == ROTATE_180) {
    for (int32_t y = 0; y < height; ++y) {
      int32_t row = y * width;
      int32_t dst_row = (height - 1 - y) * width;
      for (int32_t x = 0; x < width; ++x) {
        dst_data[(width - 1) - x + dst_row] = src_data[x + row];
      }
    }
  }
}

static VALUE
Texture_rotate(VALUE self, VALUE rbRotateType)
{
  VALUE rbSrcTexture = rb_obj_dup(self);
  RotateType rotatetype = (RotateType)NUM2INT(rbRotateType);
  Texture* src_texture;
  Texture* dst_texture;

  if (rotatetype != ROTATE_NONE &&
      rotatetype != ROTATE_CW &&
      rotatetype != ROTATE_CCW &&
      rotatetype != ROTATE_180 &&
      rotatetype != ROTATE_HORZ &&
      rotatetype != ROTATE_VERT) {
    rb_raise(rb_eArgError, "invalid RotateType %d", rotatetype);
  }

  rb_check_frozen(self);
  Data_Get_Struct(rbSrcTexture, Texture, src_texture);
  Data_Get_Struct(self, Texture, dst_texture);
  strb_TextureCheckDisposed(src_texture);
  strb_TextureCheckDisposed(dst_texture);
  strb_TextureRotate(dst_texture, src_texture, rotatetype);
  Texture_dispose(rbSrcTexture);
  return self;
}


/* */
static Texture*
strb_TextureCrop(Texture *src_texture, Rect *crop_rect)
{
  Texture *dst_texture;
  int32_t x      = crop_rect->x;
  int32_t y      = crop_rect->y;
  int32_t width  = crop_rect->width;
  int32_t height = crop_rect->height;
  if (!ModifyRectInTexture(src_texture, &(x), &(y), &(width), &(height))) {
    return NULL;
  }
  dst_texture = strb_TextureMakeNew(width, height);
  strb_TextureRender(src_texture, dst_texture, x, y, width, height,
                     0, 0, 255, NULL, NULL, BLEND_TYPE_NONE);
  return dst_texture;
}

static VALUE
Texture_crop(int argc, VALUE *argv, VALUE self)
{
  Rect rect;
  Texture* dst_texture;
  Texture* src_texture;
  VALUE rbTexture;

  rb_check_frozen(self);

  if (argc == 1) {
    VALUE rbObj = argv[0];
    if (rb_obj_is_kind_of(rbObj, rb_cRect)) {
      strb_RubyToRect(rbObj, &(rect));
    } else {
      rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)",
               rb_obj_classname(rbObj), rb_class2name(rb_cRect));
    }
  } else if (argc == 4) {
    rect.x      = NUM2INT(argv[0]);
    rect.y      = NUM2INT(argv[1]);
    rect.width  = NUM2INT(argv[2]);
    rect.height = NUM2INT(argv[3]);
  } else {
    rb_raise(rb_eArgError, "wrong number of arguments (expected 1 or 4)");
  }
  Data_Get_Struct(self, Texture, src_texture);
  strb_TextureCheckDisposed(src_texture);
  dst_texture = strb_TextureCrop(src_texture, &(rect));
  if (dst_texture) {
    /* HACK */
    rbTexture = rb_obj_alloc(rb_cTexture);
    strb_TextureFree(DATA_PTR(rbTexture));
    RDATA(rbTexture)->data = dst_texture;
    return rbTexture;
  } else {
    return Qnil;
  }
}

static void
strb_TextureRecolor(Texture* src_texture, Rect* rect,
                    const Color* src_color, Color* rep_color)
{
  int32_t dst_x      = rect->x;
  int32_t dst_y      = rect->y;
  uint32_t dst_width  = rect->width;
  uint32_t dst_height = rect->height;
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
  for (uint32_t y = 0; y < dst_height; ++y, pixels += padding) {
    for (uint32_t x = 0; x < dst_width; ++x, ++pixels) {
      if (TexturePixelRGBAMatch(pixels, src_pixel)) {
        pixels->color.red   = rep_pixel->color.red;
        pixels->color.green = rep_pixel->color.green;
        pixels->color.blue  = rep_pixel->color.blue;
        pixels->color.alpha = rep_pixel->color.alpha;
      }
    }
  }
}

static VALUE
Texture_recolor(VALUE self, VALUE rbRect,
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


#define ROW_CHECKED 0x10
#define ROW_USED 0x01

typedef uint8_t** Bitmark;

static bool
strb_TextureBucketFillRow(Texture* texture, int32_t x, int32_t y,
                          const Color* color, const Pixel* lookup_pixel,
                          const Bitmark checked)
{
  bool filled_row = false;
  int32_t rx = x, ry = y;
  Pixel* pixels = &(TextureGetPixel(texture, rx, ry));
  for (;rx < texture->width; ++rx, ++pixels) {
    if (TexturePixelRGBAMatch(pixels, lookup_pixel)) {
      filled_row = true;
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
    for (;rx >= 0; --rx, --pixels) {
      if (TexturePixelRGBAMatch(pixels, lookup_pixel)) {
        filled_row = true;
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
  filled_row = false; \
  for (int32_t j = 0; j < texture->width; ++j) { \
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

static void
strb_TextureBucketFill(Texture* texture, int32_t x, int32_t y, Color* color)
{
  const Pixel lookup_pixel = TextureGetPixel(texture, x, y);
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (!(x < texture->width)) x = texture->width-1;
  if (!(y < texture->height)) y = texture->height-1;
  uint8_t** rows = malloc(sizeof(uint8_t*) * texture->height);
  for (int32_t rry = 0; rry < texture->height; ++rry) {
    rows[rry] = malloc(sizeof(uint8_t) * texture->width);
    for (int32_t rrx = 0; rrx < texture->width; ++rrx) {
      rows[rry][rrx] = 0;
    }
  }
  strb_TextureBucketFillRow(texture, x, y, color,
                            &(lookup_pixel), (Bitmark)rows);
  int32_t oy = y;
  int32_t ty = y + 1;
  bool filled_row = false;
  for (;ty < texture->height; ++ty) {
    ROW_FILL
  }
  oy = y;
  ty = y - 1;
  for (;ty >= 0; --ty) {
    ROW_FILL
  }
  /* Cleanup */
  for (int32_t rry = 0; rry < texture->height; ++rry) {
    free(rows[rry]);
  }
  free(rows);
}

static VALUE Texture_bucket_fill(VALUE self,
                                 VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  Color* color;
  int32_t x, y;
  Texture* texture;

  x = NUM2INT(rbX);
  y = NUM2INT(rbY);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  strb_AssertObjIsKindOf(rbColor, rb_cColor);
  Data_Get_Struct(self, Color, color);
  strb_TextureBucketFill(texture, x, y, color);
  return self;
}

#define assert_MaskType(mask) \
  switch (mask) { \
    case MASK_ALPHA: \
    case MASK_GRAY:  \
    case MASK_RED:   \
    case MASK_GREEN: \
    case MASK_BLUE:  \
      break;         \
    default:         \
      rb_raise(rb_eArgError, "Invalid MaskType %d", (int)mask); \
  }

static bool
strb_TextureMask(Texture *dst_texture, Texture *src_texture,
                 int dst_x, int dst_y, Rect* src_rect,
                 MaskType dst_mask, MaskType src_mask)
{
  int src_x      = src_rect->x;
  int src_y      = src_rect->y;
  int src_width  = src_rect->width;
  int src_height = src_rect->height;
  if (!ModifyRectInTexture(src_texture,
                           &(src_x), &(src_y), &(src_width), &(src_height))) {
    return false;
  }

  const int width  = MIN(src_width,  dst_texture->width - dst_x);
  const int height = MIN(src_height, dst_texture->height - dst_y);
  const int src_padding = src_texture->width - width;
  const int dst_padding = dst_texture->width - width;

  Pixel* src = &(src_texture->pixels[src_x + src_y * src_texture->width]);
  Pixel* dst = &(dst_texture->pixels[dst_x + dst_y * dst_texture->width]);

  uint8_t mask_value = 0;
  assert_MaskType(src_mask);
  assert_MaskType(dst_mask);

  for(int j = 0; j < height; ++j, src += src_padding, dst += dst_padding) {
    for(int k = 0; k < width; ++k, ++src, ++dst) {
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
  return true;
}

static VALUE
Texture_mask(VALUE self, VALUE rbX, VALUE rbY, VALUE rbDstMask,
             VALUE rbSrcTexture, VALUE rbSrcRect, VALUE rbSrcMask)
{
  rb_check_frozen(self);
  Texture* src_texture;
  Texture* dst_texture;
  Rect* src_rect;

  int x = NUM2INT(rbX);
  int y = NUM2INT(rbY);
  MaskType dst_mask = (MaskType)NUM2INT(rbDstMask);
  MaskType src_mask = (MaskType)NUM2INT(rbSrcMask);

  Data_Get_Struct(rbSrcTexture, Texture, src_texture);
  Data_Get_Struct(self, Texture, dst_texture);
  Data_Get_Struct(rbSrcRect, Rect, src_rect);

  strb_TextureCheckDisposed(src_texture);
  strb_TextureCheckDisposed(dst_texture);

  strb_TextureMask(dst_texture, src_texture, x, y, src_rect, dst_mask, src_mask);
  return self;
}



static inline void
ChangeHue(Color* color, const double angle)
{
  uint8_t r = color->red;
  uint8_t g = color->green;
  uint8_t b = color->blue;
  uint8_t max = MAX(MAX(r, g), b);
  uint8_t min = MIN(MIN(r, g), b);
  if (max != 0) {
    const double delta255 = max - min;
    const double v = max / 255.0;
    const double s = delta255 / max;
    double h;
    if (max == r) {
      h =     (g - b) / delta255;
    } else if (max == g) {
      h = 2 + (b - r) / delta255;
    } else {
      h = 4 + (r - g) / delta255;
    }
    if (h < 0.0) {
      h += 6.0;
    }
    h += angle * 6.0 / (2 * PI);
    if (6.0 <= h) {
      h -= 6.0;
    }
    const int ii = (int)h;
    const double f = h - ii;
    const uint8_t v255 = max;
    const uint8_t aa255 = (uint8_t)(v * (1 - s) * 255);
    const uint8_t bb255 = (uint8_t)(v * (1 - s * f) * 255);
    const uint8_t cc255 = (uint8_t)(v * (1 - s * (1 - f)) * 255);
    switch (ii) {
    case 0: r = v255;  g = cc255; b = aa255; break;
    case 1: r = bb255; g = v255;  b = aa255; break;
    case 2: r = aa255; g = v255;  b = cc255; break;
    case 3: r = aa255; g = bb255; b = v255;  break;
    case 4: r = cc255; g = aa255; b = v255;  break;
    case 5: r = v255;  g = aa255; b = bb255; break;
    }
    color->red   = r;
    color->green = g;
    color->blue  = b;
  }
}

static VALUE
Texture_change_hue_bang(VALUE self, VALUE rbAngle)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  const double angle = NUM2DBL(rbAngle);
  if (angle == 0) {
    return self;
  }
  Color* pixels = (Color*)texture->pixels;
  const int length = texture->width * texture->height;
  for (int i = 0; i < length; ++i, ++pixels) {
    ChangeHue(pixels, angle);
  }
  return self;
}

static VALUE
Texture_change_hue(VALUE self, VALUE rbAngle)
{
  const Texture* texture;
  //Texture* newTexture;

  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  volatile VALUE rbTexture = rb_obj_dup(self);
  //Data_Get_Struct(rbTexture, Texture, newTexture);
  Texture_change_hue_bang(rbTexture, rbAngle);
  return rbTexture;
}

//#define USE_CAIRO_LOAD_PNG

#ifdef HAVE_CAIRO
static void
strb_cairo_surface_to_texture_ARGB(cairo_surface_t* cr_surface,
                                   Texture* strb_texture)
{
  const int16_t width  = cairo_image_surface_get_width(cr_surface);
  const int16_t height = cairo_image_surface_get_height(cr_surface);
  const int32_t length = width * height;

  CairoColor32* data = (CairoColor32*)cairo_image_surface_get_data(cr_surface);

  for(int32_t i = 0; i < length; ++i)
  {
    strb_texture->pixels[i].color.red   = data[i].red;
    strb_texture->pixels[i].color.green = data[i].green;
    strb_texture->pixels[i].color.blue  = data[i].blue;
    strb_texture->pixels[i].color.alpha = data[i].alpha;
  }
}

VALUE
strb_TextureFromCairoSurface(cairo_surface_t* cr_surface)
{
  const int16_t width  = cairo_image_surface_get_width(cr_surface);
  const int16_t height = cairo_image_surface_get_height(cr_surface);
  VALUE rbTexture = rb_class_new_instance(2,
                                          (VALUE[]){INT2NUM(width),
                                                    INT2NUM(height)},
                                                    rb_cTexture);

  Texture* trg_texture;
  Data_Get_Struct(rbTexture, Texture, trg_texture);

  switch(cairo_image_surface_get_format(cr_surface))
  {
    case CAIRO_FORMAT_INVALID:
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB16_565:
    case CAIRO_FORMAT_RGB30:
      rb_raise(rb_eStarRubyError, "Unsupported format");
      break;
    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_ARGB32: // 0xAARRGGBB
      switch(cairo_surface_get_content(cr_surface))
      {
        case CAIRO_CONTENT_COLOR:
        case CAIRO_CONTENT_COLOR_ALPHA:
          strb_cairo_surface_to_texture_ARGB(cr_surface, trg_texture);
          break;
        case CAIRO_CONTENT_ALPHA:
          rb_raise(rb_eStarRubyError, "Cannot load Alpha only Image");
      }
      break;
  }

  return rbTexture;
}
#endif

typedef struct {
  char* bytes;
  unsigned long size;
  unsigned long offset;
} PngBuffer;

static void
ReadPng(png_structp pngPtr, png_bytep buf, png_size_t size)
{
  PngBuffer* pngBuffer = (PngBuffer*)png_get_io_ptr(pngPtr);
  if (pngBuffer->offset + size <= pngBuffer->size) {
    MEMCPY(buf, &(pngBuffer->bytes[pngBuffer->offset]), char, size);
    pngBuffer->offset += size;
  } else {
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG data");
  }
}

static VALUE
Texture_s_load_png(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbPathOrIO, rbOptions;
  rb_scan_args(argc, argv, "11", &rbPathOrIO, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  }
  const bool hasPalette = RTEST(rb_hash_aref(rbOptions, symbol_palette));
  unsigned long ioLength = 0;
  volatile VALUE val;
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_io_length))) {
    if (RTEST(rb_obj_is_kind_of(val, rb_cNumeric))) {
      if (RTEST(rb_funcall(val, rb_intern("<="), 1, INT2FIX(0)))) {
        rb_raise(rb_eArgError, "invalid io_length");
      }
      ioLength = NUM2ULONG(val);
      if (ioLength <= 8) {
        rb_raise(rb_eArgError, "invalid io_length");
      }
    } else {
      rb_raise(rb_eTypeError, "wrong argument type %s (expected Numeric)",
               rb_obj_classname(rbPathOrIO));
    }
  }

  png_structp pngPtr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pngPtr) {
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
  }
  png_infop infoPtr = png_create_info_struct(pngPtr);
  if (!infoPtr) {
    png_destroy_read_struct(&pngPtr, NULL, NULL);
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
  }
  png_infop endInfo = png_create_info_struct(pngPtr);
  if (!endInfo) {
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
  }

  volatile VALUE rbIO, rbIOToClose = Qnil;
  if (TYPE(rbPathOrIO) == T_STRING) {
    volatile VALUE rbCompletePath = strb_GetCompletePath(rbPathOrIO, true);
    volatile VALUE rbOpenOption = rb_str_new2("rb");
    rbIOToClose = rbIO =
      rb_funcall(rb_mKernel, rb_intern("open"), 2,
                 rbCompletePath, rbOpenOption);
  } else if (rb_respond_to(rbPathOrIO, rb_intern("read"))) {
    rbIO = rbPathOrIO;
  } else {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected String or IO)",
             rb_obj_classname(rbPathOrIO));
  }
  volatile VALUE rbHeader = rb_funcall(rbIO, rb_intern("read"), 1, INT2FIX(8));
  if (NIL_P(rbHeader)) {
    if (!NIL_P(rbIOToClose)) {
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    }
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG file (none header)");
  }
  png_byte header[8];
  MEMCPY(header, StringValuePtr(rbHeader), png_byte, 8);
  if (png_sig_cmp(header, 0, 8)) {
    if (!NIL_P(rbIOToClose)) {
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    }
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG file (invalid header)");
  }
  volatile VALUE rbData =
    ioLength == 0 ?
    rb_funcall(rbIO, rb_intern("read"), 0) :
    rb_funcall(rbIO, rb_intern("read"), 1, ULONG2NUM(ioLength - 8));
  PngBuffer pngBuffer = {
    .bytes = StringValuePtr(rbData),
    .size = RSTRING_LEN(rbData),
    .offset = 0,
  };
  png_set_read_fn(pngPtr, (png_voidp)(&pngBuffer), (png_rw_ptr)ReadPng);
  png_set_sig_bytes(pngPtr, 8);
  png_read_info(pngPtr, infoPtr);
  png_uint_32 width, height;
  int bitDepth, colorType, interlaceType;
  png_get_IHDR(pngPtr, infoPtr, &width, &height,
               &bitDepth, &colorType, &interlaceType, NULL, NULL);
  if (interlaceType != PNG_INTERLACE_NONE) {
    png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
    if (!NIL_P(rbIOToClose)) {
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    }
    rb_raise(strb_GetStarRubyErrorClass(),
             "not supported interlacing PNG image");
  }

  volatile VALUE rbTexture =
    rb_class_new_instance(2, (VALUE[]){INT2NUM(width), INT2NUM(height)}, self);
  Texture* texture;
  Data_Get_Struct(rbTexture, Texture, texture);

  if (bitDepth == 16) {
    png_set_strip_16(pngPtr);
  }
  if (colorType == PNG_COLOR_TYPE_PALETTE && !hasPalette) {
    png_set_palette_to_rgb(pngPtr);
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(pngPtr);
    }
  }
  if (bitDepth < 8) {
    png_set_packing(pngPtr);
  }
  if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
#if 15 <= PNG_LIBPNG_VER_SONUM
    png_set_expand_gray_1_2_4_to_8(pngPtr);
#else
    png_set_gray_1_2_4_to_8(pngPtr);
#endif
  }
  png_read_update_info(pngPtr, infoPtr);
  png_colorp palette = NULL;
  int numPalette = 0;
  png_get_PLTE(pngPtr, infoPtr, &palette, &numPalette);
  if (0 < numPalette && hasPalette) {
    texture->indexes = ALLOC_N(uint8_t, width * height);
    png_bytep trans = NULL;
    int numTrans = 0;
    png_get_tRNS(pngPtr, infoPtr, &trans, &numTrans, NULL);
    texture->palette_size = numPalette;
    Color* p = texture->palette = ALLOC_N(Color, texture->palette_size);
    for (int i = 0; i < texture->palette_size; ++i, ++p) {
      const png_colorp pngColorP = &(palette[i]);
      p->red   = pngColorP->red;
      p->green = pngColorP->green;
      p->blue  = pngColorP->blue;
      p->alpha = 0xff;
      for (int j = 0; j < numTrans; ++j) {
        if (i == trans[j]) {
          p->alpha = 0;
          break;
        }
      }
    }
  }
  const int channels = png_get_channels(pngPtr, infoPtr);
  const Color* srPalette = texture->palette;
  uint8_t* indexes = texture->indexes;
  for (unsigned int j = 0; j < height; ++j) {
    png_byte row[width * channels];
    png_read_row(pngPtr, row, NULL);
    for (unsigned int i = 0; i < width; ++i, ++indexes) {
      Color* c = &(texture->pixels[width * j + i].color);
      switch (channels) {
      case 1:
        *c = srPalette[*indexes = row[i]];
        break;
      case 2:
        c->red = c->green = c->blue = row[i * channels];
        c->alpha = row[i * channels + 1];
        break;
      case 3:
      case 4:
        c->red   = row[i * channels];
        c->green = row[i * channels + 1];
        c->blue  = row[i * channels + 2];
        c->alpha = (channels == 4) ? row[i * channels + 3] : 0xff;
        break;
      }
    }
  }
  png_read_end(pngPtr, endInfo);
  png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
  if (!NIL_P(rbIOToClose)) {
    rb_funcall(rbIOToClose, rb_intern("close"), 0);
  }
  if (TYPE(rbPathOrIO) == T_STRING) rb_iv_set(rbTexture, "filename", rbPathOrIO);
  return rbTexture;
}

static VALUE
Texture_save_png(VALUE self, VALUE rbPath)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  const char* path = StringValueCStr(rbPath);
  FILE* fp = fopen(path, "wb");
  if (!fp) {
    rb_raise(rb_path2class("Errno::ENOENT"), "%s", path);
  }
  rewind(fp);
  png_structp pngPtr =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop infoPtr = png_create_info_struct(pngPtr);
  png_init_io(pngPtr, fp);
  png_set_IHDR(pngPtr, infoPtr, texture->width, texture->height,
               8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(pngPtr, infoPtr);

  for (int j = 0; j < texture->height; ++j) {
    png_byte row[texture->width * 4];
    for (int i = 0; i < texture->width; ++i) {
      const Color* c = &(texture->pixels[texture->width * j + i].color);
      png_byte* const r = &(row[i * 4]);
      r[0] = c->red;
      r[1] = c->green;
      r[2] = c->blue;
      r[3] = c->alpha;
    }
    png_write_row(pngPtr, row);
  }

  png_write_end(pngPtr, infoPtr);
  png_destroy_write_struct(&pngPtr, &infoPtr);
  fclose(fp);
  return Qtrue;
}

// Verbosity
// 0 - Ignore and fix all errors internally
// 1 - Warn I
// 2 - Warn II
// 3 - Strict
#define VERBOSE_TEXTURE_TOOL 3

VALUE rb_mTextureTool = Qnil;

static VALUE
TextureTool_color_blend(VALUE klass, VALUE rbTexture, VALUE rbColor)
{
  strb_AssertObjIsKindOf(rbTexture, rb_cTexture);
  rb_check_frozen(rbTexture);

  const Texture* texture;
  Color color;

  Data_Get_Struct(rbTexture, Texture, texture);
  strb_TextureCheckDisposed(texture);

  strb_RubyToColor(rbColor, &color);

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

static VALUE
TextureTool_clipping_mask(VALUE klass,
                          VALUE rbDstTexture, VALUE rbDstX, VALUE rbDstY,
                          VALUE rbSrcTexture,
                          VALUE rbSrcX, VALUE rbSrcY,
                          VALUE rbSrcWidth, VALUE rbSrcHeight)
{
  strb_AssertObjIsKindOf(rbDstTexture, rb_cTexture);
  strb_AssertObjIsKindOf(rbSrcTexture, rb_cTexture);

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
  strb_AssertObjIsKindOf(rbRect, rb_cRect);
  strb_AssertObjIsKindOf(rbTexture, rb_cTexture);

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
  rb_define_singleton_method(rb_mTextureTool, "color_blend",
                   TextureTool_color_blend, 2);
  rb_define_singleton_method(rb_mTextureTool, "clipping_mask",
                   TextureTool_clipping_mask, 8);
  rb_define_singleton_method(rb_mTextureTool, "noise",
                             TextureTool_noise, 5);
  rb_iv_set(rb_mTextureTool, "@noise_seed", INT2NUM(0));

  return rb_mTextureTool;
}


static VALUE
Texture_transform_in_perspective(int argc, VALUE* argv, VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  volatile VALUE rbX, rbY, rbHeight, rbOptions;
  rb_scan_args(argc, argv, "31", &rbX, &rbY, &rbHeight, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  }
  const int screenWidth = texture->width;
  PerspectiveOptions options;
  AssignPerspectiveOptions(&options, rbOptions, texture);
  const double cosYaw   = cos(options.cameraYaw);
  const double sinYaw   = sin(options.cameraYaw);
  const double cosPitch = cos(options.cameraPitch);
  const double sinPitch = sin(options.cameraPitch);
  const double cosRoll  = cos(options.cameraRoll);
  const double sinRoll  = sin(options.cameraRoll);
  double x = NUM2INT(rbX) - options.cameraX;
  double y = NUM2DBL(rbHeight);
  double z = NUM2INT(rbY) - options.cameraY;
  double x2, y2, z2;
  x2 = cosYaw  * x + sinYaw * z;
  z2 = -sinYaw * x + cosYaw * z;
  x = x2;
  z = z2;
  y2 = sinPitch * z + cosPitch * (y - options.cameraHeight)
    + options.cameraHeight;
  z2 = cosPitch * z - sinPitch * (y - options.cameraHeight);
  y = y2;
  z = z2;
  volatile VALUE rbResult = rb_ary_new3(3, Qnil, Qnil, Qnil);
  OBJ_FREEZE(rbResult);
  if (z == 0) {
    return rbResult;
  }
  const double distance = screenWidth / (2 * tan(options.viewAngle / 2));
  const double scale = -distance / z;
  const double screenX = x * scale;
  const double screenY = (options.cameraHeight - y) * scale;
  const long screenXLong =
    (long)(cosRoll  * screenX + sinRoll * screenY + options.intersectionX);
  const long screenYLong =
    (long)(-sinRoll * screenX + cosRoll * screenY + options.intersectionY);
  if (FIXABLE(screenXLong) && INT_MIN <= screenXLong && screenXLong <= INT_MAX) {
    RARRAY_PTR(rbResult)[0] = LONG2FIX(screenXLong);
  } else {
    RARRAY_PTR(rbResult)[0] = Qnil;
  }
  if (FIXABLE(screenYLong) && INT_MIN <= screenYLong && screenYLong <= INT_MAX) {
    RARRAY_PTR(rbResult)[1] = LONG2FIX(screenYLong);
  } else {
    RARRAY_PTR(rbResult)[1] = Qnil;
  }
  RARRAY_PTR(rbResult)[2] = rb_float_new(scale);
  return rbResult;
}


static VALUE
Texture_fill(VALUE self, VALUE rbColor)
{
  Color color;
  Pixel* pixels;
  Texture* texture;

  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  strb_RubyToColor(rbColor, &color);

  pixels = texture->pixels;
  for (uint32_t i = 0; i < texture->size; ++i, ++pixels) {
    pixels->color = color;
  }
  return self;
}

static VALUE
Texture_render_rect(int argc, VALUE* argv, VALUE self)
{
  BlendType blend_type;
  uint32_t padding;
  Pixel px_color;
  Pixel* pixels;
  Rect cclip_rect;
  Rect rect;
  Texture* texture;
  VALUE rbBlendType;
  VALUE rbColor;

  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  if (argc <= 3)
  {
    VALUE rbRect;
    rb_scan_args(argc, argv, "21", &rbRect, &rbColor, &rbBlendType);
    strb_RubyToRect(rbRect, &(rect));
  } else if (argc <= 6)  {
    VALUE rbX, rbY, rbWidth, rbHeight;
    rb_scan_args(argc, argv, "51", &rbX, &rbY, &rbWidth, &rbHeight,
                                   &rbColor, &rbBlendType);
    rect.x      = NUM2INT(rbX);
    rect.y      = NUM2INT(rbY);
    rect.width  = NUM2INT(rbWidth);
    rect.height = NUM2INT(rbHeight);
  } else {
    rb_raise(rb_eArgError, "expected 2, 3, 5 or 6 arguments but recieved %d", argc);
  }

  if (!ModifyRectInTexture_Rect(texture, &(rect))) {
    return Qfalse;
  }

  if(NIL_P(rbBlendType)) {
    blend_type = BLEND_TYPE_NONE;
  } else {
    strb_RubyToBlendType(rbBlendType, &blend_type);
  }

  strb_RubyToColor(rbColor, &(px_color.color));
  strb_Rect_get_clip_rect(texture, &cclip_rect);

  pixels = &(texture->pixels[rect.x + rect.y * texture->width]);
  padding = texture->width - rect.width;

  for (uint32_t j = 0; j < rect.height; ++j, pixels += padding) {
    if(is_y_in_rect(cclip_rect, rect.y + j)) {
      for (uint32_t i = 0; i < rect.width; ++i, ++pixels) {
        if(is_x_in_rect(cclip_rect, rect.x + i)) {
          PIXEL_BLEND(blend_type, pixels, &px_color, 255);
        }
      }
    }
  }
  return self;
}


#define BIG_DELTA(x, m, n) (x * m / n)

#define CALC_DST_COLOR(m, n)                             \
  const int8_t dstRed   = CLAMPU255(baseRed   + BIG_DELTA(diffRed, m, n)); \
  const int8_t dstGreen = CLAMPU255(baseGreen + BIG_DELTA(diffGreen, m, n)); \
  const int8_t dstBlue  = CLAMPU255(baseBlue  + BIG_DELTA(diffBlue, m, n)); \
  const int8_t dstAlpha = CLAMPU255(baseAlpha + BIG_DELTA(diffAlpha, m, n));

#define SET_DST_COLOR(px)     \
  px->color.red   = dstRed;   \
  px->color.green = dstGreen; \
  px->color.blue  = dstBlue;  \
  px->color.alpha = dstAlpha;

static VALUE
Texture_gradient_fill_rect(int argc, VALUE* argv, VALUE self)
{
  bool vertical;
  Color color1;
  Color color2;
  int32_t padding;   /* Used to advance by 1 row in a texture */
  int32_t rows_back; /* Used by vertical to return to next column */
  Pixel* pixels;     /* Pointer to the Texture pixels data */
  Rect rect;
  int16_t baseAlpha;
  int16_t baseBlue;
  int16_t baseGreen;
  int16_t baseRed;
  int16_t diffAlpha;
  int16_t diffBlue;
  int16_t diffGreen;
  int16_t diffRed;
  Texture* texture;

  rb_check_frozen(self);

  if (argc == 3 || argc == 4)
  {
    VALUE rbRect, rbColor1, rbColor2, rbVertical;
    rb_scan_args(argc, argv, "31", &rbRect, &rbColor1, &rbColor2, &rbVertical);
    strb_RubyToRect(rbRect, &(rect));
    strb_RubyToColor(rbColor1, &(color1));
    strb_RubyToColor(rbColor2, &(color2));
    vertical = RTEST(rbVertical);
  } else if (argc == 6 || argc == 7)  {
    VALUE rbX, rbY, rbWidth, rbHeight, rbColor1, rbColor2, rbVertical;
    rb_scan_args(argc, argv, "61", &rbX, &rbY, &rbWidth, &rbHeight,
                                   &rbColor1, &rbColor2, &rbVertical);
    rect.x      = NUM2INT(rbX);
    rect.y      = NUM2INT(rbY);
    rect.width  = NUM2INT(rbWidth);
    rect.height = NUM2INT(rbHeight);
    strb_RubyToColor(rbColor1, &(color1));
    strb_RubyToColor(rbColor2, &(color2));
    vertical = RTEST(rbVertical);
  } else {
    rb_raise(rb_eArgError, "expected 3, 4, 6 or 7 arguments but recieved %d", argc);
  }

  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  if (!ModifyRectInTexture_Rect(texture, &(rect))) {
    return self;
  }

  pixels = &(texture->pixels[rect.x + rect.y * texture->width]);

  baseRed   = color1.red;
  baseGreen = color1.green;
  baseBlue  = color1.blue;
  baseAlpha = color1.alpha;
  diffRed   = color2.red   - baseRed;
  diffGreen = color2.green - baseGreen;
  diffBlue  = color2.blue  - baseBlue;
  diffAlpha = color2.alpha - baseAlpha;

  if (vertical) {
    padding = texture->width - rect.width;
    for(int32_t y = 0; y < rect.height; ++y, pixels += padding) {
      CALC_DST_COLOR(y, rect.height);
      for(int32_t x = 0; x < rect.width; ++x, ++pixels) {
        SET_DST_COLOR(pixels);
      }
    }
  } else {
    padding = texture->width;
    rows_back = padding * rect.height;
    for(int32_t x = 0; x < rect.width; ++x, pixels -= rows_back, ++pixels) {
      CALC_DST_COLOR(x, rect.width);
      for(int32_t y = 0; y < rect.height; ++y, pixels += padding) {
        SET_DST_COLOR(pixels);
      }
    }
  }

  return self;
}


#ifdef HAVE_CAIRO
static VALUE
Texture_cr_context(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  return texture->rb_cr_context;
}
#endif

static VALUE
Texture_filename(VALUE self)
{
  return rb_iv_get(self, "filename");
}

VALUE strb_InitializeTexture(VALUE rb_mStarRuby)
{
  rb_cTexture = rb_define_class_under(rb_mStarRuby, "Texture", rb_cObject);
  rb_define_alloc_func(rb_cTexture, Texture_alloc);

  rb_define_singleton_method(rb_cTexture, "load_png", Texture_s_load_png, -1);
  rb_define_singleton_method(rb_cTexture, "load", Texture_s_load_png, -1);

  rb_define_private_method(rb_cTexture, "initialize", Texture_initialize, 2);
  rb_define_private_method(rb_cTexture, "initialize_copy",
                           Texture_initialize_copy, 1);

  rb_define_method(rb_cTexture, "filename", Texture_filename, 0);

  rb_define_method(rb_cTexture, "[]",
                   Texture_aref, 2);
  rb_define_method(rb_cTexture, "[]=",
                   Texture_aset, 3);
  rb_define_method(rb_cTexture, "blur",
                   Texture_blur, 0);
  rb_define_method(rb_cTexture, "change_hue",
                   Texture_change_hue, 1);
  rb_define_method(rb_cTexture, "change_hue!",
                   Texture_change_hue_bang, 1);

  rb_define_method(rb_cTexture, "clear",
                   Texture_clear, 0);
  rb_define_method(rb_cTexture, "dispose",
                   Texture_dispose, 0);
  rb_define_method(rb_cTexture, "disposed?",
                   Texture_disposed, 0);

  rb_define_method(rb_cTexture, "dump",
                   Texture_dump, 1);
  rb_define_method(rb_cTexture, "undump",
                   Texture_undump, 2);

  rb_define_method(rb_cTexture, "fill",
                   Texture_fill, 1);
  rb_define_method(rb_cTexture, "clear_rect",
                   Texture_clear_rect, -1);
  rb_define_method(rb_cTexture, "gradient_fill_rect",
                   Texture_gradient_fill_rect, -1);
  rb_define_method(rb_cTexture, "height",
                   Texture_height, 0);

  //rb_define_method(rb_cTexture, "render_in_perspective",
  //                 Texture_render_in_perspective, -1);
  //rb_define_method(rb_cTexture, "transform_in_perspective",
  //                 Texture_transform_in_perspective, -1);

  rb_define_method(rb_cTexture, "render_line",
                   Texture_render_line, 5);

  rb_define_method(rb_cTexture, "render_pixel",
                   Texture_render_pixel, 3);

  rb_define_method(rb_cTexture, "render_rect",
                   Texture_render_rect, -1);

  rb_define_method(rb_cTexture, "render_text",
                   Texture_render_text, -1);

  rb_define_method(rb_cTexture, "render_texture",
                   Texture_render_texture, -1);

  /* saving options */
  rb_define_method(rb_cTexture, "save_png", Texture_save_png, 1);
  rb_define_alias(rb_cTexture, "save", "save_png");

  rb_define_method(rb_cTexture, "size",
                   Texture_size, 0);

  rb_define_method(rb_cTexture, "width",
                   Texture_width, 0);

  rb_define_method(rb_cTexture, "rect",
                   Texture_rect, 0);

  //rb_define_method(rb_cTexture, "rotate",
  //                 Texture_rotate, 1);

  //rb_define_method(rb_cTexture, "crop",
  //                 Texture_crop, -1);

  //rb_define_method(rb_cTexture, "recolor",
  //                 Texture_recolor, 3);

  //rb_define_method(rb_cTexture, "bucket_fill",
  //                 Texture_bucket_fill, 3);

  rb_define_method(rb_cTexture, "mask",
                   Texture_mask, 6);

  /* clip_rect */
  rb_define_method(rb_cTexture, "clip_rect",
                   Texture_clip_rect, 0);
  rb_define_method(rb_cTexture, "clip_rect=",
                   Texture_clip_rect_eq, 1);
#ifdef HAVE_CAIRO
  rb_define_method(rb_cTexture, "cr_context", Texture_cr_context, 0);
  rb_define_method(rb_cTexture, "cr_recontext", Texture_recycle_cr_context, 0);
#endif

  rb_define_const(rb_cTexture, "BLEND_NONE",     INT2FIX(BLEND_TYPE_NONE));
  rb_define_const(rb_cTexture, "BLEND_ALPHA",    INT2FIX(BLEND_TYPE_ALPHA));
  rb_define_const(rb_cTexture, "BLEND_ADD",      INT2FIX(BLEND_TYPE_ADD));
  rb_define_const(rb_cTexture, "BLEND_SUBTRACT", INT2FIX(BLEND_TYPE_SUBTRACT));
  rb_define_const(rb_cTexture, "BLEND_MULTIPLY", INT2FIX(BLEND_TYPE_MULTIPLY));
  rb_define_const(rb_cTexture, "BLEND_DIVIDE",   INT2FIX(BLEND_TYPE_DIVIDE));
  rb_define_const(rb_cTexture, "BLEND_SRC_MASK", INT2FIX(BLEND_TYPE_SRC_MASK));
  rb_define_const(rb_cTexture, "BLEND_DST_MASK", INT2FIX(BLEND_TYPE_DST_MASK));
  rb_define_const(rb_cTexture, "BLEND_CLEAR",    INT2FIX(BLEND_TYPE_CLEAR));

  rb_define_const(rb_cTexture, "ROTATE_NONE", INT2FIX(ROTATE_NONE));
  rb_define_const(rb_cTexture, "ROTATE_CW",   INT2FIX(ROTATE_CW));
  rb_define_const(rb_cTexture, "ROTATE_CCW",  INT2FIX(ROTATE_CCW));
  rb_define_const(rb_cTexture, "ROTATE_180",  INT2FIX(ROTATE_180));
  rb_define_const(rb_cTexture, "ROTATE_HORZ", INT2FIX(ROTATE_HORZ));
  rb_define_const(rb_cTexture, "ROTATE_VERT", INT2FIX(ROTATE_VERT));

  rb_define_const(rb_cTexture, "MASK_ALPHA",  INT2FIX(MASK_ALPHA));
  rb_define_const(rb_cTexture, "MASK_GRAY",   INT2FIX(MASK_GRAY));
  rb_define_const(rb_cTexture, "MASK_RED",    INT2FIX(MASK_RED));
  rb_define_const(rb_cTexture, "MASK_GREEN",  INT2FIX(MASK_GREEN));
  rb_define_const(rb_cTexture, "MASK_BLUE",   INT2FIX(MASK_BLUE));

  return rb_cTexture;
}
