/*
  StarRuby Texture Cairo Bindings

    for interfacing with ruby cairo.
  */

static VALUE
Texture_bind_cairo(VALUE self, VALUE cairo_surface)
{
  rb_check_frozen(self);
  Texture *texture;
  Data_Get_Struct(self, Texture, texture);
  cairo_surface_t *cr_surface;
  Data_Get_Struct(cairo_surface, cairo_surface_t, cr_surface);

  if(texture->binded)
    rb_raise(rb_eStarRubyError, "Texture was already binded");

  if(texture->pixels)
    free(texture->pixels); // get rid of the old data

  texture->width = cairo_image_surface_get_width(cr_surface);
  texture->height = cairo_image_surface_get_height(cr_surface);
  texture->pixels = (Pixel*)cairo_image_surface_get_data(cr_surface);
  texture->binded = true;

  cairo_surface_mark_dirty(cr_surface);

  return Qnil;
}

static VALUE
Texture_s_bind_from_cairo(VALUE klass, VALUE cairo_surface)
{
  VALUE obj = rb_obj_alloc(klass);
  Texture_bind_cairo(obj, cairo_surface);
  return obj;
}

static VALUE Texture_unbind(VALUE self)
{
  rb_check_frozen(self);
  Texture *texture;
  Data_Get_Struct(self, Texture, texture);
  texture->pixels = NULL;
  texture->binded = false;

  return Qnil;
}
