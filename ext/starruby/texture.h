/*
  starruby/ext/starruby/texture.h
  vr 0.61
 */
#ifndef STARRUBY_TEXTURE_H
  #define STARRUBY_TEXTURE_H

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

  inline bool strb_Texture_dimensions_match(Texture *t1, Texture *t2)
  {
    return (t1->width == t2->width || t1->height == t2->height);
  }

#endif
