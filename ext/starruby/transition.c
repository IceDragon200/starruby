#include "starruby_private.h"
#include "transition.h"
#include "rect.h"

#define COLOR_AVERAGE(color) (color.red + color.blue + color.green) / 3
#define COLOR_P_AVERAGE(color) (color->red + color->blue + color->green) / 3

static VALUE rb_cTransition = Qundef;

static void Transition_free(Transition*);
STRUCT_CHECK_TYPE_FUNC(Transition, Transition);

void Transition_check_disposed(const Transition *transition)
{
  if (!transition->data) {
    rb_raise(rb_eRuntimeError,
             "can't modify disposed StarRuby::Transition");
  }
}

static VALUE
Transition_set(VALUE self, VALUE rbX, VALUE rbY, VALUE rbValue)
{
  Transition *src_transition;
  uint32_t x = NUM2INT(rbX);
  uint32_t y = NUM2INT(rbY);
  uint8_t value = MIN(MAX(NUM2INT(rbValue), 0), 255);

  Data_Get_Struct(self, Transition, src_transition);

  Transition_check_disposed(src_transition);

  src_transition->data[x + (y * src_transition->width)] = value;
  return INT2FIX(value);
}

static VALUE
Transition_get(VALUE self, VALUE rbX, VALUE rbY)
{
  Transition *src_transition;
  uint32_t x = NUM2INT(rbX);
  uint32_t y = NUM2INT(rbY);

  Data_Get_Struct(self, Transition, src_transition);

  Transition_check_disposed(src_transition);

  return INT2FIX(src_transition->data[x + (y * src_transition->width)]);
}

static VALUE
TextureToTransition(VALUE klass, VALUE rbTexture)
{

  return Qnil;
}

//
// Texture target, Rect rect, Texture t1, Vector2I v1, Texture t2, Vector2I v2, int t
static VALUE
Transition_transition(self, rbTTexture, rbRct, rbT1, rbV1, rbT2, rbV2, rbt)
  VALUE self, rbTTexture, rbRct, rbT1, rbV1, rbT2, rbV2, rbt;
{
  return Qnil;
}

static VALUE
Transition_init_copy(VALUE self, VALUE rbTransition)
{
  Transition *src_transition, *trg_transition;

  Data_Get_Struct(self, Transition, trg_transition);
  Data_Get_Struct(rbTransition, Transition, src_transition);

  Transition_check_disposed(src_transition);

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
  return rb_class_new_instance(4, rbArgv, strb_GetRectClass());
}

static void
Transition_free(Transition* transition)
{
  free(transition->data);
  free(transition);
}

static VALUE
Transition_alloc(VALUE klass)
{
  Transition* transition = ALLOC(Transition);
  transition->width = 0;
  transition->height = 0;
  return Data_Wrap_Struct(klass, 0, Transition_free, transition);
}

VALUE
strb_InitializeTransition(VALUE rb_mStarRuby)
{
  rb_cTransition = rb_define_class_under(
    rb_mStarRuby, "Transition", rb_cObject);
  rb_define_alloc_func(rb_cTransition, Transition_alloc);

  rb_define_singleton_method(
    rb_cTransition, "from_texture", TextureToTransition, 1);

  rb_define_method(
    rb_cTransition, "initialize_copy", Transition_init_copy, 1);
  rb_define_method(rb_cTransition, "width", Transition_width, 0);
  rb_define_method(rb_cTransition, "height", Transition_height, 0);
  rb_define_method(rb_cTransition, "[]", Transition_get, 2);
  rb_define_method(rb_cTransition, "[]=", Transition_set, 3);
  rb_define_method(rb_cTransition, "transition", Transition_transition, 7);

  rb_define_method(rb_cTransition, "disposed?", Transition_is_disposed, 0);

  rb_define_method(rb_cTransition, "rect", Transition_rect, 0);

  return rb_cTransition;
}
