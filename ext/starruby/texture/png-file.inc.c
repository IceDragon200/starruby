static inline VALUE
rb_GetErrno(char* name)
{
  return rb_const_get(rb_const_get(rb_cObject, rb_intern("Errno")), rb_intern(name));
}

static void
strb_cairo_surface_to_texture_ARGB(cairo_surface_t* cr_surface,
                                     Texture* strb_texture)
{
  const uint32_t width  = cairo_image_surface_get_width(cr_surface);
  const uint32_t height = cairo_image_surface_get_height(cr_surface);
  const uint64_t length = width * height;

  Color32* data = (Color32*)cairo_image_surface_get_data(cr_surface);

  for(uint64_t i = 0; i < length; i++)
  {
    Color32 px = data[i];
    strb_texture->pixels[i].color.red   = px.red;
    strb_texture->pixels[i].color.green = px.green;
    strb_texture->pixels[i].color.blue  = px.blue;
    strb_texture->pixels[i].color.alpha = px.alpha;
  }
}

//static void
//strb_cairo_surface_to_texture_RGB(cairo_surface_t* cr_surface,
//                                    Texture* strb_texture)
//{
//  strb_cairo_surface_to_texture_ARGB(cr_surface, strb_texture);
//}

static VALUE
Texture_s_load(int argc, VALUE* argv, VALUE self)
{
  VALUE rbPath;
  rb_scan_args(argc, argv, "10", &rbPath);
  Check_Type(rbPath, T_STRING);

  VALUE rbFullPath = strb_GetCompletePath(rb_obj_dup(rbPath), false);

  if(NIL_P(rbFullPath))
    rbFullPath = rb_obj_dup(rbPath);
    //rb_raise(rb_eStarRubyError, "Could not resolve basepath");

  const char* filename = StringValuePtr(rbPath);
  const char* filename_full = StringValuePtr(rbFullPath);

  cairo_surface_t* cr_surface = cairo_image_surface_create_from_png(filename_full);

  // CAIRO_STATUS_SUCCESS
  cairo_status_t status = cairo_surface_status(cr_surface);
  if(status == CAIRO_STATUS_NULL_POINTER)
    rb_raise(rb_eStarRubyError, "C ERROR: Null Surface Pointer");
  else if(status == CAIRO_STATUS_NO_MEMORY)
    rb_raise(rb_eStarRubyError, "No Memory");
  else if(status == CAIRO_STATUS_READ_ERROR)
    rb_raise(rb_eStarRubyError, "Read error occured in png file");
  else if(status == CAIRO_STATUS_INVALID_CONTENT)
    rb_raise(rb_eStarRubyError, "Inavlid PNG content");
  else if(status == CAIRO_STATUS_INVALID_FORMAT)
    rb_raise(rb_eStarRubyError, "Inavlid PNG Format");
  else if(status == CAIRO_STATUS_INVALID_VISUAL)
    rb_raise(rb_eStarRubyError, "Invalid PNG Visual");
  else if(status == CAIRO_STATUS_FILE_NOT_FOUND)
    rb_raise(rb_GetErrno("ENOENT"), filename);

  cairo_surface_flush(cr_surface);

  const uint32_t width  = cairo_image_surface_get_width(cr_surface);
  const uint32_t height = cairo_image_surface_get_height(cr_surface);
  volatile VALUE rbTexture = rb_class_new_instance(2,
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
      rb_raise(rb_eStarRubyError,
               "this png (%s) format is not supported, use RGB or ARGB instead",
               filename);
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
          rb_raise(rb_eStarRubyError, "Cannot load Alpha only PNG");
          break;
      }
      break;
  }

  //cairo_surface_mark_dirty(cr_surface);
  cairo_surface_destroy(cr_surface);

  return rbTexture;
}

static VALUE
Texture_save(VALUE self, VALUE rbPath)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_CheckDisposedTexture(texture);
  const char* path = StringValueCStr(rbPath);
  FILE* fp = fopen(path, "wb");
  if (!fp) {
    rb_raise(rb_path2class("Errno::ENOENT"), "%s", path);
  }
  png_structp pngPtr =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop infoPtr = png_create_info_struct(pngPtr);
  png_init_io(pngPtr, fp);
  png_set_IHDR(pngPtr, infoPtr, texture->width, texture->height,
               8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(pngPtr, infoPtr);
  for (int j = 0; j < texture->height; j++) {
    png_byte row[texture->width * 4];
    for (int i = 0; i < texture->width; i++) {
      const Color* c = &(texture->pixels[texture->width * j + i].color);
      png_byte* const r = &(row[i * 4]);
      r[0] = c->red;
      r[1] = c->green;
      r[2] = c->blue;
      r[3] = c->alpha;
    }
    png_write_row(pngPtr, row);
  }
  png_write_end(pngPtr, infoPtr);
  png_destroy_write_struct(&pngPtr, &infoPtr);
  fclose(fp);
  return Qnil;
}
