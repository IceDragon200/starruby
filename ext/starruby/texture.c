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
#include "texture/png-file.inc.c"
#include "texture/cairo-bind.inc.c"

static void
Texture_free(Texture* texture)
{
  if(!(texture->binded))
    free(texture->pixels);
  texture->pixels = NULL;

  free(texture);
}

STRUCT_CHECK_TYPE_FUNC(Texture, Texture);

VALUE
strb_GetTextureClass(void)
{
  return rb_cTexture;
}

static inline bool Texture_is_binded(Texture *texture)
{
  return texture->binded;
}

static VALUE
Texture_alloc(VALUE klass)
{
  Texture* texture = ALLOC(Texture);
  texture->width  = 0;
  texture->height = 0;
  texture->binded = false;
  texture->pixels = NULL;
  return Data_Wrap_Struct(klass, NULL, Texture_free, texture);
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

static VALUE
Texture_aref(VALUE self, VALUE rbX, VALUE rbY)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    rb_raise(rb_eArgError, "index out of range: (%d, %d)", x, y);
  }
  const Color color = texture->pixels[x + y * texture->width].color;
  return rb_funcall(strb_GetColorClass(), rb_intern("new"), 4,
                    INT2FIX(color.red),
                    INT2FIX(color.green),
                    INT2FIX(color.blue),
                    INT2FIX(color.alpha));
}

static VALUE
Texture_aset(VALUE self, VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);

  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    return Qnil;
  }
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  texture->pixels[x + y * texture->width].color = color;
  return rbColor;
}

static VALUE Texture_change_hue_bang(VALUE, VALUE);

static VALUE
Texture_change_hue(VALUE self, VALUE rbAngle)
{
  const Texture* texture;
  //Texture* newTexture;

  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  volatile VALUE rbTexture = rb_obj_dup(self);
  //Data_Get_Struct(rbTexture, Texture, newTexture);
  Texture_change_hue_bang(rbTexture, rbAngle);
  return rbTexture;
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
  strb_CheckDisposedTexture(texture);
  const double angle = NUM2DBL(rbAngle);
  if (angle == 0) {
    return Qnil;
  }
  Color* pixels = (Color*)texture->pixels;
  const int length = texture->width * texture->height;
  for (int i = 0; i < length; i++, pixels++) {
    ChangeHue(pixels, angle);
  }
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
Texture_dispose(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);

  if(!Texture_is_binded(texture))
    free(texture->pixels);

  texture->pixels = NULL;
  return Qnil;
}

static VALUE
Texture_disposed(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  return !texture->pixels ? Qtrue : Qfalse;
}

static VALUE
Texture_dump(VALUE self, VALUE rbFormat)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  const char* format = StringValuePtr(rbFormat);
  const int formatLength = RSTRING_LEN(rbFormat);
  const int pixelLength = texture->width * texture->height;
  volatile VALUE rbResult = rb_str_new(NULL, pixelLength * formatLength);
  uint8_t* strPtr = (uint8_t*)RSTRING_PTR(rbResult);
  const Pixel* pixels = texture->pixels;
  for (int i = 0; i < pixelLength; i++, pixels++) {
    for (int j = 0; j < formatLength; j++, strPtr++) {
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

#define TRANSITION_COLOR(col, base, diff, rt, rM)            \
  Color col;                                                 \
  col.red   = CLAMP255(base.red   + (diff.red   * rt / rM)); \
  col.green = CLAMP255(base.green + (diff.green * rt / rM)); \
  col.blue  = CLAMP255(base.blue  + (diff.blue  * rt / rM)); \
  col.alpha = CLAMP255(base.alpha + (diff.alpha * rt / rM))

struct ColorDiff {
  int red, green, blue, alpha;
};

static VALUE
Texture_gradient_fill_rect(VALUE self,
                  VALUE rbX, VALUE rbY,
                  VALUE rbWidth, VALUE rbHeight,
                  VALUE rbColor1, VALUE rbColor2, VALUE rbVertical)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);

  Pixel* pixels = texture->pixels;

  int rx = NUM2INT(rbX);
  int ry = NUM2INT(rbY);
  int rwidth = NUM2INT(rbWidth);
  int rheight = NUM2INT(rbHeight);
  int rx2 = rx + rwidth;
  int ry2 = ry + rheight;

  if (!ModifyRectInTexture(texture, &rx, &ry, &rwidth, &rheight)) {
    return self;
  }

  Color color1, color2;
  strb_GetColorFromRubyValue(&color1, rbColor1);
  strb_GetColorFromRubyValue(&color2, rbColor2);

  struct ColorDiff base;
  base.red   = color1.red;
  base.green = color1.green;
  base.blue  = color1.blue;
  base.alpha = color1.alpha;

  struct ColorDiff diff;
  diff.red   = color2.red   - base.red;
  diff.green = color2.green - base.green;
  diff.blue  = color2.blue  - base.blue;
  diff.alpha = color2.alpha - base.alpha;

  signed int r = 0;
  if(rbVertical == Qtrue) {
    for(int y = ry; y < ry2; y++, r++) {
      TRANSITION_COLOR(color, base, diff, r, rheight);

      for(int x = rx; x < rx2; x++) {
        pixels[x + (y * texture->width)].color = color;
      }
    }
  }
  else
  {
    for(int x = rx; x < rx2; x++, r++) {
      TRANSITION_COLOR(color, base, diff, r, rwidth);

      for(int y = ry; y < ry2; y++) {
        pixels[x + (y * texture->width)].color = color;
      }
    }
  }

  return self;
}

