#define STARRUBY_TEXTURE_LOAD_SVG
static VALUE
Texture_s_load_svg(VALUE self, VALUE rbPath, VALUE rbWidth, VALUE rbHeight)
{
  Check_Type(rbPath, T_STRING);
  VALUE rbTexture;
  VALUE rbFullPath = strb_GetCompletePath(rb_obj_dup(rbPath), False);

  if(NIL_P(rbFullPath))
    rbFullPath = rb_obj_dup(rbPath);

  /*const String filename = StringValueCStr(rbPath);*/
  const String filepath = StringValueCStr(rbFullPath);
  const Integer width = NUM2INT(rbWidth);
  const Integer height = NUM2INT(rbHeight);

  GError *error = Null;
  RsvgHandle* hndle = rsvg_handle_new_from_file(filepath, &error);
  RsvgDimensionData* src_dims = Null;
  rsvg_handle_get_dimensions(hndle, src_dims);
  cairo_surface_t* cr_surface =
    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

  cairo_status_t status = cairo_surface_status(cr_surface);
  if(status == CAIRO_STATUS_NULL_POINTER) {
    rb_raise(rb_eStarRubyError, "C ERROR: Null Surface Pointer");
  } else if(status == CAIRO_STATUS_NO_MEMORY) {
    rb_raise(rb_eStarRubyError, "No Memory");
  }

  cairo_t* cr_context = cairo_create(cr_surface);
  cairo_matrix_t* matrix = Null;
  cairo_get_matrix(cr_context, matrix);
  cairo_matrix_init_scale(matrix,
                          (Double)width / (Double)src_dims->width,
                          (Double)height / (Double)src_dims->height);
  cairo_set_matrix(cr_context, matrix);
  rsvg_handle_render_cairo(hndle, cr_context);
  //rsvg_handle_render_cairo_sub(hndle, cr_context, Null);
  cairo_surface_flush(cr_surface);
  //cairo_surface_finish(cr_surface);
  rbTexture = strb_TextureFromCairoSurface(cr_surface);

  cairo_destroy(cr_context);
  cairo_surface_destroy(cr_surface);

  return rbTexture;
}
