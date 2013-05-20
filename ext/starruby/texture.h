/*
  starruby/ext/starruby/texture.h
  vr 0.61
 */
#ifndef STARRUBY_TEXTURE_H
#define STARRUBY_TEXTURE_H

//typedef Void(*BlendFunc)(Pixel* dst, const Pixel* src, const uint8_t alpha);

struct ColorDiff {
  int32_t red, green, blue, alpha;
} ;

typedef enum {
  BLEND_TYPE_NONE,
  BLEND_TYPE_ALPHA,
  BLEND_TYPE_ADD,
  BLEND_TYPE_SUB,
  BLEND_TYPE_MUL,
  BLEND_TYPE_MASK
} BlendType;

typedef enum {
  BLUR_TYPE_NONE,
  BLUR_TYPE_COLOR,
  BLUR_TYPE_BACKGROUND
} BlurType;

typedef enum {
  ROTATE_NONE,
  ROTATE_CW,
  ROTATE_CCW,
  ROTATE_180,
  ROTATE_HORZ,
  ROTATE_VERT
} RotateType;

typedef enum {
  MASK_ALPHA,
  MASK_GRAY,
  MASK_RED,
  MASK_GREEN,
  MASK_BLUE
} MaskType;

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
  Color color;
  BlendType blendType;
  uint8_t alpha;
} RenderingTextureOptions;

inline bool strb_Texture_dimensions_match(Texture *t1, Texture *t2)
{
  return (t1->width == t2->width && t1->height == t2->height);
}

void strb_TextureRender(const Texture* srcTexture, const Texture* dstTexture,
                        int32_t srcX, int32_t srcY,
                        int32_t srcWidth, int32_t srcHeight,
                        int32_t dstX, int32_t dstY,
                        const uint8_t alpha, const Tone *tone, const Color *color,
                        const BlendType blendType);
#endif