// #blur
#define NORMALIZE_COLORS3(target_color, color1, color2, color3)                \
  target_color.red   = ((color1.red + color2.red + color3.red) / 3);       \
  target_color.green = ((color1.green + color2.green + color3.green) / 3); \
  target_color.blue  = ((color1.blue + color2.blue + color3.blue) / 3);    \
  target_color.alpha = ((color1.alpha + color2.alpha + color3.alpha) / 3);

static VALUE
Texture_blur(VALUE self)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);

  Pixel* src_pixels = texture->pixels;
  Pixel* pixels = texture->pixels;

  for(int y = 1; y < texture->height; y++) {
    for(int x = 1; x < texture->width; x++) {
      Pixel cpx, hpx, vpx;
      Color rscol;

      const int cpx_index = x + (y * texture->width);

      cpx = (src_pixels[cpx_index]);
      hpx = (src_pixels[x - 1 + (y * texture->width)]);
      vpx = (src_pixels[x + ((y - 1) * texture->width)]);

      NORMALIZE_COLORS3(rscol, cpx.color, hpx.color, vpx.color);

      pixels[cpx_index].color = rscol;
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

      pixels[cpx_index].color = rscol;
    }
  }

  return self;
}

static VALUE
Texture_height(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  return INT2NUM(texture->height);
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
      strb_GetColorFromRubyValue(&color, val);
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
  strb_CheckTexture(rbTexture);
  const Texture* srcTexture;
  Data_Get_Struct(rbTexture, Texture, srcTexture);
  strb_CheckDisposedTexture(srcTexture);
  const Texture* dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);
  strb_CheckDisposedTexture(dstTexture);

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
  const Vector3F screenDX = {
    cosRoll * cosYaw + sinRoll * sinPitch * sinYaw,
    sinRoll * -cosPitch,
    cosRoll * sinYaw - sinRoll * sinPitch * cosYaw,
  };
  const Vector3F screenDY = {
    -sinRoll * cosYaw + cosRoll * sinPitch * sinYaw,
    cosRoll * -cosPitch,
    -sinRoll * sinYaw - cosRoll * sinPitch * cosYaw,
  };
  const double distance = dstWidth / (2 * (tan(options.viewAngle / 2)));
  const Point3F intersection = {
    distance * (cosPitch * sinYaw),
    distance * sinPitch + options.cameraHeight,
    distance * (-cosPitch * cosYaw),
  };
  const Point3F screenO = {
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
  Point3F screenP;
  for (int j = 0; j < dstHeight; j++) {
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
        dst++;
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
  strb_CheckDisposedTexture(texture);

  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  int x = x1;
  int y = y1;
  const int dx = abs(x2 - x1);
  const int dy = abs(y2 - y1);
  const int signX = (x1 <= x2) ? 1 : -1;
  const int signY = (y1 <= y2) ? 1 : -1;
  if (dy <= dx) {
    int e = dx;
    const int eLimit = dx << 1;
    for (int i = 0; i <= dx; i++) {
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
    for (int i = 0; i <= dy; i++) {
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
  strb_CheckDisposedTexture(texture);

  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    return self;
  }
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  Pixel* pixel = &(texture->pixels[x + y * texture->width]);
  RENDER_PIXEL(pixel->color, color);
  return self;
}

static VALUE
Texture_render_rect(VALUE self, VALUE rbX, VALUE rbY,
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
      RENDER_PIXEL(pixels->color, color);
    }
  }
  return self;
}

static VALUE Texture_render_texture(int, VALUE*, VALUE);

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

static int
AssignRenderingTextureOptions(st_data_t key, st_data_t val,
                              RenderingTextureOptions* options)
{
  if (key == symbol_src_x) {
    options->srcX = NUM2INT(val);
  } else if (key == symbol_src_y) {
    options->srcY = NUM2INT(val);
  } else if (key == symbol_src_width) {
    options->srcWidth = NUM2INT(val);
  } else if (key == symbol_src_height) {
    options->srcHeight = NUM2INT(val);
  } else if (key == symbol_scale_x) {
    options->scaleX = NUM2DBL(val);
  } else if (key == symbol_scale_y) {
    options->scaleY = NUM2DBL(val);
  } else if (key == symbol_angle) {
    options->angle = NUM2DBL(val);
  } else if (key == symbol_center_x) {
    options->centerX = NUM2INT(val);
  } else if (key == symbol_center_y) {
    options->centerY = NUM2INT(val);
  } else if (key == symbol_matrix) {
    ASSIGN_MATRIX(options, val);
  } else if (key == symbol_alpha) {
    options->alpha = NUM2DBL(val);
  } else if (key == symbol_blend_type) {
    Check_Type(val, T_SYMBOL);
    if (val == symbol_none) {
      options->blendType = BLEND_TYPE_NONE;
    } else if (val == symbol_alpha) {
      options->blendType = BLEND_TYPE_ALPHA;
    } else if (val == symbol_add) {
      options->blendType = BLEND_TYPE_ADD;
    } else if (val == symbol_sub) {
      options->blendType = BLEND_TYPE_SUB;
    } else if (val == symbol_mask) {
      options->blendType = BLEND_TYPE_MASK;
    }
  } else if (key == symbol_tone_red) {
    options->tone.red = NUM2INT(val);
  } else if (key == symbol_tone_green) {
    options->tone.green = NUM2INT(val);
  } else if (key == symbol_tone_blue) {
    options->tone.blue = NUM2INT(val);
  } else if (key == symbol_saturation) {
    options->tone.saturation = NUM2INT(val);
  }
  return ST_CONTINUE;
}

static void
RenderTexture(const Texture* srcTexture, const Texture* dstTexture,
              int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY,
              const uint8_t alpha, const BlendType blendType)
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
  const Pixel* src = &(srcTexture->pixels[srcX + srcY * srcTextureWidth]);
  Pixel* dst       = &(dstTexture->pixels[dstX + dstY * dstTextureWidth]);
  const int srcPadding = srcTextureWidth - width;
  const int dstPadding = dstTextureWidth - width;
  switch (blendType) {
  case BLEND_TYPE_ALPHA:
    if (alpha == 255) {
      for (int j = 0; j < height; j++, src += srcPadding, dst += dstPadding) {
        LOOP({
            const uint8_t beta = src->color.alpha;
            const uint8_t dstAlpha = dst->color.alpha;
            if ((beta == 255) | (dstAlpha == 0)) {
              *dst = *src;
            } else if (beta) {
              if (dstAlpha < beta) {
                dst->color.alpha = beta;
              }
              dst->color.red =
                ALPHA(src->color.red,   dst->color.red,   beta);
              dst->color.green =
                ALPHA(src->color.green, dst->color.green, beta);
              dst->color.blue =
                ALPHA(src->color.blue,  dst->color.blue,  beta);
            }
            src++;
            dst++;
          }, width);
      }
    } else if (0 < alpha) {
      for (int j = 0; j < height; j++, src += srcPadding, dst += dstPadding) {
        LOOP({
            const uint8_t dstAlpha = dst->color.alpha;
            const uint8_t beta = DIV255(src->color.alpha * alpha);
            if (dstAlpha == 0) {
              dst->color.alpha = beta;
              dst->color.red   = src->color.red;
              dst->color.green = src->color.green;
              dst->color.blue  = src->color.blue;
            } else if (beta) {
              if (dstAlpha < beta) {
                dst->color.alpha = beta;
              }
              dst->color.red =
                ALPHA(src->color.red,   dst->color.red,   beta);
              dst->color.green =
                ALPHA(src->color.green, dst->color.green, beta);
              dst->color.blue =
                ALPHA(src->color.blue,  dst->color.blue,  beta);
            }
            src++;
            dst++;
          }, width);
      }
    }
    break;
  case BLEND_TYPE_NONE:
    for (int j = 0; j < height; j++, src += srcPadding, dst += dstPadding) {
      LOOP({
          *dst = *src;
          src++;
          dst++;
        }, width);
    }
    break;
  default:
    assert(false);
    break;
  }
}

