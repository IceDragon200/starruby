
Texture* strb_TextureCrop(Texture *src_texture, Rect *crop_rect)
{
  Texture *dst_texture;
  int32_t x      = crop_rect->x;
  int32_t y      = crop_rect->y;
  int32_t width  = crop_rect->width;
  int32_t height = crop_rect->height;
  if (!ModifyRectInTexture(src_texture, &(x), &(y), &(width), &(height))) {
    return NULL;
  }
  dst_texture = strb_TextureMakeNew(width, height);
  strb_TextureRender(src_texture, dst_texture, x, y, width, height,
                     0, 0, 0xFF, NULL, NULL, BLEND_TYPE_NONE);
  return dst_texture;
}

static VALUE Texture_crop(int argc, VALUE *argv, VALUE self)
{
  Rect rect;
  Texture* dst_texture;
  Texture* src_texture;
  VALUE rbTexture;

  rb_check_frozen(self);

  if (argc == 1) {
    VALUE rbObj = argv[0];
    if (rb_obj_is_kind_of(rbObj, rb_cRect)) {
      strb_RubyToRect(rbObj, &(rect));
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

