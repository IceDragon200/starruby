/*
  StarRuby Texture
 */
#include <assert.h>
#include <png.h>
#include "vector.h"
#include "rect.h"
#include "starruby.prv.h"
#include "texture.prv.h"

#include "texture/inline.inc.c"
#include "texture/alloc.inc.c"
#include "texture/png-file.inc.c"
#include "texture/cairo-bind.inc.c"
#include "texture/blur.inc.c"
#include "texture/hue.inc.c"
#include "texture/pixel.inc.c"
#include "texture/pixel-blend.inc.c"
#include "texture/query.inc.c"
#include "texture/render.inc.c"
#include "texture/render-texture.inc.c"
#include "texture/render-text.inc.c"
#include "texture/gradient_fill_rect.inc.c"
#include "texture/dump.inc.c"
#include "texturetool/texturetool.c"

STRUCT_CHECK_TYPE_FUNC(Texture, Texture);

VALUE strb_GetTextureClass(void)
{
  return rb_cTexture;
}

static VALUE
Texture_initialize(VALUE self, VALUE rbWidth, VALUE rbHeight)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  const int32_t width  = NUM2INT(rbWidth);
  const int32_t height = NUM2INT(rbHeight);

  if (width <= 0)
    rb_raise(rb_eArgError, "width must be greater than 0");
  if (height <= 0)
    rb_raise(rb_eArgError, "height must be greater than 0");

  texture->width  = width;
  texture->height = height;

  const uint64_t length = texture->width * texture->height;
  texture->pixels = ALLOC_N(Pixel, length);
  MEMZERO(texture->pixels, Pixel, length);

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
  strb_CheckDisposedTexture(texture);

  MEMZERO(texture->pixels, Color, texture->width * texture->height);
  return self;
}

static VALUE
Texture_fill(VALUE self, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);

  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  const int length = texture->width * texture->height;
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
  strb_CheckDisposedTexture(texture);

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
  strb_CheckDisposedTexture(texture);
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

VALUE
strb_InitializeTexture(VALUE rb_mStarRuby)
{
  rb_cTexture = rb_define_class_under(rb_mStarRuby, "Texture", rb_cObject);
  rb_define_singleton_method(rb_cTexture, "load", Texture_s_load, -1);
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

  rb_define_method(rb_cTexture, "bind_to_cairo", Texture_bind_cairo, 1);
  rb_define_method(rb_cTexture, "unbind", Texture_unbind, 0);

  symbol_add            = ID2SYM(rb_intern("add"));
  symbol_alpha          = ID2SYM(rb_intern("alpha"));
  symbol_angle          = ID2SYM(rb_intern("angle"));
  symbol_background     = ID2SYM(rb_intern("background"));
  symbol_blend_type     = ID2SYM(rb_intern("blend_type"));
  symbol_blur           = ID2SYM(rb_intern("blur"));
  symbol_camera_height  = ID2SYM(rb_intern("camera_height"));
  symbol_camera_pitch   = ID2SYM(rb_intern("camera_pitch"));
  symbol_camera_roll    = ID2SYM(rb_intern("camera_roll"));
  symbol_camera_x       = ID2SYM(rb_intern("camera_x"));
  symbol_camera_y       = ID2SYM(rb_intern("camera_y"));
  symbol_camera_yaw     = ID2SYM(rb_intern("camera_yaw"));
  symbol_center_x       = ID2SYM(rb_intern("center_x"));
  symbol_center_y       = ID2SYM(rb_intern("center_y"));
  symbol_color          = ID2SYM(rb_intern("color"));
  symbol_height         = ID2SYM(rb_intern("height"));
  symbol_intersection_x = ID2SYM(rb_intern("intersection_x"));
  symbol_intersection_y = ID2SYM(rb_intern("intersection_y"));
  symbol_io_length      = ID2SYM(rb_intern("io_length"));
  symbol_loop           = ID2SYM(rb_intern("loop"));
  symbol_mask           = ID2SYM(rb_intern("mask"));
  symbol_matrix         = ID2SYM(rb_intern("matrix"));
  symbol_none           = ID2SYM(rb_intern("none"));
  symbol_palette        = ID2SYM(rb_intern("palette"));
  symbol_saturation     = ID2SYM(rb_intern("saturation"));
  symbol_scale_x        = ID2SYM(rb_intern("scale_x"));
  symbol_scale_y        = ID2SYM(rb_intern("scale_y"));
  symbol_src_height     = ID2SYM(rb_intern("src_height"));
  symbol_src_width      = ID2SYM(rb_intern("src_width"));
  symbol_src_x          = ID2SYM(rb_intern("src_x"));
  symbol_src_y          = ID2SYM(rb_intern("src_y"));
  symbol_sub            = ID2SYM(rb_intern("sub"));
  symbol_tone           = ID2SYM(rb_intern("tone"));
  symbol_view_angle     = ID2SYM(rb_intern("view_angle"));
  symbol_width          = ID2SYM(rb_intern("width"));
  symbol_x              = ID2SYM(rb_intern("x"));
  symbol_y              = ID2SYM(rb_intern("y"));

  ID_expand_path    = rb_intern("expand_path");
  ID_extname        = rb_intern("extname");

  return rb_cTexture;
}