static VALUE
Texture_render_text(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbText, rbX, rbY, rbFont, rbColor, rbAntiAlias;

  rb_scan_args(argc, argv, "51",
               &rbText, &rbX, &rbY, &rbFont, &rbColor, &rbAntiAlias);

  Check_Type(rbText, T_STRING);

  if (!(RSTRING_LEN(rbText))) {
    return self;
  }

  const bool antiAlias = RTEST(rbAntiAlias);
  const char* text = StringValueCStr(rbText);

  strb_CheckFont(rbFont);

  const Font* font;

  Data_Get_Struct(rbFont, Font, font);

  volatile VALUE rbSize = rb_funcall(rbFont, rb_intern("get_size"), 1, rbText);
  volatile VALUE rbTextTexture =
    rb_class_new_instance(2, RARRAY_PTR(rbSize), rb_cTexture);

  const Texture* textTexture;
  Data_Get_Struct(rbTextTexture, Texture, textTexture);
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);

  SDL_Surface* textSurfaceRaw;
  if (antiAlias) {
    textSurfaceRaw =
      TTF_RenderUTF8_Shaded(font->sdlFont, text,
                            (SDL_Color){255, 255, 255, 255},
                            (SDL_Color){0, 0, 0, 0});
  } else {
    textSurfaceRaw =
      TTF_RenderUTF8_Solid(font->sdlFont, text,
                           (SDL_Color){255, 255, 255, 255});
  }
  if (!textSurfaceRaw) {
    rb_raise_sdl_ttf_error();
  }
  SDL_PixelFormat format = {
    .palette = NULL, .BitsPerPixel = 32, .BytesPerPixel = 4,
    .Rmask = 0x00ff0000, .Gmask = 0x0000ff00,
    .Bmask = 0x000000ff, .Amask = 0xff000000,
    .colorkey = 0, .alpha = 255,
  };
  SDL_Surface* textSurface =
    SDL_ConvertSurface(textSurfaceRaw, &format, SDL_SWSURFACE);
  SDL_FreeSurface(textSurfaceRaw);
  textSurfaceRaw = NULL;
  if (!textSurface) {
    rb_raise_sdl_error();
  }
  SDL_LockSurface(textSurface);
  const Pixel* src = (Pixel*)(textSurface->pixels);
  Pixel* dst = textTexture->pixels;
  const int size = textTexture->width * textTexture->height;
  for (int i = 0; i < size; i++, src++, dst++) {
    if (src->value) {
      dst->color = color;
      if (color.alpha == 255) {
        dst->color.alpha = src->color.red;
      } else {
        dst->color.alpha = DIV255(src->color.red * color.alpha);
      }
    }
  }
  SDL_UnlockSurface(textSurface);
  SDL_FreeSurface(textSurface);
  textSurface = NULL;

  Texture *dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);

  RenderTexture(textTexture, dstTexture,
    0, 0, textTexture->width, textTexture->height, NUM2INT(rbX), NUM2INT(rbY),
    255, 1);

  Texture_dispose(rbTextTexture);
  return self;
}

