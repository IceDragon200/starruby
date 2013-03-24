static VALUE
Texture_aref(VALUE self, VALUE rbX, VALUE rbY)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    rb_raise(rb_eArgError, "index out of range: (%d, %d)", x, y);
  }
  const Color color = texture->pixels[x + y * texture->width].color;
  return rb_funcall(strb_GetColorClass(), rb_intern("new"), 4,
                    INT2FIX(color.red),
                    INT2FIX(color.green),
                    INT2FIX(color.blue),
                    INT2FIX(color.alpha));
}

static VALUE
Texture_aset(VALUE self, VALUE rbX, VALUE rbY, VALUE rbColor)
{
  rb_check_frozen(self);
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);

  const int x = NUM2INT(rbX);
  const int y = NUM2INT(rbY);
  if (x < 0 || texture->width <= x || y < 0 || texture->height <= y) {
    return Qnil;
  }
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);
  texture->pixels[x + y * texture->width].color = color;
  return rbColor;
}
