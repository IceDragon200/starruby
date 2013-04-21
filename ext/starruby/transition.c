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
#include "transition/crossfade.inc.c"

volatile VALUE rb_cTransition = Qundef;

#define COLOR_AVERAGE(color) (color.red + color.blue + color.green) / 3
#define COLOR_P_AVERAGE(color) (color->red + color->blue + color->green) / 3

void strb_CheckDisposedTransition(const Transition *transition)
{
  if (!transition->data) {
    rb_raise(rb_eRuntimeError,
             "can't modify disposed StarRuby::Transition");
  }
}

static VALUE
Transition_set(VALUE self, VALUE rbX, VALUE rbY, VALUE rbValue)
{
  rb_check_frozen(self);
  Transition *src_transition;
  Data_Get_Struct(self, Transition, src_transition);
  strb_CheckDisposedTransition(src_transition);
  const int32_t x = NUM2INT(rbX);
  const int32_t y = NUM2INT(rbY);
  const uint8_t value = MIN(MAX(NUM2INT(rbValue), 0), 255);
  const int32_t index = x + (y * src_transition->width);
  const int32_t size = src_transition->width * src_transition->height;

  if(index >= 0 || index < size)
    src_transition->data[x + (y * src_transition->width)] = value;
  return Qnil;
}

static VALUE
Transition_get(VALUE self, VALUE rbX, VALUE rbY)
{
  Transition *src_transition;
  Data_Get_Struct(self, Transition, src_transition);
  strb_CheckDisposedTransition(src_transition);
  const int32_t x = NUM2INT(rbX);
  const int32_t y = NUM2INT(rbY);
  const int32_t index = x + (y * src_transition->width);
  const int32_t size = src_transition->width * src_transition->height;

  if(index >= 0 || index < size)
    return INT2FIX(src_transition->data[index]);
  else
    return INT2FIX(0);
}

static VALUE
TextureToTransition(VALUE klass, VALUE rbTexture)
{
  Texture *texture;
  Transition *transition;
  VALUE rbTransition;
  strb_CheckObjIsKindOf(rbTexture, rb_cTexture);
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
  for(uint32_t i = 0; i < length; i++, data++, pixel++)
  {
    *data = DIV255(MIN(MAX((pixel->red + pixel->green + pixel->blue) / 3, 0), 255) * pixel->alpha);
  }
  return rbTransition;
}

static VALUE
Transition_transition(self, rbTTexture, rbT1, rbT2, rbt)
  VALUE self, rbTTexture, rbT1, rbT2, rbt;
{
  strb_CheckObjIsKindOf(rbTTexture, rb_cTexture);
  strb_CheckObjIsKindOf(rbT1, rb_cTexture);
  strb_CheckObjIsKindOf(rbT2, rb_cTexture);

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

  for(uint32_t y = 0; y < height; y++, data += ypadding) {
    for(uint32_t x = 0; x < width; x++, data++, pixels++, src_pixels++, trg_pixels++) {
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
Transition_init_copy(VALUE self, VALUE rbTransition)
{
  Transition *src_transition, *trg_transition;

  Data_Get_Struct(self, Transition, trg_transition);
  Data_Get_Struct(rbTransition, Transition, src_transition);

  strb_CheckDisposedTransition(src_transition);

  MEMCPY(trg_transition->data, src_transition->data, uint8_t,
         src_transition->width * src_transition->height);

  return Qnil;
}

static VALUE
Transition_width(VALUE self)
{
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  return INT2NUM(transition->width);
}

static VALUE
Transition_height(VALUE self)
{
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  return INT2NUM(transition->height);
}

static VALUE
Transition_is_disposed(VALUE self)
{
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  return !transition->data ? Qtrue : Qfalse;
}

static VALUE
Transition_rect(VALUE self)
{
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  VALUE rbArgv[4] = { INT2NUM(0), INT2NUM(0),
                      INT2NUM(transition->width), INT2NUM(transition->height) };
  return rb_class_new_instance(4, rbArgv, rb_cRect);
}

static void
Transition_free(Transition* transition)
{
  if(transition->data)
    free(transition->data);
  transition->data = NULL;

  free(transition);
}

static VALUE
Transition_alloc(VALUE klass)
{
  Transition* transition = ALLOC(Transition);
  transition->width  = 0;
  transition->height = 0;
  transition->data   = NULL;
  return Data_Wrap_Struct(klass, 0, Transition_free, transition);
}

static VALUE Transition_initialize(VALUE self, VALUE rbWidth, VALUE rbHeight)
{
  const int16_t width = NUM2INT(rbWidth);
  const int16_t height = NUM2INT(rbHeight);

  if (width <= 0)
    rb_raise(rb_eArgError, "width must be greater than 0");
  if (height <= 0)
    rb_raise(rb_eArgError, "height must be greater than 0");

  Transition *transition;
  Data_Get_Struct(self, Transition, transition);

  transition->width = width;
  transition->height = height;
  transition->size = transition->width * transition->height;
  transition->data = ALLOC_N(uint8_t, transition->size);
  MEMZERO(transition->data, uint8_t, transition->size);

  return Qnil;
}

static VALUE
Transition_dispose(VALUE self)
{
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  if(transition->data)
    free(transition->data);
  else
    rb_raise(rb_eStarRubyError, "Cannot dispose already disposed Transition");

  transition->data = NULL;

  return Qnil;
}

static VALUE
Transition_invert_bang(VALUE self)
{
  rb_check_frozen(self);
  Transition *transition;
  Data_Get_Struct(self, Transition, transition);
  strb_CheckDisposedTransition(transition);
  uint8_t *data = transition->data;
  for(uint32_t i = 0; i < transition->size; i++, data++) {
    *data = 255 - *data;
  }
  return self;
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

  for(uint32_t i = 0; i < transition->size; i++, data++, pixels++) {
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
                                         "Transition", rb_cObject);
  rb_define_alloc_func(rb_cTransition, Transition_alloc);
  rb_define_singleton_method(rb_cTransition, "from_texture",
                             TextureToTransition, 1);
  rb_define_singleton_method(rb_cTransition, "crossfade",
                             Transition_crossfade, 4);

  rb_define_method(rb_cTransition, "initialize", Transition_initialize, 2);
  rb_define_method(rb_cTransition, "initialize_copy", Transition_init_copy, 1);
  rb_define_method(rb_cTransition, "width", Transition_width, 0);
  rb_define_method(rb_cTransition, "height", Transition_height, 0);
  rb_define_method(rb_cTransition, "[]", Transition_get, 2);
  rb_define_method(rb_cTransition, "[]=", Transition_set, 3);
  rb_define_method(rb_cTransition, "transition", Transition_transition, 4);
  rb_define_method(rb_cTransition, "invert!", Transition_invert_bang, 0);
  rb_define_method(rb_cTransition, "to_texture", Transition_to_texture, 0);
  rb_define_method(rb_cTransition, "dispose", Transition_dispose, 0);
  rb_define_method(rb_cTransition, "disposed?", Transition_is_disposed, 0);
  rb_define_method(rb_cTransition, "rect", Transition_rect, 0);

  return rb_cTransition;
}
