inline bool
strb_Texture_is_disposed(const Texture* const texture)
{
  return !texture->pixels;
}

inline void
strb_CheckDisposedTexture(const Texture* const texture)
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
  if (*x < 0) {
    *width -= -(*x);
    *x = 0;
  }
  if (*y < 0) {
    *height -= -(*y);
    *y = 0;
  }
  if ((int32_t)texture->width <= *x || (int32_t)texture->height <= *y) {
    return false;
  }
  if ((int32_t)texture->width <= *x + *width) {
    *width = texture->width - *x;
  }
  if ((int32_t)texture->height <= *y + *height) {
    *height = texture->height - *y;
  }
  if (*width <= 0 || *height <= 0) {
    return false;
  }
  return true;
}

inline static bool
Texture_is_binded(Texture *texture)
{
  return texture->binded;
}
