/*
  StarRuby Transition
    dc
    dm 24/03/2013
    vr 1.1.0
  */
#include "starruby.prv.h"
#include "rect.h"
#include "texture.h"
#include "transition.h"

/*
  StarRuby Transition [Cross Fade]
    dc 24/03/2013
    dm 24/03/2013
  vr 1.0.0
  */
static VALUE Transition_s_crossfade(VALUE module,
                                  VALUE rbT0, VALUE rbT1, VALUE rbT2,
                                  VALUE rbDelta)
{
  strb_AssertObjIsKindOf(rbT0, rb_cTexture);
  strb_AssertObjIsKindOf(rbT1, rb_cTexture);
  strb_AssertObjIsKindOf(rbT2, rb_cTexture);

  Texture *t0, *t1, *t2;

  Data_Get_Struct(rbT0, Texture, t0);
  Data_Get_Struct(rbT1, Texture, t1);
  Data_Get_Struct(rbT2, Texture, t2);

  strb_TextureCheckDisposed(t0);
  strb_TextureCheckDisposed(t1);
  strb_TextureCheckDisposed(t2);

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


volatile VALUE rb_cTransition = Qundef;

#define COLOR_AVERAGE(color) (color.red + color.blue + color.green) / 3
#define COLOR_P_AVERAGE(color) (color->red + color->blue + color->green) / 3

inline void
strb_CheckDisposedTransition(const Transition *transition)
{
  if (!transition->data) {
    rb_raise(rb_eRuntimeError,
             "can't modify disposed StarRuby::Transition");
  }
}

static VALUE
TextureToTransition(VALUE klass, VALUE rbTexture)
{
  Texture *texture;
  Transition *transition;
  VALUE rbTransition;
  strb_AssertObjIsKindOf(rbTexture, rb_cTexture);
  Data_Get_Struct(rbTexture, Texture, texture);
  strb_TextureCheckDisposed(texture);

  rbTransition = rb_class_new_instance(2, (VALUE[]){
                                       INT2FIX(texture->width),
                                       INT2FIX(texture->height)},
                                       rb_cTransition);

  Data_Get_Struct(rbTransition, Transition, transition);

  const uint32_t length = transition->size;
  uint8_t *data = transition->data;
  Color32 *pixel = (Color32*)texture->pixels;
  for(uint32_t i = 0; i < length; ++i, ++data, ++pixel)
  {
    *data = DIV255(MIN(MAX((pixel->red + pixel->green + pixel->blue) / 3, 0), 255) * pixel->alpha);
  }
  return rbTransition;
}

static VALUE
Transition_transition(self, rbTTexture, rbT1, rbT2, rbt)
  VALUE self, rbTTexture, rbT1, rbT2, rbt;
{
  strb_AssertObjIsKindOf(rbTTexture, rb_cTexture);
  strb_AssertObjIsKindOf(rbT1, rb_cTexture);
  strb_AssertObjIsKindOf(rbT2, rb_cTexture);

  Texture *texture, *src_texture, *trg_texture;
  Transition *transition;

  Data_Get_Struct(rbTTexture, Texture, texture);
  Data_Get_Struct(rbT1, Texture, src_texture);
  Data_Get_Struct(rbT2, Texture, trg_texture);
  Data_Get_Struct(self, Transition, transition);

  strb_TextureCheckDisposed(texture);
  strb_TextureCheckDisposed(src_texture);
  strb_TextureCheckDisposed(trg_texture);
  strb_CheckDisposedTransition(transition);

  if(!strb_Texture_dimensions_match(texture, src_texture) ||
     !strb_Texture_dimensions_match(texture, trg_texture))
  {
    rb_raise(rb_eArgError, "Texture dimensions must all match");
  }

  const uint8_t delta = MIN(MAX(NUM2INT(rbt), 0), 255);
  const uint32_t height = texture->height;
  const uint32_t width = texture->width;
  const uint32_t ypadding = transition->width - width;
  const uint8_t *data = transition->data;

  Pixel *pixels = texture->pixels;
  Pixel *src_pixels = src_texture->pixels;
  Pixel *trg_pixels = trg_texture->pixels;

  for(uint32_t y = 0; y < height; ++y, data += ypadding) {
    for(uint32_t x = 0; x < width; ++x, ++data, ++pixels, ++src_pixels, ++trg_pixels) {
      if(*data < delta)
        pixels->value = trg_pixels->value;
      else
        pixels->value = src_pixels->value;
      //DIV255(*data * delta)
    }
  }
  return Qnil;
}

static VALUE
Transition_to_texture(VALUE self)
{
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  strb_CheckDisposedTransition(transition);

  VALUE rbTexture = rb_class_new_instance(2, (VALUE[]){
                                          INT2FIX(transition->width),
                                          INT2FIX(transition->height)},
                                          rb_cTexture);

  Texture *texture;
  Data_Get_Struct(rbTexture, Texture, texture);

  uint8_t *data = transition->data;
  Pixel *pixels = texture->pixels;

  for(uint32_t i = 0; i < transition->size; ++i, ++data, ++pixels) {
    pixels->color.red   = *data;
    pixels->color.green = *data;
    pixels->color.blue  = *data;
    pixels->color.alpha = 255;
  }
  return rbTexture;
}

VALUE
strb_InitializeTransition(VALUE rb_mStarRuby)
{
  rb_cTransition = rb_define_class_under(rb_mStarRuby,
                                         "Transition", rb_cBytemap);
  rb_define_singleton_method(rb_cTransition, "from_texture",
                             TextureToTransition, 1);
  rb_define_singleton_method(rb_cTransition, "crossfade",
                             Transition_s_crossfade, 4);
  rb_define_method(rb_cTransition, "transition", Transition_transition, 4);
  rb_define_method(rb_cTransition, "to_texture", Transition_to_texture, 0);

  return rb_cTransition;
}
