/*
  StarRuby Texture
 */
#include <png.h>

#include "vector.h"
#include "rect.h"
#include "starruby.prv.h"
#include "texture.prv.h"

#include "texture/inline.inc.c"
#include "texture/alloc.inc.c"
#include "texture/cairo-bind.inc.c"
#include "texture/blur.inc.c"
#include "texture/rotate.inc.c"
#include "texture/crop.inc.c"
#include "texture/recolor.inc.c"
#include "texture/mask.inc.c"
#include "texture/hue.inc.c"
#include "texture/pixel.inc.c"
#include "texture/pixel-blend.inc.c"
#include "texture/query.inc.c"
#include "texture/render.inc.c"
#include "texture/render-texture.inc.c"
#include "texture/render-text.inc.c"
#include "texture/gradient_fill_rect.inc.c"
#include "texture/dump.inc.c"
#include "texture/texure-from-cairo.inc.c"
#include "texture/load-png.inc.c"
#ifdef STRB_CAN_LOAD_SVG
  #include "texture/load-svg.inc.c"
#endif
//#include "texture/load-jpg.inc.c"
//#include "texture/load-bmp.inc.c"
#include "texture/texture-save.inc.c"
#include "texturetool/texturetool.c"

volatile VALUE rb_cTexture = Qundef;

static VALUE Texture_initialize(VALUE self, VALUE rbWidth, VALUE rbHeight)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  const Integer width  = NUM2INT(rbWidth);
  const Integer height = NUM2INT(rbHeight);

  if (width <= 0) {
    rb_raise(rb_eArgError, "width must be greater than 0");
  }
  if (height <= 0) {
    rb_raise(rb_eArgError, "height must be greater than 0");
  }

  texture->width  = width;
  texture->height = height;
  strb_TextureAllocData(texture);

  return Qnil;
}

static VALUE
Texture_initialize_copy(VALUE self, VALUE rbTexture)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  const Texture* origTexture;
  Data_Get_Struct(rbTexture, Texture, origTexture);
  texture->width  = origTexture->width;
  texture->height = origTexture->height;
  const int length = texture->width * texture->height;
  texture->pixels = ALLOC_N(Pixel, length);
  MEMCPY(texture->pixels, origTexture->pixels, Pixel, length);
  texture->binded = false;

  return Qnil;
}

inline static VALUE
Texture_clear(VALUE self)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  MEMZERO(texture->pixels, Pixel, texture->width * texture->height);
  return self;
}

static VALUE
Texture_fill(VALUE self, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  const Bignum length = texture->width * texture->height;
  Pixel* pixels = texture->pixels;
  for (int i = 0; i < length; i++, pixels++) {
    pixels->color = color;
  }
  return self;
}

static VALUE
Texture_fill_rect(VALUE self, VALUE rbX, VALUE rbY,
                  VALUE rbWidth, VALUE rbHeight, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  int rectX = NUM2INT(rbX);
  int rectY = NUM2INT(rbY);
  int rectWidth  = NUM2INT(rbWidth);
  int rectHeight = NUM2INT(rbHeight);
  if (!ModifyRectInTexture(texture, &rectX, &rectY, &rectWidth, &rectHeight)) {
    return self;
  }
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  Pixel* pixels = &(texture->pixels[rectX + rectY * texture->width]);
  const int paddingJ = texture->width - rectWidth;
  for (int j = rectY; j < rectY + rectHeight; j++, pixels += paddingJ) {
    for (int i = rectX; i < rectX + rectWidth; i++, pixels++) {
      pixels->color = color;
    }
  }
  return self;
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

VALUE strb_InitializeTexture(VALUE rb_mStarRuby)
{
  rb_cTexture = rb_define_class_under(rb_mStarRuby, "Texture", rb_cObject);
#ifdef STARRUBY_TEXTURE_LOAD_PNG
  rb_define_singleton_method(rb_cTexture, "load", Texture_s_load, 1);
#endif
#ifdef STARRUBY_TEXTURE_LOAD_BMP
  rb_define_singleton_method(rb_cTexture, "load_bmp", Texture_s_load_bmp, 1);
#endif
#ifdef STARRUBY_TEXTURE_LOAD_JPG
  rb_define_singleton_method(rb_cTexture, "load_jpg", Texture_s_load_jpg, 1);
#endif
#ifdef STARRUBY_TEXTURE_LOAD_SVG
  rb_define_singleton_method(rb_cTexture, "load_svg", Texture_s_load_svg, 3);
#endif
  rb_define_singleton_method(rb_cTexture, "bind_from_cairo",
                             Texture_s_bind_from_cairo, 1);
  rb_define_alloc_func(rb_cTexture, Texture_alloc);
  rb_define_private_method(rb_cTexture, "initialize", Texture_initialize, 2);
  rb_define_private_method(rb_cTexture, "initialize_copy",
                           Texture_initialize_copy, 1);
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
  rb_define_method(rb_cTexture, "fill",
                   Texture_fill, 1);
  rb_define_method(rb_cTexture, "fill_rect",
                   Texture_fill_rect, 5);
  rb_define_method(rb_cTexture, "gradient_fill_rect",
                   Texture_gradient_fill_rect, 7);
  rb_define_method(rb_cTexture, "height",
                   Texture_height, 0);

  rb_define_method(rb_cTexture, "render_in_perspective",
                   Texture_render_in_perspective, -1);
  rb_define_method(rb_cTexture, "render_line",
                   Texture_render_line, 5);
  rb_define_method(rb_cTexture, "render_pixel",
                   Texture_render_pixel, 3);
  rb_define_method(rb_cTexture, "render_rect",
                   Texture_render_rect, 5);
  rb_define_method(rb_cTexture, "render_text",
                   Texture_render_text, -1);
  rb_define_method(rb_cTexture, "render_texture",
                   Texture_render_texture, -1);
  rb_define_method(rb_cTexture, "save",
                   Texture_save, 1);
  rb_define_method(rb_cTexture, "size",
                   Texture_size, 0);
  rb_define_method(rb_cTexture, "transform_in_perspective",
                   Texture_transform_in_perspective, -1);
  rb_define_method(rb_cTexture, "undump",
                   Texture_undump, 2);
  rb_define_method(rb_cTexture, "width",
                   Texture_width, 0);

  rb_define_method(rb_cTexture, "rect",
                   Texture_rect, 0);

  rb_define_method(rb_cTexture, "rotate",
                   Texture_rotate, 1);

  rb_define_method(rb_cTexture, "crop",
                   Texture_crop, -1);
  rb_define_method(rb_cTexture, "recolor",
                   Texture_recolor, 3);
  rb_define_method(rb_cTexture, "mask",
                   Texture_mask, 6);
  rb_define_method(rb_cTexture, "bind_to_cairo", Texture_bind_cairo, 1);
  rb_define_method(rb_cTexture, "unbind", Texture_unbind, 0);

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
