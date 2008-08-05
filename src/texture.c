#include <assert.h>
#include "starruby.h"
#include "starruby_private.h"
#include <png.h>
#include "st.h"

#define ALPHA(src, dst, a) DIV255((dst << 8) - dst + (src - dst) * a)

#define LOOP(process, length) \
  do {                        \
    int n = (length + 7) / 8; \
    switch (length % 8) {     \
    case 0: do { process;     \
      case 7: process;        \
      case 6: process;        \
      case 5: process;        \
      case 4: process;        \
      case 3: process;        \
      case 2: process;        \
      case 1: process;        \
      } while (--n > 0);      \
    }                         \
  } while (false)

static volatile VALUE rb_cTexture;

static volatile VALUE symbol_add;
static volatile VALUE symbol_alpha;
static volatile VALUE symbol_angle;
static volatile VALUE symbol_background;
static volatile VALUE symbol_blend_type;
static volatile VALUE symbol_blur;
static volatile VALUE symbol_camera_height;
static volatile VALUE symbol_camera_pitch;
static volatile VALUE symbol_camera_roll;
static volatile VALUE symbol_camera_x;
static volatile VALUE symbol_camera_y;
static volatile VALUE symbol_camera_yaw;
static volatile VALUE symbol_center_x;
static volatile VALUE symbol_center_y;
static volatile VALUE symbol_height;
static volatile VALUE symbol_intersection_x;
static volatile VALUE symbol_intersection_y;
static volatile VALUE symbol_io_length;
static volatile VALUE symbol_loop;
static volatile VALUE symbol_mask;
static volatile VALUE symbol_none;
static volatile VALUE symbol_palette;
static volatile VALUE symbol_saturation;
static volatile VALUE symbol_scale_x;
static volatile VALUE symbol_scale_y;
static volatile VALUE symbol_src_height;
static volatile VALUE symbol_src_width;
static volatile VALUE symbol_src_x;
static volatile VALUE symbol_src_y;
static volatile VALUE symbol_sub;
static volatile VALUE symbol_tone_blue;
static volatile VALUE symbol_tone_green;
static volatile VALUE symbol_tone_red;
static volatile VALUE symbol_view_angle;
static volatile VALUE symbol_width;
static volatile VALUE symbol_x;
static volatile VALUE symbol_y;

typedef enum {
  BLEND_TYPE_NONE,
  BLEND_TYPE_ALPHA,
  BLEND_TYPE_ADD,
  BLEND_TYPE_SUB,
  BLEND_TYPE_MASK,
} BlendType;

typedef enum {
  BLUR_TYPE_NONE,
  BLUR_TYPE_COLOR,
  BLUR_TYPE_BACKGROUND,
} BlurType;

typedef struct {
  double a, b, c, d, tx, ty;
} AffineMatrix;

typedef struct {
  int x, y, z;
} Point;

typedef struct {
  double x, y, z;
} PointF;

typedef Point Vector;
typedef PointF VectorF;

typedef struct {
  int cameraX;
  int cameraY;
  double cameraHeight;
  double cameraYaw;
  double cameraPitch;
  double cameraRoll;
  double viewAngle;
  int intersectionX;
  int intersectionY;
  bool isLoop;
  BlurType blurType;
  Color blurColor;
} PerspectiveOptions;

typedef struct {
  double angle;
  double scaleX;
  double scaleY;
  int centerX;
  int centerY;
  int srcHeight;
  int srcWidth;
  int srcX;
  int srcY;
  int toneRed;
  int toneGreen;
  int toneBlue;
  int saturation;
  BlendType blendType;
  uint8_t alpha;
} RenderingTextureOptions;

VALUE
strb_GetTextureClass(void)
{
  return rb_cTexture;
}

inline static void
CheckDisposed(Texture* texture)
{
  if (!texture->pixels)
    rb_raise(rb_eRuntimeError,
             "can't modify disposed StarRuby::Texture");
}

inline static void
CheckPalette(Texture* texture)
{
  if (texture->palette)
    rb_raise(strb_GetStarRubyErrorClass(),
             "can't modify a texture with a palette");
}

