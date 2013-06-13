#define STRB_TEXTURE_LOAD_PNG
#define USE_LIBPNG_LOAD_PNG
//#define USE_CAIRO_LOAD_PNG

#ifdef HAVE_CAIRO
static void
strb_cairo_surface_to_texture_ARGB(cairo_surface_t* cr_surface,
                                   Texture* strb_texture)
{
  const int16_t width  = cairo_image_surface_get_width(cr_surface);
  const int16_t height = cairo_image_surface_get_height(cr_surface);
  const int32_t length = width * height;

  CairoColor32* data = (CairoColor32*)cairo_image_surface_get_data(cr_surface);

  for(int32_t i = 0; i < length; i++)
  {
    strb_texture->pixels[i].color.red   = data[i].red;
    strb_texture->pixels[i].color.green = data[i].green;
    strb_texture->pixels[i].color.blue  = data[i].blue;
    strb_texture->pixels[i].color.alpha = data[i].alpha;
  }
}

VALUE strb_TextureFromCairoSurface(cairo_surface_t* cr_surface)
{
  const int16_t width  = cairo_image_surface_get_width(cr_surface);
  const int16_t height = cairo_image_surface_get_height(cr_surface);
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
      }
      break;
  }

  return rbTexture;
}
#endif

#ifdef USE_LIBPNG_LOAD_PNG

typedef struct {
  char* bytes;
  unsigned long size;
  unsigned long offset;
} PngBuffer;

static void
ReadPng(png_structp pngPtr, png_bytep buf, png_size_t size)
{
  PngBuffer* pngBuffer = (PngBuffer*)png_get_io_ptr(pngPtr);
  if (pngBuffer->offset + size <= pngBuffer->size) {
    MEMCPY(buf, &(pngBuffer->bytes[pngBuffer->offset]), char, size);
    pngBuffer->offset += size;
  } else {
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG data");
  }
}

static VALUE
Texture_s_load_png(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbPathOrIO, rbOptions;
  rb_scan_args(argc, argv, "11", &rbPathOrIO, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  }
  const bool hasPalette = RTEST(rb_hash_aref(rbOptions, symbol_palette));
  unsigned long ioLength = 0;
  volatile VALUE val;
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_io_length))) {
    if (RTEST(rb_obj_is_kind_of(val, rb_cNumeric))) {
      if (RTEST(rb_funcall(val, rb_intern("<="), 1, INT2FIX(0)))) {
        rb_raise(rb_eArgError, "invalid io_length");
      }
      ioLength = NUM2ULONG(val);
      if (ioLength <= 8) {
        rb_raise(rb_eArgError, "invalid io_length");
      }
    } else {
      rb_raise(rb_eTypeError, "wrong argument type %s (expected Numeric)",
               rb_obj_classname(rbPathOrIO));
    }
  }

  png_structp pngPtr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pngPtr) {
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
  }
  png_infop infoPtr = png_create_info_struct(pngPtr);
  if (!infoPtr) {
    png_destroy_read_struct(&pngPtr, NULL, NULL);
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
  }
  png_infop endInfo = png_create_info_struct(pngPtr);
  if (!endInfo) {
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
    rb_raise(strb_GetStarRubyErrorClass(), "PNG error");
  }

  volatile VALUE rbIO, rbIOToClose = Qnil;
  if (TYPE(rbPathOrIO) == T_STRING) {
    volatile VALUE rbCompletePath = strb_GetCompletePath(rbPathOrIO, true);
    volatile VALUE rbOpenOption = rb_str_new2("rb");
    rbIOToClose = rbIO =
      rb_funcall(rb_mKernel, rb_intern("open"), 2,
                 rbCompletePath, rbOpenOption);
  } else if (rb_respond_to(rbPathOrIO, rb_intern("read"))) {
    rbIO = rbPathOrIO;
  } else {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected String or IO)",
             rb_obj_classname(rbPathOrIO));
  }
  volatile VALUE rbHeader = rb_funcall(rbIO, rb_intern("read"), 1, INT2FIX(8));
  if (NIL_P(rbHeader)) {
    if (!NIL_P(rbIOToClose)) {
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    }
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG file (none header)");
  }
  png_byte header[8];
  MEMCPY(header, StringValuePtr(rbHeader), png_byte, 8);
  if (png_sig_cmp(header, 0, 8)) {
    if (!NIL_P(rbIOToClose)) {
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    }
    rb_raise(strb_GetStarRubyErrorClass(), "invalid PNG file (invalid header)");
  }
  volatile VALUE rbData =
    ioLength == 0 ?
    rb_funcall(rbIO, rb_intern("read"), 0) :
    rb_funcall(rbIO, rb_intern("read"), 1, ULONG2NUM(ioLength - 8));
  PngBuffer pngBuffer = {
    .bytes = StringValuePtr(rbData),
    .size = RSTRING_LEN(rbData),
    .offset = 0,
  };
  png_set_read_fn(pngPtr, (png_voidp)(&pngBuffer), (png_rw_ptr)ReadPng);
  png_set_sig_bytes(pngPtr, 8);
  png_read_info(pngPtr, infoPtr);
  png_uint_32 width, height;
  int bitDepth, colorType, interlaceType;
  png_get_IHDR(pngPtr, infoPtr, &width, &height,
               &bitDepth, &colorType, &interlaceType, NULL, NULL);
  if (interlaceType != PNG_INTERLACE_NONE) {
    png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
    if (!NIL_P(rbIOToClose)) {
      rb_funcall(rbIOToClose, rb_intern("close"), 0);
    }
    rb_raise(strb_GetStarRubyErrorClass(),
             "not supported interlacing PNG image");
  }

  volatile VALUE rbTexture =
    rb_class_new_instance(2, (VALUE[]){INT2NUM(width), INT2NUM(height)}, self);
  Texture* texture;
  Data_Get_Struct(rbTexture, Texture, texture);

  if (bitDepth == 16) {
    png_set_strip_16(pngPtr);
  }
  if (colorType == PNG_COLOR_TYPE_PALETTE && !hasPalette) {
    png_set_palette_to_rgb(pngPtr);
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(pngPtr);
    }
  }
  if (bitDepth < 8) {
    png_set_packing(pngPtr);
  }
  if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
