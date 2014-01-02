#include "starruby_ext.h"

volatile VALUE rb_mStarRuby = Qundef;
volatile VALUE rb_eStarRubyError = Qundef;

VALUE
strb_GetStarRubyErrorClass(void)
{
  return rb_eStarRubyError;
}

VALUE
strb_GetCompletePath(VALUE rbPath, bool raiseNotFoundError)
{
  VALUE dir_klass = rb_path2class("Dir");
  const char* path = StringValueCStr(rbPath);
  volatile VALUE rbPathes;
  volatile VALUE rbFileName;

  if (!RTEST(rb_funcall(rb_cFile, ID_file_question, 1, rbPath))) {
    rbPathes   = rb_funcall(dir_klass, ID_glob, 1, rb_str_cat2(rb_str_dup(rbPath), ".*"));
    rbFileName = rb_funcall(rb_cFile, ID_basename, 1, rbPath);
    for (int i = 0; i < rb_funcall(rbPathes, ID_length, 0); i++) {
      volatile VALUE rbFileNameWithoutExt = rb_funcall(rb_cFile, ID_basename, 2, rb_ary_entry(rbPathes, i), rb_str_new2(".*"));
      if (rb_str_cmp(rbFileName, rbFileNameWithoutExt) != 0) {
        rb_ary_store(rbPathes, i, Qnil);
      }
    }
    rb_funcall(rbPathes, ID_compact_bang, 0);
    switch (rb_funcall(rbPathes, ID_length, 0)) {
      case 0:
        if (raiseNotFoundError) {
          rb_raise(rb_path2class("Errno::ENOENT"), "%s", path);
        }
        break;
      case 1:
        return rb_ary_entry(rbPathes, 0);
      default:
        rb_raise(rb_eArgError, "ambiguous path: %s", path);
        break;
    }
    return Qnil;
  } else {
    return rbPath;
  }
}

static void
FinalizeStarRuby(VALUE unused)
{
  TTF_Quit();
#ifdef STRB_USE_AUDIO
  strb_FinalizeAudio();
#endif
  strb_FinalizeInput();
  SDL_Quit();
}

/*
static VALUE StarRuby_finalize_s(VALUE mod)
{
  FinalizeStarRuby(Qnil);
  return Qnil;
}*/

static VALUE
Numeric_to_radian(VALUE self)
{
  return rb_float_new(NUM2DBL(self) * PI / 180.0);
}

static VALUE
Numeric_to_degree(VALUE self)
{
  return rb_float_new(NUM2DBL(self) / PI * 180.0);
}

void
Init_starruby_ext(void)
{
  rb_mStarRuby = rb_define_module("StarRuby");
  rb_eStarRubyError = rb_define_class_under(rb_mStarRuby,
                                            "StarRubyError", rb_eStandardError);
#ifdef STRB_CAN_LOAD_SVG
  g_type_init(); /* for SVG loading */
#endif

  int sdl_options = 0;
  sdl_options |= SDL_INIT_JOYSTICK;

#ifdef STRB_USE_AUDIO
  sdl_options |= SDL_INIT_AUDIO;
#endif

  if (SDL_Init(sdl_options))  {
    rb_raise_sdl_error();
  }

  volatile VALUE rbVersion = rb_str_new2(STRB_VERSION_S);
  OBJ_FREEZE(rbVersion);
  rb_define_const(rb_mStarRuby, "VERSION", rbVersion);
  strb_InitializeSymbols(rb_mStarRuby);

  strb_InitializeSdlFont();
  strb_InitializeSdlInput();

#ifdef STRB_USE_AUDIO
  strb_InitializeSdlAudio();
  rb_define_const(rb_mStarRuby, "HAS_AUDIO", Qtrue);
  strb_InitializeAudio(rb_mStarRuby);
#else
  rb_define_const(rb_mStarRuby, "HAS_AUDIO", Qfalse);
#endif

  strb_InitializeBytemap(rb_mStarRuby);
  strb_InitializeColor(rb_mStarRuby);
  strb_InitializeFont(rb_mStarRuby);
  strb_InitializeGame(rb_mStarRuby);
  strb_InitializeInput(rb_mStarRuby);
  strb_InitializeMatrix(rb_mStarRuby);
  strb_InitializeRect(rb_mStarRuby);
  strb_InitializeTable(rb_mStarRuby);
  strb_InitializeTexture(rb_mStarRuby);
  strb_InitializeTextureTool(rb_mStarRuby);
  strb_InitializeTone(rb_mStarRuby);
  strb_InitializeTransition(rb_mStarRuby);
  strb_InitializeVector(rb_mStarRuby);

  rb_set_end_proc(FinalizeStarRuby, Qnil);

  rb_define_method(rb_cNumeric, "to_radian", Numeric_to_radian, 0);
  rb_define_method(rb_cNumeric, "to_degree", Numeric_to_degree, 0);

  //rb_define_module_function(rb_mStarRuby, "finalize", StarRuby_finalize_s, 0);

#ifdef DEBUG
  strb_TestInput();
#endif
}