static void
RenderTextureWithOptions(const Texture* srcTexture, const Texture* dstTexture,
                         int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY,
                         const RenderingTextureOptions* options)
{
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
  const int dstTextureWidth  = dstTexture->width;
  const int dstTextureHeight = dstTexture->height;
  if (dstTextureWidth <= dstX0 || dstTextureHeight <= dstY0 ||
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
  const int dstWidth  = MIN(dstTextureWidth,  (int)dstX1) - dstX0Int;
  const int dstHeight = MIN(dstTextureHeight, (int)dstY1) - dstY0Int;

  const int_fast32_t srcOX16  = (int_fast32_t)(srcOX  * (1 << 16));
  const int_fast32_t srcOY16  = (int_fast32_t)(srcOY  * (1 << 16));
  const int_fast32_t srcDXX16 = (int_fast32_t)(srcDXX * (1 << 16));
  const int_fast32_t srcDXY16 = (int_fast32_t)(srcDXY * (1 << 16));
  const int_fast32_t srcDYX16 = (int_fast32_t)(srcDYX * (1 << 16));
  const int_fast32_t srcDYY16 = (int_fast32_t)(srcDYY * (1 << 16));

  Texture* clonedTexture = NULL;
  if (srcTexture == dstTexture) {
    clonedTexture = ALLOC(Texture);
    clonedTexture->pixels      = NULL;
    clonedTexture->width  = dstTexture->width;
    clonedTexture->height = dstTexture->height;
    const int length = dstTexture->width * dstTexture->height;
    clonedTexture->pixels = ALLOC_N(Pixel, length);
    MEMCPY(clonedTexture->pixels, dstTexture->pixels, Pixel, length);
    srcTexture = clonedTexture;
  }

  const int srcX2 = srcX + srcWidth;
  const int srcY2 = srcY + srcHeight;
  const int srcTextureWidth = srcTexture->width;
  const uint8_t alpha       = options->alpha;
  const BlendType blendType = options->blendType;
  const int saturation      = options->tone.saturation;
  const int toneRed         = options->tone.red;
  const int toneGreen       = options->tone.green;
  const int toneBlue        = options->tone.blue;

  for (int j = 0; j < dstHeight; j++) {
    int_fast32_t srcI16 = srcOX16 + j * srcDYX16;
    int_fast32_t srcJ16 = srcOY16 + j * srcDYY16;
    Pixel* dst =
      &(dstTexture->pixels[dstX0Int + (dstY0Int + j) * dstTextureWidth]);
    for (int i = 0; i < dstWidth;
         i++, dst++, srcI16 += srcDXX16, srcJ16 += srcDXY16) {
      const int_fast32_t srcI = srcI16 >> 16;
      const int_fast32_t srcJ = srcJ16 >> 16;
      if (srcX <= srcI && srcI < srcX2 && srcY <= srcJ && srcJ < srcY2) {
        const Color srcColor =
          srcTexture->pixels[srcI + srcJ * srcTextureWidth].color;
        if (blendType == BLEND_TYPE_MASK) {
          dst->color.alpha = srcColor.red;
        } else {
          uint8_t srcRed   = srcColor.red;
          uint8_t srcGreen = srcColor.green;
          uint8_t srcBlue  = srcColor.blue;
          uint8_t srcAlpha = srcColor.alpha;
          if (saturation < 255) {
            // http://www.poynton.com/ColorFAQ.html
            const uint8_t y =
              (6969 * srcRed + 23434 * srcGreen + 2365 * srcBlue) / 32768;
            srcRed   = ALPHA(srcRed,   y, saturation);
            srcGreen = ALPHA(srcGreen, y, saturation);
            srcBlue  = ALPHA(srcBlue,  y, saturation);
          }
          if (toneRed) {
            if (0 < toneRed) {
              srcRed = ALPHA(255, srcRed, toneRed);
            } else {
              srcRed = ALPHA(0,   srcRed, -toneRed);
            }
          }
          if (toneGreen) {
            if (0 < toneGreen) {
              srcGreen = ALPHA(255, srcGreen, toneGreen);
            } else {
              srcGreen = ALPHA(0,   srcGreen, -toneGreen);
            }
          }
          if (toneBlue) {
            if (0 < toneBlue) {
              srcBlue = ALPHA(255, srcBlue, toneBlue);
            } else {
              srcBlue = ALPHA(0,   srcBlue, -toneBlue);
            }
          }
          if (blendType == BLEND_TYPE_NONE) {
            dst->color.red   = srcRed;
            dst->color.green = srcGreen;
            dst->color.blue  = srcBlue;
            dst->color.alpha = srcAlpha;
          } else if (dst->color.alpha == 0) {
            const uint8_t beta = DIV255(srcAlpha * alpha);
            switch (blendType) {
            case BLEND_TYPE_ALPHA:
              dst->color.red   = srcRed;
              dst->color.green = srcGreen;
              dst->color.blue  = srcBlue;
              dst->color.alpha = beta;
              break;
            case BLEND_TYPE_ADD:
              {
                const int addR = srcRed   + dst->color.red;
                const int addG = srcGreen + dst->color.green;
                const int addB = srcBlue  + dst->color.blue;
                dst->color.red   = MIN(255, addR);
                dst->color.green = MIN(255, addG);
                dst->color.blue  = MIN(255, addB);
                dst->color.alpha = beta;
              }
              break;
            case BLEND_TYPE_SUB:
              {
                const int subR = -srcRed   + dst->color.red;
                const int subG = -srcGreen + dst->color.green;
                const int subB = -srcBlue  + dst->color.blue;
                dst->color.red   = MAX(0, subR);
                dst->color.green = MAX(0, subG);
                dst->color.blue  = MAX(0, subB);
                dst->color.alpha = beta;
              }
              break;
            case BLEND_TYPE_MASK:
              assert(false);
              break;
            case BLEND_TYPE_NONE:
              assert(false);
              break;
            }
          } else {
            const uint8_t beta = DIV255(srcAlpha * alpha);
            if (dst->color.alpha < beta) {
              dst->color.alpha = beta;
            }
            switch (blendType) {
            case BLEND_TYPE_ALPHA:
              dst->color.red   = ALPHA(srcRed,   dst->color.red,   beta);
              dst->color.green = ALPHA(srcGreen, dst->color.green, beta);
              dst->color.blue  = ALPHA(srcBlue,  dst->color.blue,  beta);
              break;
            case BLEND_TYPE_ADD:
              {
                const int addR = DIV255(srcRed   * beta) + dst->color.red;
                const int addG = DIV255(srcGreen * beta) + dst->color.green;
                const int addB = DIV255(srcBlue  * beta) + dst->color.blue;
                dst->color.red   = MIN(255, addR);
                dst->color.green = MIN(255, addG);
                dst->color.blue  = MIN(255, addB);
              }
              break;
            case BLEND_TYPE_SUB:
              {
                const int subR = -DIV255(srcRed   * beta) + dst->color.red;
                const int subG = -DIV255(srcGreen * beta) + dst->color.green;
                const int subB = -DIV255(srcBlue  * beta) + dst->color.blue;
                dst->color.red   = MAX(0, subR);
                dst->color.green = MAX(0, subG);
                dst->color.blue  = MAX(0, subB);
              }
              break;
            case BLEND_TYPE_MASK:
              assert(false);
              break;
            case BLEND_TYPE_NONE:
              assert(false);
              break;
            }
          }
        }
      } else if ((srcI < srcX && srcDXX <= 0) ||
                 (srcX2 <= srcI && 0 <= srcDXX) ||
                 (srcJ < srcY && srcDXY <= 0) ||
                 (srcY2 <= srcJ && 0 <= srcDXY)) {
        break;
      }
    }
  }
  if (clonedTexture) {
    Texture_free(clonedTexture);
    clonedTexture = NULL;
  }
}

static VALUE
Texture_render_texture(int argc, VALUE* argv, VALUE self)
{
  rb_check_frozen(self);
  const Texture* dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);
  strb_CheckDisposedTexture(dstTexture);

  volatile VALUE rbTexture, rbX, rbY, rbOptions;
  if (3 <= argc && argc <= 4) {
    rbTexture = argv[0];
    rbX       = argv[1];
    rbY       = argv[2];
    rbOptions = (argc == 4) ? argv[3] : Qnil;
  } else {
    rb_scan_args(argc, argv, "31", &rbTexture, &rbX, &rbY, &rbOptions);
  }

  strb_CheckTexture(rbTexture);
  const Texture* srcTexture;
  Data_Get_Struct(rbTexture, Texture, srcTexture);
  strb_CheckDisposedTexture(srcTexture);

  const int srcTextureWidth  = srcTexture->width;
  const int srcTextureHeight = srcTexture->height;
  RenderingTextureOptions options = {
    .srcX         = 0,
    .srcY         = 0,
    .srcWidth     = srcTextureWidth,
    .srcHeight    = srcTextureHeight,
    .scaleX       = 1,
    .scaleY       = 1,
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
  };
  if (!SPECIAL_CONST_P(rbOptions) && BUILTIN_TYPE(rbOptions) == T_HASH) {
    if (NIL_P(RHASH_IFNONE(rbOptions))) {
      st_table* table = RHASH_TBL(rbOptions);
      if (0 < table->num_entries) {
        volatile VALUE val;
        st_foreach(table, AssignRenderingTextureOptions, (st_data_t)&options);
        if (!st_lookup(table, (st_data_t)symbol_src_width, (st_data_t*)&val)) {
          options.srcWidth = srcTextureWidth - options.srcX;
        }
        if (!st_lookup(table, (st_data_t)symbol_src_height, (st_data_t*)&val)) {
          options.srcHeight = srcTextureHeight - options.srcY;
        }
      }
    } else {
      volatile VALUE val;
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_x))) {
        options.srcX = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_y))) {
        options.srcY = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_width))) {
        options.srcWidth = NUM2INT(val);
      } else {
        options.srcWidth = srcTextureWidth - options.srcX;
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_height))) {
        options.srcHeight = NUM2INT(val);
      } else {
        options.srcHeight = srcTextureHeight - options.srcY;
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_x))) {
        options.scaleX = NUM2DBL(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_y))) {
        options.scaleY = NUM2DBL(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_angle))) {
        options.angle = NUM2DBL(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_center_x))) {
        options.centerX = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_center_y))) {
        options.centerY = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_matrix))) {
        ASSIGN_MATRIX((&options), val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_alpha))) {
        options.alpha = NUM2DBL(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_blend_type))) {
        Check_Type(val, T_SYMBOL);
        if (val == symbol_none) {
          options.blendType = BLEND_TYPE_NONE;
        } else if (val == symbol_alpha) {
          options.blendType = BLEND_TYPE_ALPHA;
        } else if (val == symbol_add) {
          options.blendType = BLEND_TYPE_ADD;
        } else if (val == symbol_sub) {
          options.blendType = BLEND_TYPE_SUB;
        } else if (val == symbol_mask) {
          options.blendType = BLEND_TYPE_MASK;
        }
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone_red))) {
        options.tone.red = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone_green))) {
        options.tone.green = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone_blue))) {
        options.tone.blue = NUM2INT(val);
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_saturation))) {
        options.tone.saturation = NUM2INT(val);
      }
    }
  } else if (!NIL_P(rbOptions)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Hash)",
             rb_obj_classname(rbOptions));
  }

  options.tone.red = options.tone.red;
  options.tone.green = options.tone.green;
  options.tone.blue = options.tone.blue;
  options.tone.saturation = options.tone.saturation;

  //strb_CheckToneRange(&options.tone);

  const int saturation = options.tone.saturation;
  const int toneRed    = options.tone.red;
  const int toneGreen  = options.tone.green;
  const int toneBlue   = options.tone.blue;

  if (toneRed   < -255 || 255 < toneRed   ||
      toneGreen < -255 || 255 < toneGreen ||
      toneBlue  < -255 || 255 < toneBlue  ||
      saturation < 0   || 255 < saturation) {
    rb_raise(rb_eArgError, "invalid tone value: (r:%d, g:%d, b:%d, s:%d)",
             toneRed, toneGreen, toneBlue, saturation);
  }
  int srcX      = options.srcX;
  int srcY      = options.srcY;
  int srcWidth  = options.srcWidth;
  int srcHeight = options.srcHeight;
  const AffineMatrix* matrix = &(options.matrix);
  if (!ModifyRectInTexture(srcTexture,
                           &(srcX), &(srcY), &(srcWidth), &(srcHeight))) {
    return self;
  }
  if (srcTexture != dstTexture &&
      (matrix->a == 1 && matrix->b == 0 && matrix->c == 0 && matrix->d == 1) &&
      (options.scaleX == 1 && options.scaleY == 1 && options.angle == 0 &&
       toneRed == 0 && toneGreen == 0 && toneBlue == 0 && saturation == 255 &&
       (options.blendType == BLEND_TYPE_ALPHA || options.blendType == BLEND_TYPE_NONE))) {
    RenderTexture(srcTexture, dstTexture,
                  srcX, srcY, srcWidth, srcHeight, NUM2INT(rbX), NUM2INT(rbY),
                  options.alpha, options.blendType);
  } else {
    RenderTextureWithOptions(srcTexture, dstTexture,
                             srcX, srcY, srcWidth, srcHeight, NUM2INT(rbX), NUM2INT(rbY),
                             &options);
  }
  return self;
}

