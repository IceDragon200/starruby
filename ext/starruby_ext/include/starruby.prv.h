#ifndef STARRUBY_PRIVATE_H
#define STARRUBY_PRIVATE_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#ifdef HAVE_SDL_MIXER
#  include <SDL_mixer.h>
#endif
#include <SDL_ttf.h>
#include <SDL_opengl.h>
#include <assert.h>
#ifdef STRB_CAN_LOAD_SVG
#  include <librsvg/rsvg.h>
#endif

/* Ruby Related Headers */
#include <ruby.h>
#ifdef HAVE_CAIRO
#  include <cairo.h>
#  include "rb_cairo.h"
#endif

#ifndef RHASH_IFNONE
#  define RHASH_IFNONE(h) (RHASH(h)->ifnone)
#endif

#ifndef RHASH_TBL
#  define RHASH_TBL(h) (RHASH(h)->tbl)
#endif

#ifdef HAVE_RUBY_ST_H
#  include "ruby/st.h"
#else
#  include "st.h"
#endif

/* Windows Related Headers */
#ifdef WIN32
#  include <windows.h>
#  include <tchar.h>
#  include <winreg.h>
#  include <shlobj.h>
#  ifndef SHGFP_TYPE_CURRENT
#    define SHGFP_TYPE_CURRENT (0)
#  endif
#endif

//#include "rb_cairo.h"

#include "rb_classes.h"
#include "rb_symbols.h"
#include "starruby-version.h"
#include "starruby-datatypes.h"
#include "starruby-helpers.h"
#include "starruby-accessor.h"
#include "starruby-errors.h"

/* Forward Declarations */

VALUE strb_GetStarRubyErrorClass();

VALUE strb_GetCompletePath(VALUE, bool);

void strb_UpdateInput(void);

void strb_FinalizeAudio(void);
void strb_FinalizeInput(void);

void strb_InitializeSdlAudio(void);
void strb_InitializeSdlFont(void);
void strb_InitializeSdlInput(void);

void strb_GetRealScreenSize(int*, int*);
void strb_GetScreenSize(int*, int*);
int strb_GetWindowScale(void);

/*inline int
min(int a, int b)
{
  return a < b ? a : b;
}

inline int
max(int a, int b)
{
  return a > b ? a : b;
}*/

inline VALUE
strb_AssertObjIsKindOf(VALUE rbObj, VALUE rbKind)
{
  if (!rb_obj_is_kind_of(rbObj, rbKind)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)",
             rb_obj_classname(rbObj), rb_class2name(rbKind));
    return Qfalse; /* if it every gets to this point */
  }
  return Qtrue;
}

#ifdef DEBUG
  #include <assert.h>
  void strb_TestInput(void);
#endif

#ifndef HAVE_RUBY_ENCODING_H
  #define rb_intern_str(str) ID2SYM(rb_intern(StringValueCStr(str)))
#endif

#endif

