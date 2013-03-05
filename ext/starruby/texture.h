/*
  starruby/ext/starruby/texture.h
  vr 0.61
 */
#ifndef STARRUBY_TEXTURE_H
#define STARRUBY_TEXTURE_H

#define ALPHA(src, dst, a) DIV255((dst << 8) - dst + (src - dst) * a)
#define CLAMP255(v) (MAX(MIN((v), 255), 0))

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

#define RENDER_PIXEL(_dst, _src)                              \
  do {                                                        \
    if (_dst.alpha == 0) {                                    \
      _dst = _src;                                            \
    } else {                                                  \
      if (_dst.alpha < _src.alpha) {                          \
        _dst.alpha = _src.alpha;                              \
      }                                                       \
      _dst.red   = ALPHA(_src.red,   _dst.red,   _src.alpha); \
      _dst.green = ALPHA(_src.green, _dst.green, _src.alpha); \
      _dst.blue  = ALPHA(_src.blue,  _dst.blue,  _src.alpha); \
    }                                                         \
  } while (false)

static volatile VALUE rb_cTexture = Qundef;

static volatile VALUE symbol_add            = Qundef;
static volatile VALUE symbol_alpha          = Qundef;
static volatile VALUE symbol_angle          = Qundef;
static volatile VALUE symbol_background     = Qundef;
static volatile VALUE symbol_blend_type     = Qundef;
static volatile VALUE symbol_blur           = Qundef;
static volatile VALUE symbol_camera_height  = Qundef;
static volatile VALUE symbol_camera_pitch   = Qundef;
static volatile VALUE symbol_camera_roll    = Qundef;
static volatile VALUE symbol_camera_x       = Qundef;
static volatile VALUE symbol_camera_y       = Qundef;
static volatile VALUE symbol_camera_yaw     = Qundef;
static volatile VALUE symbol_center_x       = Qundef;
static volatile VALUE symbol_center_y       = Qundef;
static volatile VALUE symbol_height         = Qundef;
static volatile VALUE symbol_intersection_x = Qundef;
static volatile VALUE symbol_intersection_y = Qundef;
static volatile VALUE symbol_io_length      = Qundef;
static volatile VALUE symbol_loop           = Qundef;
static volatile VALUE symbol_mask           = Qundef;
static volatile VALUE symbol_matrix         = Qundef;
static volatile VALUE symbol_none           = Qundef;
static volatile VALUE symbol_palette        = Qundef;
static volatile VALUE symbol_saturation     = Qundef;
static volatile VALUE symbol_scale_x        = Qundef;
static volatile VALUE symbol_scale_y        = Qundef;
static volatile VALUE symbol_src_height     = Qundef;
static volatile VALUE symbol_src_width      = Qundef;
static volatile VALUE symbol_src_x          = Qundef;
static volatile VALUE symbol_src_y          = Qundef;
static volatile VALUE symbol_sub            = Qundef;
static volatile VALUE symbol_tone_blue      = Qundef;
static volatile VALUE symbol_tone_green     = Qundef;
static volatile VALUE symbol_tone_red       = Qundef;
static volatile VALUE symbol_view_angle     = Qundef;
static volatile VALUE symbol_width          = Qundef;
static volatile VALUE symbol_x              = Qundef;
static volatile VALUE symbol_y              = Qundef;

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
  AffineMatrix matrix;
  int srcHeight;
  int srcWidth;
  int srcX;
  int srcY;
  Tone tone;
  BlendType blendType;
  uint8_t alpha;
} RenderingTextureOptions;

#endif
