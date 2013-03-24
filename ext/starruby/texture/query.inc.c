static VALUE
Texture_width(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  return INT2NUM(texture->width);
}

static VALUE
Texture_height(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  return INT2NUM(texture->height);
}

static VALUE
Texture_size(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  volatile VALUE rbSize =
    rb_assoc_new(INT2NUM(texture->width), INT2NUM(texture->height));
  OBJ_FREEZE(rbSize);
  return rbSize;
}

static VALUE
Texture_rect(VALUE self)
{
  Texture *texture;
  Data_Get_Struct(self, Texture, texture);
  VALUE rbArgv[4] = { INT2NUM(0), INT2NUM(0),
                      INT2NUM(texture->width), INT2NUM(texture->height) };
  return rb_class_new_instance(4, rbArgv, strb_GetRectClass());
}