static VALUE
Texture_size(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  volatile VALUE rbSize =
    rb_assoc_new(INT2NUM(texture->width), INT2NUM(texture->height));
  OBJ_FREEZE(rbSize);
  return rbSize;
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

static VALUE
Texture_undump(VALUE self, VALUE rbData, VALUE rbFormat)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);

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
  for (int i = 0; i < pixelLength; i++, pixels++) {
    for (int j = 0; j < formatLength; j++, data++) {
      switch (format[j]) {
      case 'r': pixels->color.red   = *data; break;
      case 'g': pixels->color.green = *data; break;
      case 'b': pixels->color.blue  = *data; break;
      case 'a': pixels->color.alpha = *data; break;
      }
    }
  }
  return self;
}

static VALUE
Texture_width(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  return INT2NUM(texture->width);
}

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
  VALUE rbAlpha, VALUE rbBlendType)
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

  int dstX = NUM2INT(rbDstX);
  int dstY = NUM2INT(rbDstY);

  int srcX = NUM2INT(rbSrcX);
  int srcY = NUM2INT(rbSrcY);
  int srcWidth = NUM2INT(rbSrcWidth);
  int srcHeight = NUM2INT(rbSrcHeight);

  const uint8_t alpha = MAX(MIN(NUM2DBL(rbAlpha), 255.0), 0.0);
  const int blendType = NUM2INT(rbBlendType);

  if (!ModifyRectInTexture(srcTexture,
                           &(srcX), &(srcY), &(srcWidth), &(srcHeight))) {
    return Qnil;
  }

  RenderTexture(
    srcTexture, dstTexture,
    srcX, srcY, srcWidth, srcHeight, dstX, dstY,
    alpha, blendType);

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
    TextureTool_render_texture_fast, 10);
  rb_define_singleton_method(rb_mTextureTool, "color_blend",
                   TextureTool_color_blend, 2);
  rb_define_singleton_method(rb_mTextureTool, "clipping_mask",
                   TextureTool_clipping_mask, 8);

  return rb_mTextureTool;
}

static VALUE
Texture_rect(VALUE self)
{
  Texture *texture;
  Data_Get_Struct(self, Texture, texture);
  VALUE rbArgv[4] = { INT2NUM(0), INT2NUM(0),
                      INT2NUM(texture->width), INT2NUM(texture->height) };
  return rb_class_new_instance(4, rbArgv, strb_GetRectClass());
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
  symbol_tone_blue      = ID2SYM(rb_intern("tone_blue"));
  symbol_tone_green     = ID2SYM(rb_intern("tone_green"));
  symbol_tone_red       = ID2SYM(rb_intern("tone_red"));
  symbol_view_angle     = ID2SYM(rb_intern("view_angle"));
  symbol_width          = ID2SYM(rb_intern("width"));
  symbol_x              = ID2SYM(rb_intern("x"));
  symbol_y              = ID2SYM(rb_intern("y"));

  ID_expand_path    = rb_intern("expand_path");
  ID_extname        = rb_intern("extname");

  return rb_cTexture;
}
