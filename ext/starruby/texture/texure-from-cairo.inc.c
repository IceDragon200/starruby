static void
strb_cairo_surface_to_texture_ARGB(cairo_surface_t* cr_surface,
                                   Texture* strb_texture)
{
  const Short width  = cairo_image_surface_get_width(cr_surface);
  const Short height = cairo_image_surface_get_height(cr_surface);
  const Integer length = width * height;

  CairoColor32* data = (CairoColor32*)cairo_image_surface_get_data(cr_surface);

  for(Integer i = 0; i < length; i++)
  {
    strb_texture->pixels[i].color.red   = data[i].red;
    strb_texture->pixels[i].color.green = data[i].green;
    strb_texture->pixels[i].color.blue  = data[i].blue;
    strb_texture->pixels[i].color.alpha = data[i].alpha;
  }
}

VALUE strb_TextureFromCairoSurface(cairo_surface_t* cr_surface)
{
  const Short width  = cairo_image_surface_get_width(cr_surface);
  const Short height = cairo_image_surface_get_height(cr_surface);
  VALUE rbTexture = rb_class_new_instance(2,
                                          (VALUE[]){INT2NUM(width),
                                                    INT2NUM(height)},
                                                    rb_cTexture);

  Texture* trg_texture;
  Data_Get_Struct(rbTexture, Texture, trg_texture);

  switch(cairo_image_surface_get_format(cr_surface))
  {
    case CAIRO_FORMAT_INVALID:
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
    case CAIRO_FORMAT_RGB16_565:
    case CAIRO_FORMAT_RGB30:
      rb_raise(rb_eStarRubyError, "Unsupported format");
      break;
    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_ARGB32: // 0xAARRGGBB
      switch(cairo_surface_get_content(cr_surface))
      {
        case CAIRO_CONTENT_COLOR:
        case CAIRO_CONTENT_COLOR_ALPHA:
          strb_cairo_surface_to_texture_ARGB(cr_surface, trg_texture);
          break;
        case CAIRO_CONTENT_ALPHA:
          rb_raise(rb_eStarRubyError, "Cannot load Alpha only Image");
          return Null;
      }
      break;
  }

  return rbTexture;
}