#if 15 <= PNG_LIBPNG_VER_SONUM
    png_set_expand_gray_1_2_4_to_8(pngPtr);
#else
    png_set_gray_1_2_4_to_8(pngPtr);
#endif
  }
  png_read_update_info(pngPtr, infoPtr);
  png_colorp palette = NULL;
  int numPalette = 0;
  png_get_PLTE(pngPtr, infoPtr, &palette, &numPalette);
  if (0 < numPalette && hasPalette) {
    texture->indexes = ALLOC_N(uint8_t, width * height);
    png_bytep trans = NULL;
    int numTrans = 0;
    png_get_tRNS(pngPtr, infoPtr, &trans, &numTrans, NULL);
    texture->paletteSize = numPalette;
    Color* p = texture->palette = ALLOC_N(Color, texture->paletteSize);
    for (int i = 0; i < texture->paletteSize; i++, p++) {
      const png_colorp pngColorP = &(palette[i]);
      p->red   = pngColorP->red;
      p->green = pngColorP->green;
      p->blue  = pngColorP->blue;
      p->alpha = 0xff;
      for (int j = 0; j < numTrans; j++) {
        if (i == trans[j]) {
          p->alpha = 0;
          break;
        }
      }
    }
  }
  const int channels = png_get_channels(pngPtr, infoPtr);
  const Color* srPalette = texture->palette;
  uint8_t* indexes = texture->indexes;
  for (unsigned int j = 0; j < height; j++) {
    png_byte row[width * channels];
    png_read_row(pngPtr, row, NULL);
    for (unsigned int i = 0; i < width; i++, indexes++) {
      Color* c = &(texture->pixels[width * j + i].color);
      switch (channels) {
      case 1:
        *c = srPalette[*indexes = row[i]];
        break;
      case 2:
        c->red = c->green = c->blue = row[i * channels];
        c->alpha = row[i * channels + 1];
        break;
      case 3:
      case 4:
        c->red   = row[i * channels];
        c->green = row[i * channels + 1];
        c->blue  = row[i * channels + 2];
        c->alpha = (channels == 4) ? row[i * channels + 3] : 0xff;
        break;
      }
    }
  }
  png_read_end(pngPtr, endInfo);
  png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
  if (!NIL_P(rbIOToClose)) {
    rb_funcall(rbIOToClose, rb_intern("close"), 0);
  }
  return rbTexture;
}

#elif USE_CAIRO_LOAD_PNG

static VALUE Texture_s_load_png(int argc, VALUE* argv, VALUE self)
{
  VALUE self;
  VALUE rbPath;
  VALUE rbUnused;
  rb_scan_args(argc, "11", argv, &self, &rbPath, &rbUnused);

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

#endif
