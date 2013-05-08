static VALUE Texture_initialize(VALUE self, VALUE rbWidth, VALUE rbHeight)
{
  Texture* texture;
  Integer width  = NUM2INT(rbWidth);
  Integer height = NUM2INT(rbHeight);
  Data_Get_Struct(self, Texture, texture);

  if (width <= 0) {
    rb_raise(rb_eArgError, "Given width for Texture must be greater than 0");
  }
  if (height <= 0) {
    rb_raise(rb_eArgError, "Given height for Texture must be greater than 0");
  }

  texture->width  = width;
  texture->height = height;
  strb_TextureAllocData(texture);

  return Qnil;
}
