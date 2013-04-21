
Texture* strb_TextureCrop(Texture *src_texture, Rect *crop_rect)
{
  Texture *dst_texture;
  Integer x      = crop_rect->x;
  Integer y      = crop_rect->y;
  Integer width  = crop_rect->width;
  Integer height = crop_rect->height;
  if (!ModifyRectInTexture(src_texture, &(x), &(y), &(width), &(height))) {
    return Null;
  }
  dst_texture = strb_TextureMakeNew(width, height);
  strb_TextureRender(src_texture, dst_texture, x, y, width, height,
                     0, 0, 0xFF, Null, Null, BLEND_TYPE_NONE);
  return dst_texture;
}

static VALUE Texture_crop(Size argc, VALUE *argv, VALUE self)
{
  rb_check_frozen(self);
  VALUE rbTexture;
  Texture* src_texture;
  Texture* dst_texture;
  Rect rect = (Rect){ 0, 0, 0, 0 };
  if (argc == 1) {
    VALUE rbObj = argv[0];
    if (rb_obj_is_kind_of(rbObj, rb_cRect)) {
      Rect *tmp_rect;
      Data_Get_Struct(rbObj, Rect, tmp_rect);
      rect.x      = tmp_rect->x;
      rect.y      = tmp_rect->y;
      rect.width  = tmp_rect->width;
      rect.height = tmp_rect->width;
    } else {
      rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)",
               rb_obj_classname(rbObj), rb_class2name(rb_cRect));
    }
  } else if (argc == 4) {
    rect.x      = NUM2INT(argv[0]);
    rect.y      = NUM2INT(argv[1]);
    rect.width  = NUM2INT(argv[2]);
    rect.height = NUM2INT(argv[3]);
  } else {
    rb_raise(rb_eArgError, "wrong number of arguments (expected 1 or 4)");
  }
  Data_Get_Struct(self, Texture, src_texture);
  strb_TextureCheckDisposed(src_texture);
  dst_texture = strb_TextureCrop(src_texture, &(rect));
  if (dst_texture) {
    /* HACK */
    rbTexture = rb_obj_alloc(rb_cTexture);
    strb_TextureFree(DATA_PTR(rbTexture));
    RDATA(rbTexture)->data = dst_texture;
    return rbTexture;
  } else {
    return Qnil;
  }
}

