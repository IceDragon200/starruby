/*
  StarRuby Texture
    render-texture
  vr 1.0.0
  */
static void
RenderTexture(const Texture* srcTexture, const Texture* dstTexture,
              int srcX, int srcY, int srcWidth, int srcHeight, int dstX, int dstY,
              const uint8_t alpha, const Tone *tone, const Color *color,
              const BlendType blendType)
{
  BlendFunc blendFunc = NULL;
  switch(blendType)
  {
    case BLEND_TYPE_ALPHA:
      blendFunc = Pixel_blend_alpha;
      break;
    case BLEND_TYPE_NONE:
      blendFunc = Pixel_blend_none;
      break;
    case BLEND_TYPE_MASK:
      blendFunc = Pixel_blend_mask;
      break;
    case BLEND_TYPE_ADD:
      blendFunc = Pixel_blend_add;
      break;
    case BLEND_TYPE_SUB:
      blendFunc = Pixel_blend_sub;
      break;
    case BLEND_TYPE_MUL:
      blendFunc = Pixel_blend_mul;
      break;
    default:
      rb_raise(rb_eArgError, "Invalid Blend Type %d", (int)blendType);
  }

  RenderTexture_blend(srcTexture, dstTexture,
                      srcX, srcY, srcWidth, srcHeight,
                      dstX, dstY, alpha, tone, color, blendFunc);
  /*
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
  */
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
            case BLEND_TYPE_MUL:
              assert(false);
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
            case BLEND_TYPE_MUL:
              assert(false);
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
    .color        = (Color) {
      .red = 0, .green = 0, .blue = 0, .alpha = 0
    }
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
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_tone))) {
        Tone *tone;
        Data_Get_Struct(val, Tone, tone);
        options.tone = *tone;
      }
      if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_color))) {
        Color *color;
        Data_Get_Struct(val, Color, color);
        options.color = *color;
      }
    }
  } else if (!NIL_P(rbOptions)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Hash)",
             rb_obj_classname(rbOptions));
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
      (options.scaleX == 1 && options.scaleY == 1 && options.angle == 0)) {
    RenderTexture(srcTexture, dstTexture,
                  srcX, srcY, srcWidth, srcHeight, NUM2INT(rbX), NUM2INT(rbY),
                  options.alpha,
                  &(options.tone),
                  &(options.color),
                  options.blendType);
  } else {
    RenderTextureWithOptions(srcTexture, dstTexture,
                             srcX, srcY, srcWidth, srcHeight, NUM2INT(rbX), NUM2INT(rbY),
                             &options);
  }
  return self;
}
