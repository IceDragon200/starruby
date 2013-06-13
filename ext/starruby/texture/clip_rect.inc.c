static VALUE Texture_clip_rect(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  if (NIL_P(texture->clip_rect)) {
    Rect *rect;
    texture->clip_rect = rb_class_new_instance(0, (VALUE[]){}, rb_cRect);
    Data_Get_Struct(texture->clip_rect, Rect, rect);
    rect->x = 0;
    rect->y = 0;
    rect->width = texture->width;
    rect->height = texture->height;
  }
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