inline static bool
ModifyRectInTexture(Texture* texture, int* x, int* y, int* width, int* height)
{
  if (*x < 0) {
    *width -= -(*x);
    *x = 0;
  }
  if (*y < 0) {
    *height -= -(*y);
    *y = 0;
  }
  if (texture->width <= *x || texture->height <= *y)
    return false;
  if (texture->width <= *x + *width)
    *width = texture->width - *x;
  if (texture->height <= *y + *height)
    *height = texture->height - *y;
  if (*width <= 0 || *height <= 0)
    return false;
  return true;
}

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
Texture_s_load(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbPathOrIO, rbOptions;
  rb_scan_args(argc, argv, "11", &rbPathOrIO, &rbOptions);
  if (NIL_P(rbOptions))
    rbOptions = rb_hash_new();
  bool hasPalette = RTEST(rb_hash_aref(rbOptions, symbol_palette));
  unsigned long ioLength = 0;
  volatile VALUE val;
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_io_length))) {
    if (RTEST(rb_obj_is_kind_of(val, rb_cNumeric))) {
      if (RTEST(rb_funcall(val, rb_intern("<="), 1, INT2FIX(0))))
        rb_raise(rb_eArgError, "invalid io_length");
      ioLength = NUM2ULONG(val);
      if (ioLength <= 8)
        rb_raise(rb_eArgError, "invalid io_length");
    } else {
      rb_raise(rb_eTypeError, "wrong argument type %s (expected Numeric)",
               rb_obj_classname(rbPathOrIO));
    }
  }

  png_structp pngPtr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pngPtr)
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
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
    volatile VALUE rbOpenOption =rb_str_new2("rb"); 
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
    if (!NIL_P(rbIOToClose))
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG file (none header)");
  }
  png_byte header[8];
  MEMCPY(header, StringValuePtr(rbHeader), png_byte, 8);
  if (png_sig_cmp(header, 0, 8)) {
    if (!NIL_P(rbIOToClose))
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
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
    if (!NIL_P(rbIOToClose))
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    rb_raise(strb_GetStarRubyErrorClass(),
             "not supported interlacing PNG image");
  }

  volatile VALUE rbTexture =
    rb_class_new_instance(2, (VALUE[]){INT2NUM(width), INT2NUM(height)}, self);
  Texture* texture;
  Data_Get_Struct(rbTexture, Texture, texture);

  if (bitDepth == 16)
    png_set_strip_16(pngPtr);
  if (colorType == PNG_COLOR_TYPE_PALETTE && !hasPalette) {
    png_set_palette_to_rgb(pngPtr);
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(pngPtr);
  }
  if (bitDepth < 8)
    png_set_packing(pngPtr);
  if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
    png_set_gray_1_2_4_to_8(pngPtr);
  png_read_update_info(pngPtr, infoPtr);
  if (0 < infoPtr->num_palette && hasPalette) {
    texture->indexes = ALLOC_N(uint8_t, width * height);
    png_colorp palette = infoPtr->palette;
    int numTrans = infoPtr->num_trans;
    png_bytep trans = infoPtr->trans;
    texture->paletteSize = infoPtr->num_palette;
    Color* p = texture->palette = ALLOC_N(Color, texture->paletteSize);
    for (int i = 0; i < texture->paletteSize; i++, p++) {
      png_colorp pngColorP = &(palette[i]);
      p->red   = pngColorP->red;
      p->green = pngColorP->green;
      p->blue  = pngColorP->blue;
      p->alpha = 0xff;
      for (int j = 0; j < numTrans; j++) {
        if (i == trans[j]) {
          p->alpha = 0;
          break;
        }
      }
    }
  }
  int channels = png_get_channels(pngPtr, infoPtr);
  Color* palette = texture->palette;
  uint8_t* indexes = texture->indexes;
  for (unsigned int j = 0; j < height; j++) {
    png_byte row[width * channels];
    png_read_row(pngPtr, row, NULL);
    for (unsigned int i = 0; i < width; i++, indexes++) {
      Color* c = &(texture->pixels[width * j + i].color);
      switch (channels) {
      case 1:
        *c = palette[*indexes = row[i]];
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
  if (!NIL_P(rbIOToClose))
    rb_funcall(rbIOToClose, rb_intern("close"), 0);
  return rbTexture;
}

static void
Texture_free(Texture* texture)
{
  free(texture->pixels);
  texture->pixels = NULL;
  free(texture->palette);
  texture->palette = NULL;
  free(texture->indexes);
  texture->indexes = NULL;
  free(texture);
}

static VALUE
Texture_alloc(VALUE klass)
{
  Texture* texture = ALLOC(Texture);
  texture->pixels = NULL;
  texture->paletteSize = 0;
  texture->palette = NULL;
  texture->indexes = NULL;
  return Data_Wrap_Struct(klass, 0, Texture_free, texture);
}

static VALUE
Texture_initialize(VALUE self, VALUE rbWidth, VALUE rbHeight)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  int width  = NUM2INT(rbWidth);
  int height = NUM2INT(rbHeight);
  if (width <= 0)
    rb_raise(rb_eArgError, "width less than or equal to 0");
  if (height <= 0)
    rb_raise(rb_eArgError, "height less than or equal to 0");
  texture->width  = width;
  texture->height = height;
  texture->pixels = ALLOC_N(Pixel, texture->width * texture->height);
  MEMZERO(texture->pixels, Pixel, texture->width * texture->height);
  return Qnil;
}

static VALUE
Texture_initialize_copy(VALUE self, VALUE rbTexture)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  Texture* origTexture;
  Data_Get_Struct(rbTexture, Texture, origTexture);
  texture->width  = origTexture->width;
  texture->height = origTexture->height;
  int length = texture->width * texture->height;
  texture->pixels = ALLOC_N(Pixel, length);
  MEMCPY(texture->pixels, origTexture->pixels, Pixel, length);
  if (origTexture->palette) {
    int paletteSize = texture->paletteSize = origTexture->paletteSize;
    texture->palette = ALLOC_N(Color, paletteSize);
    MEMCPY(texture->palette, origTexture->palette, Color, paletteSize);
    int length = texture->width * texture->height;
    texture->indexes = ALLOC_N(uint8_t, length);
    MEMCPY(texture->indexes, origTexture->indexes, uint8_t, length);
  }
  return Qnil;
}

static void
AssignPerspectiveOptions(PerspectiveOptions* options, VALUE rbOptions)
{
  volatile VALUE val;
  Check_Type(rbOptions, T_HASH);
  MEMZERO(options, PerspectiveOptions, 1);
  options->viewAngle = PI / 4;
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_x)))
    options->cameraX = NUM2INT(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_y)))
    options->cameraY = NUM2INT(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_height)))
    options->cameraHeight = NUM2DBL(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_yaw)))
    options->cameraYaw = NUM2DBL(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_pitch)))
    options->cameraPitch = NUM2DBL(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_camera_roll)))
    options->cameraRoll = NUM2DBL(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_view_angle))) {
    options->viewAngle = NUM2DBL(val);
    if (!isfinite(options->viewAngle))
      rb_raise(rb_eArgError, "invalid :view_angle value");
    if (options->viewAngle <= 0 || PI <= options->viewAngle)
      rb_raise(rb_eArgError, "invalid :view_angle value");
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_intersection_x)))
    options->intersectionX = NUM2INT(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_intersection_y)))
    options->intersectionY = NUM2INT(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_loop)))
    options->isLoop = RTEST(val);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_blur))) {
    switch (TYPE(val)) {
    case T_DATA:
      options->blurType = BLUR_TYPE_COLOR;
      Color color;
      strb_GetColorFromRubyValue(&color, val);
      options->blurColor = color;
      break;
    case T_SYMBOL:
      if (val == symbol_background)
        options->blurType = BLUR_TYPE_BACKGROUND;
      else
        options->blurType = BLUR_TYPE_NONE;
      break;
    default:
      rb_raise(rb_eTypeError,
               "wrong argument type %s (expected Color or Symbol)",
               rb_obj_classname(val));
      break;
    }
  }
}

static VALUE Texture_change_hue_bang(VALUE, VALUE);
static VALUE
Texture_change_hue(VALUE self, VALUE rbAngle)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  volatile VALUE rbTexture = rb_funcall(self, rb_intern("dup"), 0);
  Texture* newTexture;
  Data_Get_Struct(rbTexture, Texture, newTexture);
  Texture_change_hue_bang(rbTexture, rbAngle);
  return rbTexture;
}

