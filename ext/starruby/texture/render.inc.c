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
  } else if (key == symbol_tone) {
    Tone *tone;
    Data_Get_Struct(val, Tone, tone);
    options->tone = *tone;
  } else if (key == symbol_color) {
    Color *color;
    Data_Get_Struct(val, Color, color);
    options->color = *color;
  }
  return ST_CONTINUE;
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
