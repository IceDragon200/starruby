/* Old Render Texture loop */
  //private(iy, j, src_px, dst_px) // for (iy, j, src_px, dst_px)
  for(int32_t j = 0; j < height; ++iy, ++j, src_px += srcPadding, dst_px += dstPadding) {
    int32_t ix = dstX;
    for(int32_t k = 0; k < width; ++ix, ++k, ++src_px, ++dst_px) {
      if (is_xy_in_rect(cclip_rect, ix, iy)) {
        if(use_tone && use_color) {
          Pixel pixel;
          Pixel* px_color = (Pixel*)color;
          pixel = *src_px;
          Pixel_tone((&pixel), tone, alpha);
          switch(blendType)
          {
            case BLEND_TYPE_NONE:
              Pixel_blend_none(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_ALPHA:
              Pixel_blend_alpha(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_MASK:
              Pixel_blend_mask(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_ADD:
              Pixel_blend_add(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_SUBTRACT:
              Pixel_blend_sub(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_MULTIPLY:
              Pixel_blend_mul(dst_px, &pixel, alpha);
              break;
          }
          Pixel_blend_color(dst_px, px_color, alpha);
        } else if(use_color) {
          Pixel* px_color = (Pixel*)color;
          switch(blendType)
          {
            case BLEND_TYPE_NONE:
              Pixel_blend_none(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_ALPHA:
              Pixel_blend_alpha(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_MASK:
              Pixel_blend_mask(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_ADD:
              Pixel_blend_add(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_SUBTRACT:
              Pixel_blend_sub(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_MULTIPLY:
              Pixel_blend_mul(dst_px, src_px, alpha);
              break;
          }
          Pixel_blend_color(dst_px, px_color, alpha);
        } else if(use_tone) {
          Pixel pixel;
          pixel = *src_px;
          Pixel_tone((&pixel), tone, alpha);
          switch(blendType)
          {
            case BLEND_TYPE_NONE:
              Pixel_blend_none(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_ALPHA:
              Pixel_blend_alpha(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_MASK:
              Pixel_blend_mask(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_ADD:
              Pixel_blend_add(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_SUBTRACT:
              Pixel_blend_sub(dst_px, &pixel, alpha);
              break;
            case BLEND_TYPE_MULTIPLY:
              Pixel_blend_mul(dst_px, &pixel, alpha);
              break;
          }
        } else {
          switch(blendType)
          {
            case BLEND_TYPE_NONE:
              Pixel_blend_none(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_ALPHA:
              Pixel_blend_alpha(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_MASK:
              Pixel_blend_mask(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_ADD:
              Pixel_blend_add(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_SUBTRACT:
              Pixel_blend_sub(dst_px, src_px, alpha);
              break;
            case BLEND_TYPE_MULTIPLY:
              Pixel_blend_mul(dst_px, src_px, alpha);
              break;
          }
        }
      }
    }
  }

static void
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

  for (int j = 0; j < dstHeight; ++j) {
    int_fast32_t srcI16 = srcOX16 + j * srcDYX16;
    int_fast32_t srcJ16 = srcOY16 + j * srcDYY16;
    Pixel* dst =
      &(dst_texture->pixels[dstX0Int + (dstY0Int + j) * dst_texture->width]);
    for (int i = 0; i < dstWidth;
         ++i, ++dst, srcI16 += srcDXX16, srcJ16 += srcDXY16) {
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
            case BLEND_TYPE_SUBTRACT:
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
            case BLEND_TYPE_MULTIPLY:
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
            case BLEND_TYPE_SUBTRACT:
              {
                const int subR = -DIV255(srcRed   * beta) + dst->color.red;
                const int subG = -DIV255(srcGreen * beta) + dst->color.green;
                const int subB = -DIV255(srcBlue  * beta) + dst->color.blue;
                dst->color.red   = MAX(0, subR);
                dst->color.green = MAX(0, subG);
                dst->color.blue  = MAX(0, subB);
              }
              break;
            case BLEND_TYPE_MULTIPLY:
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