static inline void
ChangeHue(Color* color, double angle)
{
  uint8_t r = color->red;
  uint8_t g = color->green;
  uint8_t b = color->blue;
  uint8_t max = MAX(MAX(r, g), b);
  uint8_t min = MIN(MIN(r, g), b);
  if (max != 0) {
    double delta255 = max - min;
    double v = max / 255.0;
    double s = delta255 / max;
    double h;
    if (max == r)
      h =     (g - b) / delta255;
    else if (max == g)
      h = 2 + (b - r) / delta255;
    else
      h = 4 + (r - g) / delta255;
    if (h < 0.0)
      h += 6.0;
    h += angle * 6.0 / (2 * PI);
    if (6.0 <= h)
      h -= 6.0;
    int ii = (int)h;
    double f = h - ii;
    uint8_t v255 = max;
    uint8_t aa255 = (uint8_t)(v * (1 - s) * 255);
    uint8_t bb255 = (uint8_t)(v * (1 - s * f) * 255);
    uint8_t cc255 = (uint8_t)(v * (1 - s * (1 - f)) * 255);
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
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  double angle = NUM2DBL(rbAngle);
  if (angle == 0)
    return Qnil;
  Pixel* pixels = texture->pixels;
  if (!texture->palette) {
    int length = texture->width * texture->height;
    for (int i = 0; i < length; i++, pixels++)
      ChangeHue(&(pixels->color), angle);
  } else {
    int paletteSize = texture->paletteSize;
    Color* palette = texture->palette;
    for (int i = 0; i < paletteSize; i++, palette++)
      ChangeHue(palette, angle);
    palette = texture->palette;
    uint8_t* indexes = texture->indexes;
    int length = texture->width * texture->height;
    for (int i = 0; i < length; i++, pixels++, indexes++)
      pixels->color = palette[*indexes];
  }
  return Qnil;
}

static VALUE Texture_change_palette_bang(VALUE, VALUE);
static VALUE
Texture_change_palette(VALUE self, VALUE rbPalette)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  volatile VALUE rbTexture = rb_funcall(self, rb_intern("dup"), 0);
  Texture_change_palette_bang(rbTexture, rbPalette);
  return rbTexture;
}

static VALUE
Texture_change_palette_bang(VALUE self, VALUE rbPalette)
{
  rb_check_frozen(self);
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  if (!texture->palette)
    rb_raise(strb_GetStarRubyErrorClass(), "no palette texture");
  Check_Type(rbPalette, T_ARRAY);
  VALUE* rbColors = RARRAY_PTR(rbPalette);
  Color* palette = texture->palette;
  for (int i = 0; i < texture->paletteSize; i++, palette++)
    if (i < RARRAY_LEN(rbPalette))
      strb_GetColorFromRubyValue(palette, rbColors[i]);
    else
      *palette = (Color){0, 0, 0, 0};
  Pixel* pixels = texture->pixels;
  palette = texture->palette;
  uint8_t* indexes = texture->indexes;
  int length = texture->width * texture->height;
  for (int i = 0; i < length; i++, pixels++, indexes++)
    pixels->color = palette[*indexes];
  return Qnil;
}

static VALUE
Texture_clear(VALUE self)
{
  rb_check_frozen(self);
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  MEMZERO(texture->pixels, Color, texture->width * texture->height);
  return self;
}

static VALUE
Texture_dispose(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  free(texture->pixels);
  texture->pixels = NULL;
  free(texture->palette);
  texture->palette = NULL;
  free(texture->indexes);
  texture->indexes = NULL;
  return Qnil;
}

static VALUE
Texture_disposed(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  return !texture->pixels ? Qtrue : Qfalse;
}

static VALUE
Texture_dump(VALUE self, VALUE rbFormat)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  char* format = StringValuePtr(rbFormat);
  int formatLength = RSTRING_LEN(rbFormat);
  int pixelLength = texture->width * texture->height;
  volatile VALUE rbResult = rb_str_new(NULL, pixelLength * formatLength);
  uint8_t* strPtr = (uint8_t*)RSTRING_PTR(rbResult);
  Pixel* pixels = texture->pixels;
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
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  int length = texture->width * texture->height;
  Pixel* pixels = texture->pixels;
  for (int i = 0; i < length; i++, pixels++)
    pixels->color = color;
  return self;
}

static VALUE
Texture_fill_rect(VALUE self, VALUE rbX, VALUE rbY,
                  VALUE rbWidth, VALUE rbHeight, VALUE rbColor)
{
  rb_check_frozen(self);
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  int rectX = NUM2INT(rbX);
  int rectY = NUM2INT(rbY);
  int rectWidth  = NUM2INT(rbWidth);
  int rectHeight = NUM2INT(rbHeight);
  if (!ModifyRectInTexture(texture, &rectX, &rectY, &rectWidth, &rectHeight))
    return self;
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);  
  Pixel* pixels = &(texture->pixels[rectX + rectY * texture->width]);
  int paddingJ = texture->width - rectWidth;
  for (int j = rectY; j < rectY + rectHeight; j++, pixels += paddingJ)
    for (int i = rectX; i < rectX + rectWidth; i++, pixels++)
      pixels->color = color;
  return self;
}

static VALUE
Texture_get_pixel(VALUE self, VALUE rbX, VALUE rbY)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  int x = NUM2INT(rbX);
  int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y)
    rb_raise(rb_eArgError, "index out of range: (%d, %d)", x, y);
  Color color = texture->pixels[x + y * texture->width].color;
  return rb_funcall(strb_GetColorClass(), rb_intern("new"), 4,
                    INT2FIX(color.red),
                    INT2FIX(color.green),
                    INT2FIX(color.blue),
                    INT2FIX(color.alpha));
}

static VALUE
Texture_height(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  return INT2NUM(texture->height);
}

static VALUE
Texture_palette(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  if (texture->palette) {
    volatile VALUE rbArray = rb_ary_new2(texture->paletteSize);
    Color* colors = texture->palette;
    volatile VALUE rb_cColor = strb_GetColorClass();
    for (int i = 0; i < texture->paletteSize; i++, colors++) {
      VALUE rbArgs[] = {
        INT2FIX(colors->red),
        INT2FIX(colors->green),
        INT2FIX(colors->blue),
        INT2FIX(colors->alpha),
      };
      volatile VALUE rbColor =
        rb_class_new_instance(sizeof(rbArgs) / sizeof(VALUE), rbArgs, rb_cColor);
      rb_ary_push(rbArray, rbColor);
    }
    OBJ_FREEZE(rbArray);
    return rbArray;
  } else {
    return Qnil;
  }
}

