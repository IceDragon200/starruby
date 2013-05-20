#define STRB_TEXTURE_LOAD_PNG

static VALUE Texture_s_load_png(VALUE self, VALUE rbPath)
{
  Check_Type(rbPath, T_STRING);
  VALUE rbFullPath = strb_GetCompletePath(rb_obj_dup(rbPath), false);

  if(NIL_P(rbFullPath))
    rbFullPath = rb_obj_dup(rbPath);
    //rb_raise(rb_eStarRubyError, "Could not resolve basepath");

  const String filename      = StringValueCStr(rbPath);
  const String filename_full = StringValueCStr(rbFullPath);

  cairo_surface_t* cr_surface = cairo_image_surface_create_from_png(filename_full);
  // CAIRO_STATUS_SUCCESS
  cairo_status_t status = cairo_surface_status(cr_surface);
  if(status == CAIRO_STATUS_NULL_POINTER) {
    rb_raise(rb_eStarRubyError, "C ERROR: NULL Surface Pointer");
  } else if(status == CAIRO_STATUS_NO_MEMORY) {
    rb_raise(rb_eStarRubyError, "No Memory");
  } else if(status == CAIRO_STATUS_READ_ERROR) {
    rb_raise(rb_eStarRubyError, "Read error occured in png file");
  } else if(status == CAIRO_STATUS_INVALID_CONTENT) {
    rb_raise(rb_eStarRubyError, "Inavlid PNG content");
  } else if(status == CAIRO_STATUS_INVALID_FORMAT) {
    rb_raise(rb_eStarRubyError, "Inavlid PNG Format");
  } else if(status == CAIRO_STATUS_INVALID_VISUAL) {
    rb_raise(rb_eStarRubyError, "Invalid PNG Visual");
  } else if(status == CAIRO_STATUS_FILE_NOT_FOUND) {
    rb_raise(rb_GetErrno("ENOENT"), "%s", filename);
  }

  cairo_surface_flush(cr_surface);
  VALUE rbTexture = strb_TextureFromCairoSurface(cr_surface);

  //cairo_surface_mark_dirty(cr_surface);
  cairo_surface_destroy(cr_surface);

  return rbTexture;
}
