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

/*
  if x < rect.x
    w -= rect.x - x
    x = rect.x
  end
  if y < rect.y
    h -= rect.y - y
    y = rect.y
  end
  if (x2 = x + w) > (rx2 = rect.x + rect.width)
    w -= x2 - rx2
  end
  if (y2 = y + h) > (ry2 = rect.y + rect.height)
    h -= y2 - ry2
  end
 */

inline static bool
ModifyRectInTexture(const Texture* texture,
                    int* const x, int* const y,
                    int* const width, int* const height)
{
  int32_t cx = 0, cy = 0, cw = texture->width, ch = texture->height;

  if (texture->clip_rect != Qnil) {
    Rect* rect;
    Data_Get_Struct(texture->clip_rect, Rect, rect);
    cx = rect->x;
    cy = rect->y;
    cw = rect->width;
    ch = rect->height;
  }

  if (*x < cx) {
    *width -= cx - *x;
    *x = cx;
  }
  if (*y < cy) {
    *height -= cy - *y;
    *y = cy;
  }
  /* Is the X, Y outside the texture bounds? */
  if ((int32_t)cw <= *x || (int32_t)ch <= *y) {
    return false;
  }
  const int32_t x2 = *x + *width;
  const int32_t y2 = *y + *height;
  const int32_t rx2 = cx + cw;
  const int32_t ry2 = cy + ch;
  if (x2 > rx2) {
    *width -= x2 - rx2;
  }
  if (y2 > ry2) {
    *height -= y2 - ry2;
  }
  if (*width <= 0 || *height <= 0) {
    return false;
  }
  return true;
}

inline static bool ModifyRectInTexture_Rect(Texture* texture, Rect* rect)
{
  return ModifyRectInTexture(texture, &(rect->x), &(rect->y),
                                      &(rect->width), &(rect->height));
}

inline static bool
Texture_is_binded(Texture *texture)
{
  return texture->binded;
}
