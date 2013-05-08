static VALUE
Texture_initialize_copy(VALUE self, VALUE rbTexture)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  const Texture* origTexture;
  Data_Get_Struct(rbTexture, Texture, origTexture);
  texture->width  = origTexture->width;
  texture->height = origTexture->height;
  const Bignum length = texture->width * texture->height;
  texture->pixels = ALLOC_N(Pixel, length);
  MEMCPY(texture->pixels, origTexture->pixels, Pixel, length);
  texture->binded = false;

  return Qnil;
}
