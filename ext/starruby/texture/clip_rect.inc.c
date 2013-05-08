static VALUE Texture_clip_rect(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  return texture->clip_rect;
}

static VALUE Texture_clip_rect_eq(VALUE self, VALUE rbRect)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckObjIsKindOf(rbRect, rb_cRect);
  texture->clip_rect = rbRect;
  return Qnil;
}
