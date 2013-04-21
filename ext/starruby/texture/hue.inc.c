
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
  strb_TextureCheckDisposed(texture);
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

static VALUE
Texture_change_hue(VALUE self, VALUE rbAngle)
{
  const Texture* texture;
  //Texture* newTexture;

  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  volatile VALUE rbTexture = rb_obj_dup(self);
  //Data_Get_Struct(rbTexture, Texture, newTexture);
  Texture_change_hue_bang(rbTexture, rbAngle);
  return rbTexture;
}
