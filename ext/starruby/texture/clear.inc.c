static VALUE
Texture_clear(VALUE self)
{
  const Texture* texture;
  rb_check_frozen(self);
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  Integer length = texture->width * texture->height;
  Pixel* pixels = texture->pixels;
  for (Integer i = 0; i < length; i++, pixels++) {
    pixels->value = 0x00000000;
  }
  //MEMZERO(texture->pixels, Pixel, texture->width * texture->height);
  return self;
}

static VALUE
Texture_clear_rect(Integer argc, VALUE* argv, VALUE self)
{
  Integer padding;
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
  for (Integer y = 0; y < rect.height; y++, pixels += padding) {
    for (Integer x = 0; x < rect.width; x++, pixels++) {
      pixels->value = 0x00000000;
    }
  }
  return Qnil;
}
