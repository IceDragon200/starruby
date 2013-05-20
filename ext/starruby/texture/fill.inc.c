static VALUE
Texture_fill(VALUE self, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  Color color;
  strb_RubyToColor(rbColor, &color);
  const int32_t length = texture->width * texture->height;
  Pixel* pixels = texture->pixels;
  for (int32_t i = 0; i < length; i++, pixels++) {
    pixels->color = color;
  }
  return Qnil;
}

static VALUE
Texture_fill_rect(int argc, VALUE* argv, VALUE self)
{
  Color color;
  int32_t padding;
  Pixel* pixels;
  Rect rect;
  Texture* texture;

  rb_check_frozen(self);

  if (argc == 2)
  {
    VALUE rbRect, rbColor;
    rb_scan_args(argc, argv, "20", &rbRect, &rbColor);
    strb_RubyToRect(rbRect, &(rect));
    strb_RubyToColor(rbColor, &(color));
  } else if (argc == 5)  {
    VALUE rbX, rbY, rbWidth, rbHeight, rbColor;
    rb_scan_args(argc, argv, "50", &rbX, &rbY, &rbWidth, &rbHeight, &rbColor);
    rect.x      = NUM2INT(rbX);
    rect.y      = NUM2INT(rbY);
    rect.width  = NUM2INT(rbWidth);
    rect.height = NUM2INT(rbHeight);
    strb_RubyToColor(rbColor, &(color));
  } else {
    rb_raise(rb_eArgError, "expected 2 or 5 arguments but recieved %d", argc);
  }

  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  if (!ModifyRectInTexture_Rect(texture, &(rect))) {
    return Qnil;
  }

  pixels = &(texture->pixels[rect.x + rect.y * texture->width]);
  padding = texture->width - rect.width;

  for (int32_t j = 0; j < rect.height; j++, pixels += padding) {
    for (int32_t i = 0; i < rect.width; i++, pixels++) {
      pixels->color = color;
    }
  }
  return Qnil;
}
