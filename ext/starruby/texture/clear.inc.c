static VALUE
Texture_clear(VALUE self)
{
  const Texture* texture;
  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  const int32_t length = texture->width * texture->height;
  //Pixel* pixels = texture->pixels;
  //for (int32_t i = 0; i < length; i++, pixels++) {
  //  pixels->value = 0x00000000;
  //}
  MEMZERO(texture->pixels, Pixel, length);
  return self;
}

static VALUE
Texture_clear_rect(int argc, VALUE* argv, VALUE self)
{
  int32_t padding;
  Pixel* pixels;
  Rect rect;
  Texture* texture;

  rb_check_frozen(self);

  if (argc == 1)
  {
    strb_RubyToRect(argv[0], &(rect));
  } else if (argc == 4)  {
    rect.x      = NUM2INT(argv[0]);
    rect.y      = NUM2INT(argv[1]);
    rect.width  = NUM2INT(argv[2]);
    rect.height = NUM2INT(argv[3]);
  } else {
    rb_raise(rb_eArgError, "expected 1 or 4 arguments but recieved %d", argc);
  }
  Data_Get_Struct(self, Texture, texture);
  if (!ModifyRectInTexture_Rect(texture, &(rect))) {
    return Qnil;
  }
  strb_TextureCheckDisposed(texture);

  pixels = &(texture->pixels[rect.x + rect.y * texture->width]);
  padding = texture->width - rect.width;
  for (int32_t y = 0; y < rect.height; y++, pixels += padding) {
    for (int32_t x = 0; x < rect.width; x++, pixels++) {
      pixels->value = 0x00000000;
    }
  }
  return Qnil;
}