#define RENDER_PIXEL(_dst, _src)                              \
  do {                                                        \
    if (_dst.alpha == 0) {                                    \
      _dst = _src;                                            \
    } else {                                                  \
      if (_dst.alpha < _src.alpha)                            \
        _dst.alpha = _src.alpha;                              \
      _dst.red   = ALPHA(_src.red,   _dst.red,   _src.alpha); \
      _dst.green = ALPHA(_src.green, _dst.green, _src.alpha); \
      _dst.blue  = ALPHA(_src.blue,  _dst.blue,  _src.alpha); \
    }                                                         \
  } while (false)

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
  if (NIL_P(rbOptions))
    rbOptions = rb_hash_new();
  Texture* srcTexture;
  Data_Get_Struct(rbTexture, Texture, srcTexture);
  CheckDisposed(srcTexture);
  Texture* dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);
  CheckDisposed(dstTexture);
  CheckPalette(dstTexture);
  if (srcTexture == dstTexture)
    rb_raise(rb_eRuntimeError, "can't render self in perspective");
  PerspectiveOptions options;
  AssignPerspectiveOptions(&options, rbOptions);
  if (options.cameraHeight == 0)
    return self;
  int srcWidth  = srcTexture->width;
  int srcHeight = srcTexture->height;
  int dstWidth  = dstTexture->width;
  int dstHeight = dstTexture->height;
  double cosYaw   = cos(options.cameraYaw);
  double sinYaw   = sin(options.cameraYaw);
  double cosPitch = cos(options.cameraPitch);
  double sinPitch = sin(options.cameraPitch);
  double cosRoll  = cos(options.cameraRoll);
  double sinRoll  = sin(options.cameraRoll);
  VectorF screenDX = {
    cosRoll * cosYaw + sinRoll * sinPitch * sinYaw,
    sinRoll * -cosPitch,
    cosRoll * sinYaw - sinRoll * sinPitch * cosYaw,
  };
  VectorF screenDY = {
    -sinRoll * cosYaw + cosRoll * sinPitch * sinYaw,
    cosRoll * -cosPitch,
    -sinRoll * sinYaw - cosRoll * sinPitch * cosYaw,
  };
  double distance = dstWidth / (2 * (tan(options.viewAngle / 2)));
  PointF intersection = {
    distance * (cosPitch * sinYaw),
    distance * sinPitch + options.cameraHeight,
    distance * (-cosPitch * cosYaw),
  };
  PointF screenO = {
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
  int cameraHeight = (int)options.cameraHeight;
  Pixel* src = srcTexture->pixels;
  Pixel* dst = dstTexture->pixels;
  PointF screenP;
  for (int j = 0; j < dstHeight; j++) {
    screenP.x = screenO.x + j * screenDY.x;
    screenP.y = screenO.y + j * screenDY.y;
    screenP.z = screenO.z + j * screenDY.z;
    LOOP({
        if (cameraHeight != screenP.y &&
            ((0 < cameraHeight && screenP.y < cameraHeight) ||
             (cameraHeight < 0 && cameraHeight < screenP.y))) {
          double scale = cameraHeight / (cameraHeight - screenP.y);
          int srcX = (int)((screenP.x) * scale + options.cameraX);
          int srcZ = (int)((screenP.z) * scale + options.cameraY);
          if (options.isLoop) {
            srcX %= srcWidth;
            if (srcX < 0)
              srcX += srcWidth;
            srcZ %= srcHeight;
            if (srcZ < 0)
              srcZ += srcHeight;
          }
          if (options.isLoop ||
              (0 <= srcX && srcX < srcWidth && 0 <= srcZ && srcZ < srcHeight)) {
            Color* srcColor = &(src[srcX + srcZ * srcWidth].color);
            if (options.blurType == BLUR_TYPE_NONE || scale <= 1) {
              RENDER_PIXEL(dst->color, (*srcColor));
            } else {
              int rate = (int)(255 * (1 / scale));
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
  int x1 = NUM2INT(rbX1);
  int y1 = NUM2INT(rbY1);
  int x2 = NUM2INT(rbX2);
  int y2 = NUM2INT(rbY2);
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  int x = x1;
  int y = y1;
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int signX = (x1 <= x2) ? 1 : -1;
  int signY = (y1 <= y2) ? 1 : -1;
  if (dy <= dx) {
    int e = dx;
    int eLimit = dx * 2;
    for (int i = 0; i <= dx; i++) {
      if (0 <= x && x < texture->width && 0 <= y && y < texture->height) {
        Pixel* pixel = &(texture->pixels[x + y * texture->width]);
        RENDER_PIXEL(pixel->color, color);
      }
      x += signX;
      e += 2 * dy;
      if (eLimit <= e) {
        e -= eLimit;
        y += signY;
      }
    }
  } else {
    int e = dy;
    int eLimit = dy * 2;
    for (int i = 0; i <= dy; i++) {
      if (0 <= x && x < texture->width && 0 <= y && y < texture->height) {
        Pixel* pixel = &(texture->pixels[x + y * texture->width]);
        RENDER_PIXEL(pixel->color, color);
      }
      y += signY;
      e += 2 * dx;
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
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  int x = NUM2INT(rbX);
  int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y)
    return Qnil;
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
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  int rectX = NUM2INT(rbX);
  int rectY = NUM2INT(rbY);
  int rectWidth  = NUM2INT(rbWidth);
  int rectHeight = NUM2INT(rbHeight);
  if (!ModifyRectInTexture(texture, &rectX, &rectY, &rectWidth, &rectHeight))
    return Qnil;
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  Pixel* pixels = &(texture->pixels[rectX + rectY * texture->width]);
  int paddingJ = texture->width - rectWidth;
  for (int j = rectY; j < rectY + rectHeight; j++, pixels += paddingJ)
    for (int i = rectX; i < rectX + rectWidth; i++, pixels++)
      RENDER_PIXEL(pixels->color, color);
  return self;
}

static VALUE Texture_render_texture(int, VALUE*, VALUE);
static VALUE
Texture_render_text(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbText, rbX, rbY, rbFont, rbColor, rbAntiAlias;
  rb_scan_args(argc, argv, "51",
               &rbText, &rbX, &rbY, &rbFont, &rbColor, &rbAntiAlias);
  Check_Type(rbText, T_STRING);
  if (!(RSTRING_LEN(rbText)))
    return self;
  bool antiAlias = RTEST(rbAntiAlias);
  Check_Type(rbText, T_STRING);
  if (!RSTRING_LEN(rbText))
    rb_raise(rb_eArgError, "empty text");
  char* text = StringValueCStr(rbText);
  Font* font;
  Data_Get_Struct(rbFont, Font, font);
  volatile VALUE rbSize = rb_funcall(rbFont, rb_intern("get_size"), 1, rbText);
  volatile VALUE rbTextTexture =
    rb_class_new_instance(2, RARRAY_PTR(rbSize), rb_cTexture);
  Texture* textTexture;
  Data_Get_Struct(rbTextTexture, Texture, textTexture);
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);

  SDL_Surface* textSurfaceRaw;
  if (antiAlias)
    textSurfaceRaw =
      TTF_RenderUTF8_Shaded(font->sdlFont, text,
                            (SDL_Color){255, 255, 255, 255},
                            (SDL_Color){0, 0, 0, 0});
  else
    textSurfaceRaw =
      TTF_RenderUTF8_Solid(font->sdlFont, text,
                           (SDL_Color){255, 255, 255, 255});
  if (!textSurfaceRaw)
    rb_raise_sdl_ttf_error();
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
  if (!textSurface)
    rb_raise_sdl_error();
  SDL_LockSurface(textSurface);
  Pixel* src = (Pixel*)(textSurface->pixels);
  Pixel* dst = textTexture->pixels;
  int size = textTexture->width * textTexture->height;
  for (int i = 0; i < size; i++, src++, dst++) {
    if (src->value) {
      dst->color = color;
      if (color.alpha == 255)
        dst->color.alpha = src->color.red;
      else
        dst->color.alpha = DIV255(src->color.red * color.alpha);
    }
  }
  SDL_UnlockSurface(textSurface);
  SDL_FreeSurface(textSurface);
  textSurface = NULL;

  Texture_render_texture(3, (VALUE[]) {rbTextTexture, rbX, rbY}, self);
  Texture_dispose(rbTextTexture);
  return self;
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
  } else if (key == symbol_alpha) {
    options->alpha = NUM2DBL(val);
  } else if (key == symbol_blend_type) {
    Check_Type(val, T_SYMBOL);
    if (val == symbol_none)
      options->blendType = BLEND_TYPE_NONE;
    else if (val == symbol_alpha)
      options->blendType = BLEND_TYPE_ALPHA;
    else if (val == symbol_add)
      options->blendType = BLEND_TYPE_ADD;
    else if (val == symbol_sub)
      options->blendType = BLEND_TYPE_SUB;
    else if (val == symbol_mask)
      options->blendType = BLEND_TYPE_MASK;
  } else if (key == symbol_tone_red) {
    options->toneRed = NUM2INT(val);
  } else if (key == symbol_tone_green) {
    options->toneGreen = NUM2INT(val);
  } else if (key == symbol_tone_blue) {
    options->toneBlue = NUM2INT(val);
  } else if (key == symbol_saturation) {
    options->saturation = NUM2INT(val);
  }
  return ST_CONTINUE;
}

static VALUE
Texture_render_texture(int argc, VALUE* argv, VALUE self)
{
  rb_check_frozen(self);
  Texture* dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);
  CheckDisposed(dstTexture);
  CheckPalette(dstTexture);

  volatile VALUE rbTexture, rbX, rbY, rbOptions;
  if (3 <= argc && argc <= 4) {
    rbTexture = argv[0];
    rbX       = argv[1];
    rbY       = argv[2];
    rbOptions = (argc == 4) ? argv[3] : Qnil;
  } else {
    rb_scan_args(argc, argv, "31", &rbTexture, &rbX, &rbY, &rbOptions);
  }

  Texture* srcTexture;
  Data_Get_Struct(rbTexture, Texture, srcTexture);
  CheckDisposed(srcTexture);

  int srcTextureWidth  = srcTexture->width;
  int srcTextureHeight = srcTexture->height;
  int dstTextureWidth  = dstTexture->width;
  int dstTextureHeight = dstTexture->height;

  RenderingTextureOptions options = {
    .srcX       = 0,
    .srcY       = 0,
    .srcWidth   = srcTextureWidth,
    .srcHeight  = srcTextureHeight,
    .scaleX     = 1,
    .scaleY     = 1,
    .angle      = 0,
    .centerX    = 0,
    .centerY    = 0,
    .alpha      = 255,
    .blendType  = BLEND_TYPE_ALPHA,
    .toneRed    = 0,
    .toneGreen  = 0,
    .toneBlue   = 0,
    .saturation = 255,
  };

  if (!SPECIAL_CONST_P(rbOptions) && BUILTIN_TYPE(rbOptions) == T_HASH) {
    if (NIL_P(RHASH(rbOptions)->ifnone)) {
      // Only for Ruby 1.8
      st_table* table = RHASH(rbOptions)->tbl;
      if (0 < table->num_entries) {
        volatile VALUE val;
        st_foreach(table, AssignRenderingTextureOptions, (st_data_t)&options);
        if (!st_lookup(table, (st_data_t)symbol_src_width, (st_data_t*)&val))
          options.srcWidth = srcTextureWidth - options.srcX;
        if (!st_lookup(table, (st_data_t)symbol_src_height, (st_data_t*)&val))
          options.srcHeight = srcTextureHeight - options.srcY;
      }
    } else {
      volatile VALUE val;
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_x)))
        options.srcX = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_y)))
        options.srcY = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_width)))
        options.srcWidth = NUM2INT(val);
      else
        options.srcWidth = srcTextureWidth - options.srcX;
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_src_height)))
        options.srcHeight = NUM2INT(val);
      else
        options.srcHeight = srcTextureHeight - options.srcY;
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_x)))
        options.scaleX = NUM2DBL(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_scale_y)))
        options.scaleY = NUM2DBL(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_angle)))
        options.angle = NUM2DBL(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_center_x)))
        options.centerX = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_center_y)))
        options.centerY = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_alpha)))
        options.alpha = NUM2DBL(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_blend_type))) {
        Check_Type(val, T_SYMBOL);
        if (val == symbol_none)
          options.blendType = BLEND_TYPE_NONE;
        else if (val == symbol_alpha)
          options.blendType = BLEND_TYPE_ALPHA;
        else if (val == symbol_add)
          options.blendType = BLEND_TYPE_ADD;
        else if (val == symbol_sub)
          options.blendType = BLEND_TYPE_SUB;
        else if (val == symbol_mask)
          options.blendType = BLEND_TYPE_MASK;
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone_red)))
        options.toneRed = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone_green)))
        options.toneGreen = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone_blue)))
        options.toneBlue = NUM2INT(val);
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_saturation)))
        options.saturation = NUM2INT(val);
    }
  } else if (!NIL_P(rbOptions)) {
    Check_Type(rbOptions, T_HASH);
  }

  if (!ModifyRectInTexture(srcTexture,
                           &(options.srcX), &(options.srcY),
                           &(options.srcWidth), &(options.srcHeight)))
    return self;

  if (options.toneRed   < -255 || 255 < options.toneRed   ||
      options.toneGreen < -255 || 255 < options.toneGreen ||
      options.toneBlue  < -255 || 255 < options.toneBlue  ||
      options.saturation < 0 || 255 < options.saturation)
    rb_raise(rb_eArgError, "invalid tone value: (r:%d, g:%d, b:%d, s:%d)",
             options.toneRed, options.toneGreen, options.toneBlue,
             options.saturation);

  uint8_t alpha       = options.alpha;
  double angle        = options.angle;
  BlendType blendType = options.blendType;
  int centerX         = options.centerX;
  int centerY         = options.centerY;
  uint8_t saturation  = options.saturation;
  double scaleX       = options.scaleX;
  double scaleY       = options.scaleY;
  int srcHeight       = options.srcHeight;
  int srcWidth        = options.srcWidth;
  int srcX            = options.srcX;
  int srcY            = options.srcY;
  int toneRed         = options.toneRed;
  int toneGreen       = options.toneGreen;
  int toneBlue        = options.toneBlue;

  if (srcTexture != dstTexture &&
      (scaleX == 1 && scaleY == 1 && angle == 0 &&
       toneRed == 0 && toneGreen == 0 && toneBlue == 0 && saturation == 255 && 
       (blendType == BLEND_TYPE_ALPHA || blendType == BLEND_TYPE_NONE))) {
    int dstX = NUM2INT(rbX);
    int dstY = NUM2INT(rbY);
    if (dstX < 0) {
      srcX -= dstX;
      srcWidth += dstX;
      if (srcTextureWidth <= srcX || srcWidth <= 0)
        return Qnil;
      dstX = 0;
    } else if (dstTextureWidth <= dstX) {
      return Qnil;
    }
    if (dstY < 0) {
      srcY -= dstY;
      srcHeight += dstY;
      if (srcTextureHeight <= srcY || srcHeight <= 0)
        return Qnil;
      dstY = 0;
    } else if (dstTextureHeight <= dstY) {
      return Qnil;
    }
    int width  = MIN(srcWidth,  dstTextureWidth - dstX);
    int height = MIN(srcHeight, dstTextureHeight - dstY);
    Pixel* src = &(srcTexture->pixels[srcX + srcY * srcTextureWidth]);
    Pixel* dst = &(dstTexture->pixels[dstX + dstY * dstTextureWidth]);
    int srcPadding = srcTextureWidth - width;
    int dstPadding = dstTextureWidth - width;
    switch (blendType) {
    case BLEND_TYPE_ALPHA:
      if (alpha == 255) {
        for (int j = 0; j < height; j++, src += srcPadding, dst += dstPadding) {
          LOOP({
              uint8_t beta = src->color.alpha;
              uint8_t dstAlpha = dst->color.alpha;
              if ((beta == 255) | (dstAlpha == 0)) {
                *dst = *src;
              } else if (beta) {
                if (dstAlpha < beta)
                  dst->color.alpha = beta;
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
              uint8_t dstAlpha = dst->color.alpha;
              uint8_t beta = DIV255(src->color.alpha * alpha);
              if (dstAlpha == 0) {
                dst->color.alpha = beta;
                dst->color.red   = src->color.red;
                dst->color.green = src->color.green;
                dst->color.blue  = src->color.blue;
              } else if (beta) {
                if (dstAlpha < beta)
                  dst->color.alpha = beta;
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
      for (int j = 0; j < height; j++, src += srcPadding, dst += dstPadding)
        LOOP({
            *dst = *src;
            src++;
            dst++;
          }, width);
      break;
    default:
      assert(false);
      break;
    }
    return self;
  }

  AffineMatrix mat = {
    .a = 1, .c = 0, .tx = 0,
    .b = 0, .d = 1, .ty = 0,
  };
  if (scaleX != 1 || scaleY != 1 || angle != 0) {
    mat.tx -= centerX;
    mat.ty -= centerY;
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
      double a  = mat.a;
      double b  = mat.b;
      double c  = mat.c;
      double d  = mat.d;
      double tx = mat.tx;
      double ty = mat.ty;
      double cosAngle = cos(angle);
      double sinAngle = sin(angle);
      mat.a  = cosAngle * a  - sinAngle * c;
      mat.b  = cosAngle * b  - sinAngle * d;
      mat.c  = sinAngle * a  + cosAngle * c;
      mat.d  = sinAngle * b  + cosAngle * d;
      mat.tx = cosAngle * tx - sinAngle * ty;
      mat.ty = sinAngle * tx + cosAngle * ty;
    }
    mat.tx += centerX;
    mat.ty += centerY;
  }
  mat.tx += NUM2INT(rbX);
  mat.ty += NUM2INT(rbY);
  double det = mat.a * mat.d - mat.b * mat.c;
  if (det == 0)
    return self;

  double dstX00 = mat.tx;
  double dstY00 = mat.ty;
  double dstX01 = mat.b * srcHeight + mat.tx;
  double dstY01 = mat.d * srcHeight + mat.ty;
  double dstX10 = mat.a * srcWidth  + mat.tx;
  double dstY10 = mat.c * srcWidth  + mat.ty;
  double dstX11 = mat.a * srcWidth  + mat.b * srcHeight + mat.tx;
  double dstY11 = mat.c * srcWidth  + mat.d * srcHeight + mat.ty;
  double dstX0 = MIN(MIN(MIN(dstX00, dstX01), dstX10), dstX11);
  double dstY0 = MIN(MIN(MIN(dstY00, dstY01), dstY10), dstY11);
  double dstX1 = MAX(MAX(MAX(dstX00, dstX01), dstX10), dstX11);
  double dstY1 = MAX(MAX(MAX(dstY00, dstY01), dstY10), dstY11);
  if (dstTextureWidth <= dstX0 || dstTextureHeight <= dstY0 ||
      dstX1 < 0 || dstY1 < 0)
    return self;

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
  int dstX0Int = (int)dstX0;
  int dstY0Int = (int)dstY0;
  int dstWidth  = MIN(dstTextureWidth,  (int)dstX1) - dstX0Int;
  int dstHeight = MIN(dstTextureHeight, (int)dstY1) - dstY0Int;

  int_fast32_t srcOX16  = (int_fast32_t)(srcOX  * (1 << 16));
  int_fast32_t srcOY16  = (int_fast32_t)(srcOY  * (1 << 16));
  int_fast32_t srcDXX16 = (int_fast32_t)(srcDXX * (1 << 16));
  int_fast32_t srcDXY16 = (int_fast32_t)(srcDXY * (1 << 16));
  int_fast32_t srcDYX16 = (int_fast32_t)(srcDYX * (1 << 16));
  int_fast32_t srcDYY16 = (int_fast32_t)(srcDYY * (1 << 16));

  volatile VALUE rbClonedTexture = Qnil;
  if (self == rbTexture) {
    rbClonedTexture = rb_funcall(rbTexture, rb_intern("clone"), 0);
    Data_Get_Struct(rbClonedTexture, Texture, srcTexture);
  }

  int srcX2 = srcX + srcWidth;
  int srcY2 = srcY + srcHeight;
  for (int j = 0; j < dstHeight; j++) {
    int_fast32_t srcI16 = srcOX16 + j * srcDYX16;
    int_fast32_t srcJ16 = srcOY16 + j * srcDYY16;
    Pixel* dst =
      &(dstTexture->pixels[dstX0Int + (dstY0Int + j) * dstTextureWidth]);
    for (int i = 0; i < dstWidth;
         i++, dst++, srcI16 += srcDXX16, srcJ16 += srcDXY16) {
      int_fast32_t srcI = srcI16 >> 16;
      int_fast32_t srcJ = srcJ16 >> 16;
      if (srcX <= srcI && srcI < srcX2 && srcY <= srcJ && srcJ < srcY2) {
        Color srcColor = srcTexture->pixels[srcI + srcJ * srcTextureWidth].color;
        if (blendType == BLEND_TYPE_MASK) {
          dst->color.alpha = srcColor.red;
        } else {
          uint8_t srcRed   = srcColor.red;
          uint8_t srcGreen = srcColor.green;
          uint8_t srcBlue  = srcColor.blue;
          uint8_t srcAlpha = srcColor.alpha;
          if (saturation < 255) {
            // http://www.poynton.com/ColorFAQ.html
            uint8_t y = (6969  * srcRed +
                         23434 * srcGreen +
                         2365  * srcBlue) / 32768;
            srcRed   = ALPHA(srcRed,   y, saturation);
            srcGreen = ALPHA(srcGreen, y, saturation);
            srcBlue  = ALPHA(srcBlue,  y, saturation);
          }
          if (toneRed) {
            if (0 < toneRed)
              srcRed = ALPHA(255, srcRed, toneRed);
            else
              srcRed = ALPHA(0,   srcRed, -toneRed);
          }
          if (toneGreen) {
            if (0 < toneGreen)
              srcGreen = ALPHA(255, srcGreen, toneGreen);
            else
              srcGreen = ALPHA(0,   srcGreen, -toneGreen);
          }
          if (toneBlue) {
            if (0 < toneBlue)
              srcBlue = ALPHA(255, srcBlue, toneBlue);
            else
              srcBlue = ALPHA(0,   srcBlue, -toneBlue);
          }
          if (blendType == BLEND_TYPE_NONE) {
            dst->color.red   = srcRed;
            dst->color.green = srcGreen;
            dst->color.blue  = srcBlue;
            dst->color.alpha = srcAlpha;
          } else if (dst->color.alpha == 0) {
            uint8_t beta = DIV255(srcAlpha * alpha);
            switch (blendType) {
            case BLEND_TYPE_ALPHA:
              dst->color.red   = srcRed;
              dst->color.green = srcGreen;
              dst->color.blue  = srcBlue;
              dst->color.alpha = beta;
              break;
            case BLEND_TYPE_ADD:
              ;
              int addR = srcRed   + dst->color.red;
              int addG = srcGreen + dst->color.green;
              int addB = srcBlue  + dst->color.blue;
              dst->color.red   = MIN(255, addR);
              dst->color.green = MIN(255, addG);
              dst->color.blue  = MIN(255, addB);
              dst->color.alpha = beta;
              break;
            case BLEND_TYPE_SUB:
              ;
              int subR = -srcRed   + dst->color.red;
              int subG = -srcGreen + dst->color.green;
              int subB = -srcBlue  + dst->color.blue;
              dst->color.red   = MAX(0, subR);
              dst->color.green = MAX(0, subG);
              dst->color.blue  = MAX(0, subB);
              dst->color.alpha = beta;
              break;
            case BLEND_TYPE_MASK:
              assert(false);
              break;
            case BLEND_TYPE_NONE:
              assert(false);
              break;
            }
          } else {
            uint8_t beta = DIV255(srcAlpha * alpha);
            if (dst->color.alpha < beta)
              dst->color.alpha = beta;
            switch (blendType) {
            case BLEND_TYPE_ALPHA:
              dst->color.red   = ALPHA(srcRed,   dst->color.red,   beta);
              dst->color.green = ALPHA(srcGreen, dst->color.green, beta);
              dst->color.blue  = ALPHA(srcBlue,  dst->color.blue,  beta);
              break;
            case BLEND_TYPE_ADD:
              ;
              int addR = DIV255(srcRed   * beta) + dst->color.red;
              int addG = DIV255(srcGreen * beta) + dst->color.green;
              int addB = DIV255(srcBlue  * beta) + dst->color.blue;
              dst->color.red   = MIN(255, addR);
              dst->color.green = MIN(255, addG);
              dst->color.blue  = MIN(255, addB);
              break;
            case BLEND_TYPE_SUB:
              ;
              int subR = -DIV255(srcRed   * beta) + dst->color.red;
              int subG = -DIV255(srcGreen * beta) + dst->color.green;
              int subB = -DIV255(srcBlue  * beta) + dst->color.blue;
              dst->color.red   = MAX(0, subR);
              dst->color.green = MAX(0, subG);
              dst->color.blue  = MAX(0, subB);
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

  if (!NIL_P(rbClonedTexture))
    Texture_dispose(rbClonedTexture);

  return self;
}

static VALUE
Texture_save(VALUE self, VALUE rbPath)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  char* path = StringValueCStr(rbPath);
  FILE* fp = fopen(path, "wb");
  if (!fp)
    rb_raise(rb_path2class("Errno::ENOENT"), "%s", path);
  png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                               NULL, NULL, NULL);
  png_infop infoPtr = png_create_info_struct(pngPtr);
  png_init_io(pngPtr, fp);
  png_set_IHDR(pngPtr, infoPtr, texture->width, texture->height,
               8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(pngPtr, infoPtr);
  for (int j = 0; j < texture->height; j++) {
    png_byte row[texture->width * 4];
    for (int i = 0; i < texture->width; i++) {
      Color* c = &(texture->pixels[texture->width * j + i].color);
      row[i * 4]     = c->red;
      row[i * 4 + 1] = c->green;
      row[i * 4 + 2] = c->blue;
      row[i * 4 + 3] = c->alpha;
    }
    png_write_row(pngPtr, row);
  }
  png_write_end(pngPtr, infoPtr);
  png_destroy_write_struct(&pngPtr, &infoPtr);
  fclose(fp);
  return Qnil;
}

static VALUE
Texture_set_pixel(VALUE self, VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  int x = NUM2INT(rbX);
  int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y)
    return Qnil;
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  texture->pixels[x + y * texture->width].color = color;
  return rbColor;
}

static VALUE
Texture_size(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  volatile VALUE rbSize =
    rb_assoc_new(INT2NUM(texture->width), INT2NUM(texture->height));
  OBJ_FREEZE(rbSize);
  return rbSize;
}

static VALUE
Texture_transform_in_perspective(int argc, VALUE* argv, VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  volatile VALUE rbX, rbY, rbHeight, rbOptions;
  rb_scan_args(argc, argv, "31", &rbX, &rbY, &rbHeight, &rbOptions);
  if (NIL_P(rbOptions))
    rbOptions = rb_hash_new();
  int screenWidth = texture->width;
  PerspectiveOptions options;
  AssignPerspectiveOptions(&options, rbOptions);
  double cosYaw   = cos(options.cameraYaw);
  double sinYaw   = sin(options.cameraYaw);
  double cosPitch = cos(options.cameraPitch);
  double sinPitch = sin(options.cameraPitch);
  double cosRoll  = cos(options.cameraRoll);
  double sinRoll  = sin(options.cameraRoll);
  double x = NUM2INT(rbX) - options.cameraX;
  double y = NUM2DBL(rbHeight);
  double z = NUM2INT(rbY) - options.cameraY;
  double x2, y2, z2;
  x2 = cosYaw * x  + sinYaw * z;
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
  if (z == 0)
    return rbResult;
  double distance = screenWidth / (2 * tan(options.viewAngle / 2));
  double scale = -distance / z;
  double screenX = x * scale;
  double screenY = (options.cameraHeight - y) * scale;
  double screenX2 = cosRoll  * screenX + sinRoll * screenY;
  double screenY2 = -sinRoll * screenX + cosRoll * screenY;
  screenX = screenX2 + options.intersectionX;
  screenY = screenY2 + options.intersectionY;
  long screenXLong = (long)screenX;
  long screenYLong = (long)screenY;
  if (FIXABLE(screenXLong) && INT_MIN <= screenXLong && screenXLong <= INT_MAX)
    RARRAY_PTR(rbResult)[0] = LONG2FIX(screenXLong);
  else
    RARRAY_PTR(rbResult)[0] = Qnil;
  if (FIXABLE(screenYLong) && INT_MIN <= screenYLong && screenYLong <= INT_MAX)
    RARRAY_PTR(rbResult)[1] = LONG2FIX(screenYLong);
  else
    RARRAY_PTR(rbResult)[1] = Qnil;
  RARRAY_PTR(rbResult)[2] = rb_float_new(scale);
  return rbResult;
}

static VALUE
Texture_undump(VALUE self, VALUE rbData, VALUE rbFormat)
{
  rb_check_frozen(self);
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  CheckPalette(texture);
  char* format = StringValuePtr(rbFormat);
  int formatLength = RSTRING_LEN(rbFormat);
  int pixelLength = texture->width * texture->height;
  Check_Type(rbData, T_STRING);
  if (pixelLength * formatLength != RSTRING_LEN(rbData))
    rb_raise(rb_eArgError, "invalid data size: %d expected but was %ld",
             pixelLength * formatLength, RSTRING_LEN(rbData));
  uint8_t* data = (uint8_t*)RSTRING_PTR(rbData);
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
  return Qnil;
}

static VALUE
Texture_width(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  CheckDisposed(texture);
  return INT2NUM(texture->width);
}

VALUE
strb_InitializeTexture(VALUE rb_mStarRuby)
{
  rb_cTexture = rb_define_class_under(rb_mStarRuby, "Texture", rb_cObject);
  rb_define_singleton_method(rb_cTexture, "load", Texture_s_load, -1);
  rb_define_alloc_func(rb_cTexture, Texture_alloc);
  rb_define_private_method(rb_cTexture, "initialize", Texture_initialize, 2);
  rb_define_private_method(rb_cTexture, "initialize_copy",
                           Texture_initialize_copy, 1);
  rb_define_method(rb_cTexture, "[]",
                   Texture_get_pixel, 2);
  rb_define_method(rb_cTexture, "[]=",
                   Texture_set_pixel, 3);
  rb_define_method(rb_cTexture, "change_hue",
                   Texture_change_hue, 1);
  rb_define_method(rb_cTexture, "change_hue!",
                   Texture_change_hue_bang, 1);
  rb_define_method(rb_cTexture, "change_palette",
                   Texture_change_palette, 1);
  rb_define_method(rb_cTexture, "change_palette!",
                   Texture_change_palette_bang, 1);
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
  rb_define_method(rb_cTexture, "height",
                   Texture_height, 0);
  rb_define_method(rb_cTexture, "palette",
                   Texture_palette, 0);
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

  return rb_cTexture;
}
