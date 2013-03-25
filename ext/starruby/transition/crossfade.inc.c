/*
  StarRuby Transition [Cross Fade]
    dc 24/03/2013
    dm 24/03/2013
  vr 1.0.0
  */
static VALUE Transition_crossfade(VALUE module,
                                  VALUE rbT0, VALUE rbT1, VALUE rbT2,
                                  VALUE rbDelta)
{
  strb_CheckTexture(rbT0);
  strb_CheckTexture(rbT1);
  strb_CheckTexture(rbT2);

  Texture *t0, *t1, *t2;

  Data_Get_Struct(rbT0, Texture, t0);
  Data_Get_Struct(rbT1, Texture, t1);
  Data_Get_Struct(rbT2, Texture, t2);

  strb_CheckDisposedTexture(t0);
  strb_CheckDisposedTexture(t1);
  strb_CheckDisposedTexture(t2);

  if(!strb_Texture_dimensions_match(t0, t1) ||
     !strb_Texture_dimensions_match(t0, t2))
  {
    rb_raise(rb_eArgError, "Texture dimensions must all match");
  }

  const uint8_t delta   = CLAMPU255(NUM2INT(rbDelta));
  const uint32_t height = t0->height;
  const uint32_t width  = t0->width;

  Pixel *dst = t0->pixels;
  Pixel *pixels1 = t1->pixels;
  Pixel *pixels2 = t2->pixels;

  for(uint32_t y = 0; y < height; y++) {
    for(uint32_t x = 0; x < width; x++, dst++, pixels1++, pixels2++) {
      const uint8_t alpha = pixels1->color.alpha; //DIV255(pixels1->color.alpha * (255 - delta));
      const uint8_t beta  = DIV255(pixels2->color.alpha * delta);
      dst->color.alpha = MAX(alpha, beta);
      dst->color.red   = ALPHA(pixels2->color.red,   pixels1->color.red,   beta);
      dst->color.green = ALPHA(pixels2->color.green, pixels1->color.green, beta);
      dst->color.blue  = ALPHA(pixels2->color.blue,  pixels1->color.blue,  beta);
    }
  }
  return Qnil;
}
