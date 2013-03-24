static VALUE
Texture_disposed(VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  return !texture->pixels ? Qtrue : Qfalse;
}

static void
Texture_free(Texture* texture)
{
  if(!(texture->binded)) {
    if(texture->pixels) free(texture->pixels);
  }
  texture->pixels = NULL;

  free(texture);
}

static VALUE
Texture_alloc(VALUE klass)
{
  Texture* texture = ALLOC(Texture);
  texture->width  = 0;
  texture->height = 0;
  texture->binded = false;
  texture->pixels = NULL;
  return Data_Wrap_Struct(klass, NULL, Texture_free, texture);
}

static VALUE
Texture_dispose(VALUE self)
{
  Texture* texture;
  Data_Get_Struct(self, Texture, texture);

  if(!Texture_is_binded(texture))
    free(texture->pixels);

  texture->pixels = NULL;
  return Qnil;
}
