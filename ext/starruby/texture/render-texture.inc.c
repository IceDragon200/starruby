/*
  StarRuby Texture
    render-texture
  vr 1.0.0
  */

#define is_valid_tone(tone_p) (tone_p && (tone_p->saturation < 255 || tone_p->red != 0 || tone_p->green != 0 || tone_p->blue != 0))
#define is_valid_color(color_p) (color_p && (color_p->alpha > 0))
#define IMGLOOP(content) \
  for(Integer j = 0; j < height; j++, src_px += srcPadding, dst_px += dstPadding) { \
    for(Integer k = 0; k < width; k++, src_px++, dst_px++) { \
      content; \
    } \
  }

Void strb_TextureRender(const Texture* src_texture, const Texture* dst_texture,
                        Integer srcX, Integer srcY,
                        Integer srcWidth, Integer srcHeight,
                        Integer dstX, Integer dstY,
                        const UByte alpha, const Tone *tone, const Color *color,
                        const BlendType blendType)
{
  BlendFunc blendFunc = Null;
  Integer clip_x, clip_y, clip_width, clip_height;

  switch(blendType)
  {
    case BLEND_TYPE_NONE:
      blendFunc = Pixel_blend_none;
      break;
    case BLEND_TYPE_ALPHA:
      blendFunc = Pixel_blend_alpha;
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
      return;
  }

  clip_x      = 0;
  clip_y      = 0;
  clip_width  = dst_texture->width;
  clip_height = dst_texture->height;

  if (dst_texture->clip_rect != Qnil) {
    Rect* rect;
    Data_Get_Struct(dst_texture->clip_rect, Rect, rect);
    clip_x = rect->x;
    clip_y = rect->y;
    clip_width = rect->width;
    clip_height = rect->height;
  }

  if (dstX < clip_x) {
    srcX -= dstX - clip_x;
    srcWidth += dstX - clip_x;
    if (src_texture->width <= srcX || srcWidth <= 0) {
      return;
    }
    dstX = clip_x;
  } else if (dst_texture->width <= dstX) {
    return;
  }
  if (dstY < clip_y) {
    srcY -= dstY - clip_y;
    srcHeight += dstY - clip_y;
    if (src_texture->height <= srcY || srcHeight <= 0) {
      return;
    }
    dstY = clip_y;
  } else if (dst_texture->height <= dstY) {
    return;
  }

  if (srcWidth > clip_width) {
    srcWidth = clip_width;
  }
  if (srcHeight > clip_height) {
    srcHeight = clip_height;
  }

  const int width  = MIN(srcWidth,  dst_texture->width - dstX);
  const int height = MIN(srcHeight, dst_texture->height - dstY);
  Pixel* src_px = &(src_texture->pixels[srcX + srcY * src_texture->width]);
  Pixel* dst_px = &(dst_texture->pixels[dstX + dstY * dst_texture->width]);

  const Integer srcPadding = src_texture->width - width;
  const Integer dstPadding = dst_texture->width - width;

  const Boolean use_tone  = is_valid_tone(tone);
  const Boolean use_color = is_valid_color(color);

  if(use_tone && use_color) {
    Pixel pixel;
    Pixel* px_color = (Pixel*)color;
    IMGLOOP({
      pixel = *src_px;
      Pixel_tone((&pixel), tone, alpha);
      (*blendFunc)(dst_px, &pixel, alpha);
      Pixel_blend_color(dst_px, px_color, Null);
    })
  } else if(use_color) {
    Pixel* px_color = (Pixel*)color;
    IMGLOOP({
      (*blendFunc)(dst_px, src_px, alpha);
      Pixel_blend_color(dst_px, px_color, Null);
    })
  } else if(use_tone) {
    Pixel pixel;
    IMGLOOP({
      pixel = *src_px;
      Pixel_tone((&pixel), tone, alpha);
      (*blendFunc)(dst_px, &pixel, alpha);
    })
  } else {
    IMGLOOP({ (*blendFunc)(dst_px, src_px, alpha); })
  }
}

