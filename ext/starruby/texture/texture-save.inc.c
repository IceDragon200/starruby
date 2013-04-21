static VALUE
Texture_save(VALUE self, VALUE rbPath)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
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
