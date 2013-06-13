Texture* strb_TextureAlloc()
{
  Texture* texture = ALLOC(Texture);
  texture->pixels      = NULL;
  texture->width       = 0;
  texture->height      = 0;
  texture->clip_rect   = Qnil;
  texture->binded      = false;
  texture->paletteSize = 0;
  texture->palette     = NULL;
  texture->indexes     = NULL;
  return texture;
}

static void
Texture_mark(Texture* texture)
{
  if (texture && !NIL_P(texture->clip_rect)) {
    rb_gc_mark(texture->clip_rect);
  }
}

void strb_TextureAllocData(Texture* texture)
{
  const long length = texture->width * texture->height;
  assert(length > 0);
  texture->pixels = ALLOC_N(Pixel, length);
  MEMZERO(texture->pixels, Pixel, length);
}

Texture* strb_TextureMakeNew(const int32_t width, const int32_t height)
{
  Texture* texture = strb_TextureAlloc();
  texture->width  = width;
  texture->height = height;
  strb_TextureAllocData(texture);
  return texture;
}

void strb_TextureFreePixels(Texture* texture)
{
  if (texture) {
    if (!(texture->binded)) {
      if (texture->pixels) {
        free(texture->pixels);
      }
    }
    texture->pixels = NULL;
  }
}

void strb_TextureFree(Texture* texture)
{
  if (texture) {
    strb_TextureFreePixels(texture);
    texture->clip_rect = Qnil;
    free(texture);
    texture = NULL;
  }
}

static VALUE Texture_disposed(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  if (!texture) {
    return Qfalse;
  }
  return hrbBooleanToRuby(!texture->pixels);
}

static VALUE Texture_alloc(VALUE klass)
{
  Texture* texture = strb_TextureAlloc();
  return Data_Wrap_Struct(klass, Texture_mark, strb_TextureFree, texture);
}

static VALUE Texture_dispose(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  if (texture) {
    strb_TextureFreePixels(texture);
  }
  return Qnil;
}