static Void
strb_TextureRenderWithOptions(const Texture* src_texture, const Texture* dst_texture,
                              int srcX, int srcY, int srcWidth, int srcHeight,
                              int dstX, int dstY,
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
  const int saturation      = options->tone.saturation;
  const int toneRed         = options->tone.red;
  const int toneGreen       = options->tone.green;
  const int toneBlue        = options->tone.blue;

  for (int j = 0; j < dstHeight; j++) {
    int_fast32_t srcI16 = srcOX16 + j * srcDYX16;
    int_fast32_t srcJ16 = srcOY16 + j * srcDYY16;
    Pixel* dst =
      &(dst_texture->pixels[dstX0Int + (dstY0Int + j) * dst_texture->width]);
    for (int i = 0; i < dstWidth;
         i++, dst++, srcI16 += srcDXX16, srcJ16 += srcDXY16) {
      const int_fast32_t srcI = srcI16 >> 16;
      const int_fast32_t srcJ = srcJ16 >> 16;
      if (srcX <= srcI && srcI < srcX2 && srcY <= srcJ && srcJ < srcY2) {
        const Color srcColor =
          src_texture->pixels[srcI + srcJ * src_texture->width].color;
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
    strb_TextureFree(clonedTexture);
    clonedTexture = NULL;
  }
}

static VALUE
Texture_render_texture(int argc, VALUE* argv, VALUE self)
{
  AffineMatrix* matrix;
  Integer srcX, srcY, srcWidth, srcHeight;
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

  strb_CheckObjIsKindOf(rbTexture, rb_cTexture);
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

  if (!NIL_P(rbOptions) && (BUILTIN_TYPE(rbOptions) != T_HASH)) {
    if (rb_obj_respond_to(rbOptions, ID_to_h, Qfalse)) {
      rbOptions = rb_funcall(rbOptions, ID_to_h, 0);
    } else {
      rb_raise(rb_eTypeError, "cannot convert argument type %s into Hash",
               rb_obj_classname(rbOptions));
    }
  }

  if (!SPECIAL_CONST_P(rbOptions) && (BUILTIN_TYPE(rbOptions) == T_HASH) &&
      (NIL_P(RHASH_IFNONE(rbOptions)))) {
    st_table* table = RHASH_TBL(rbOptions);
    if (0 < table->num_entries) {
      //volatile VALUE val;
      st_foreach(table, AssignRenderingTextureOptions, (st_data_t)&options);
      /*if (!st_lookup(table, (st_data_t)symbol_src_width, (st_data_t*)&val)) {
        options.srcWidth = src_texture->width - options.srcX;
      }
      if (!st_lookup(table, (st_data_t)symbol_src_height, (st_data_t*)&val)) {
        options.srcHeight = src_texture->height - options.srcY;
      }*/
    }
  } else if (!NIL_P(rbOptions)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Hash)",
             rb_obj_classname(rbOptions));
  }

  srcX      = options.srcX;
  srcY      = options.srcY;
  srcWidth  = options.srcWidth;
  srcHeight = options.srcHeight;
  matrix = &(options.matrix);

  if (!ModifyRectInTexture(src_texture,
                           &(srcX), &(srcY), &(srcWidth), &(srcHeight))) {
    return self;
  }

  if (src_texture != dst_texture &&
      (matrix->a == 1 && matrix->b == 0 && matrix->c == 0 && matrix->d == 1) &&
      (options.scaleX == 1.0 && options.scaleY == 1.0 && options.angle == 0)) {
    strb_TextureRender(src_texture, dst_texture,
                      srcX, srcY, srcWidth, srcHeight, NUM2INT(rbX), NUM2INT(rbY),
                      options.alpha,
                      &(options.tone),
                      &(options.color),
                      options.blendType);
  } else {
    strb_TextureRenderWithOptions(src_texture, dst_texture,
                                  srcX, srcY, srcWidth, srcHeight,
                                  NUM2INT(rbX), NUM2INT(rbY),
                                  &options);
  }
  return self;
}
