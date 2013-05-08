inline bool
strb_Texture_is_disposed(const Texture* const texture)
{
  return !texture->pixels;
}

inline void
strb_TextureCheckDisposed(const Texture* const texture)
{
  if (!texture->pixels) {
    rb_raise(rb_eRuntimeError,
             "can't modify disposed StarRuby::Texture");
  }
}

inline static bool
ModifyRectInTexture(const Texture* texture,
                    int* const x, int* const y,
                    int* const width, int* const height)
{
  Integer cx = 0, cy = 0, cw = texture->width, ch = texture->height;

  if (texture->clip_rect != Qnil) {
    Rect* rect;
    Data_Get_Struct(texture->clip_rect, Rect, rect);
    cx = rect->x;
    cy = rect->y;
    cw = rect->width;
    ch = rect->height;
  }

  if (*x < cx) {
    *width -= *x - cx;
    *x = cx;
  }
  if (*y < cy) {
    *height -= *y - cy;
    *y = cy;
  }
  /* Is the X, Y outside the texture bounds? */
  if ((Integer)cw <= *x || (Integer)ch <= *y) {
    return false;
  }
  if ((Integer)cw <= *x + *width) {
    *width = cw - *x;
  }
  if ((Integer)ch <= *y + *height) {
    *height = ch - *y;
  }
  if (*width <= 0 || *height <= 0) {
    return false;
  }
  return true;
}

inline static Boolean ModifyRectInTexture_Rect(Texture* texture, Rect* rect)
{
  return ModifyRectInTexture(texture, &(rect->x), &(rect->y),
                                      &(rect->width), &(rect->height));
}

inline static Boolean
Texture_is_binded(Texture *texture)
{
  return texture->binded;
}
