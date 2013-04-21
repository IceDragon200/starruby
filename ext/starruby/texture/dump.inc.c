static VALUE
Texture_dump(VALUE self, VALUE rbFormat)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  const char* format = StringValuePtr(rbFormat);
  const int formatLength = RSTRING_LEN(rbFormat);
  const int pixelLength = texture->width * texture->height;
  volatile VALUE rbResult = rb_str_new(NULL, pixelLength * formatLength);
  uint8_t* strPtr = (uint8_t*)RSTRING_PTR(rbResult);
  const Pixel* pixels = texture->pixels;
  for (int i = 0; i < pixelLength; i++, pixels++) {
    for (int j = 0; j < formatLength; j++, strPtr++) {
      switch (format[j]) {
      case 'r': *strPtr = pixels->color.red;   break;
      case 'g': *strPtr = pixels->color.green; break;
      case 'b': *strPtr = pixels->color.blue;  break;
      case 'a': *strPtr = pixels->color.alpha; break;
      }
    }
  }
  return rbResult;
}

static VALUE
Texture_undump(VALUE self, VALUE rbData, VALUE rbFormat)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);

  const char* format = StringValuePtr(rbFormat);
  const int formatLength = RSTRING_LEN(rbFormat);
  const int pixelLength = texture->width * texture->height;
  Check_Type(rbData, T_STRING);
  if (pixelLength * formatLength != RSTRING_LEN(rbData)) {
    rb_raise(rb_eArgError, "invalid data size: %d expected but was %ld",
             pixelLength * formatLength, RSTRING_LEN(rbData));
  }
  const uint8_t* data = (uint8_t*)RSTRING_PTR(rbData);
  Pixel* pixels = texture->pixels;
  for (int i = 0; i < pixelLength; i++, pixels++) {
    for (int j = 0; j < formatLength; j++, data++) {
      switch (format[j]) {
      case 'r': pixels->color.red   = *data; break;
      case 'g': pixels->color.green = *data; break;
      case 'b': pixels->color.blue  = *data; break;
      case 'a': pixels->color.alpha = *data; break;
      }
    }
  }
  return self;
}
