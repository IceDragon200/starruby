static VALUE
Texture_render_text(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbText, rbX, rbY, rbFont, rbColor, rbAntiAlias;

  rb_scan_args(argc, argv, "51",
               &rbText, &rbX, &rbY, &rbFont, &rbColor, &rbAntiAlias);

  Check_Type(rbText, T_STRING);

  if (!(RSTRING_LEN(rbText))) {
    return self;
  }

  const bool antiAlias = RTEST(rbAntiAlias);
  const char* text = StringValueCStr(rbText);

  strb_CheckFont(rbFont);

  const Font* font;

  Data_Get_Struct(rbFont, Font, font);

  volatile VALUE rbSize = rb_funcall(rbFont, rb_intern("get_size"), 1, rbText);
  volatile VALUE rbTextTexture =
    rb_class_new_instance(2, RARRAY_PTR(rbSize), rb_cTexture);

  const Texture* textTexture;
  Data_Get_Struct(rbTextTexture, Texture, textTexture);
  Color color;
  strb_GetColorFromRubyValue(&color, rbColor);

  SDL_Surface* textSurfaceRaw;
  if (antiAlias) {
    textSurfaceRaw =
      TTF_RenderUTF8_Shaded(font->sdlFont, text,
                            (SDL_Color){255, 255, 255, 255},
                            (SDL_Color){0, 0, 0, 0});
  } else {
    textSurfaceRaw =
      TTF_RenderUTF8_Solid(font->sdlFont, text,
                           (SDL_Color){255, 255, 255, 255});
  }
  if (!textSurfaceRaw) {
    rb_raise_sdl_ttf_error();
  }
  SDL_PixelFormat format = {
    .palette = NULL, .BitsPerPixel = 32, .BytesPerPixel = 4,
    .Rmask = 0x00ff0000, .Gmask = 0x0000ff00,
    .Bmask = 0x000000ff, .Amask = 0xff000000,
    .colorkey = 0, .alpha = 255,
  };
  SDL_Surface* textSurface =
    SDL_ConvertSurface(textSurfaceRaw, &format, SDL_SWSURFACE);
  SDL_FreeSurface(textSurfaceRaw);
  textSurfaceRaw = NULL;
  if (!textSurface) {
    rb_raise_sdl_error();
  }
  SDL_LockSurface(textSurface);
  const Pixel* src = (Pixel*)(textSurface->pixels);
  Pixel* dst = textTexture->pixels;
  const int size = textTexture->width * textTexture->height;
  for (int i = 0; i < size; i++, src++, dst++) {
    if (src->value) {
      dst->color = color;
      if (color.alpha == 255) {
        dst->color.alpha = src->color.red;
      } else {
        dst->color.alpha = DIV255(src->color.red * color.alpha);
      }
    }
  }
  SDL_UnlockSurface(textSurface);
  SDL_FreeSurface(textSurface);
  textSurface = NULL;

  Texture *dstTexture;
  Data_Get_Struct(self, Texture, dstTexture);

  RenderTexture(textTexture, dstTexture,
    0, 0, textTexture->width, textTexture->height, NUM2INT(rbX), NUM2INT(rbY),
    255, NULL, NULL, 1);

  Texture_dispose(rbTextTexture);
  return self;
}
