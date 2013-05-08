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
  #include <SDL_mixer.h>
#endif
#include <SDL_ttf.h>
#include <SDL_opengl.h>
#include <assert.h>
#ifdef STRB_CAN_LOAD_SVG
  #include <librsvg/rsvg.h>
#endif

#include "kyameru.h"

/* Ruby Related Headers */
#include <ruby.h>

#ifndef RHASH_IFNONE
# define RHASH_IFNONE(h) (RHASH(h)->ifnone)
#endif

#ifndef RHASH_TBL
# define RHASH_TBL(h) (RHASH(h)->tbl)
#endif

#ifdef HAVE_RUBY_ST_H
# include "ruby/st.h"
#else
# include "st.h"
#endif

/* Windows Related Headers */
#ifdef WIN32
# include <windows.h>
# include <tchar.h>
# include <winreg.h>
# include <shlobj.h>
# ifndef SHGFP_TYPE_CURRENT
#  define SHGFP_TYPE_CURRENT (0)
# endif
#endif

#include "rb_cairo.h"

#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI (3.1415926535897932384626433832795)
# endif
#endif

#include "rb_classes.h"
#include "rb_symbols.h"
#include "starruby-version.h"
#include "starruby-datatypes.h"
#include "starruby-helpers.h"
#include "starruby-accessor.h"
#include "starruby-errors.h"

VALUE strb_GetCompletePath(VALUE, bool);

void strb_GetColorFromRubyValue(Color*, VALUE);

void strb_UpdateInput(void);

void strb_FinalizeAudio(void);
void strb_FinalizeInput(void);

void strb_InitializeSdlAudio(void);
void strb_InitializeSdlFont(void);
void strb_InitializeSdlInput(void);

void strb_GetRealScreenSize(int*, int*);
void strb_GetScreenSize(int*, int*);
int strb_GetWindowScale(void);

void strb_TextureCheckDisposed(const Texture* const);
bool strb_Texture_is_disposed(const Texture* const);

VALUE strb_CheckObjIsKindOf(VALUE rbObj, VALUE rbKind);

Void strb_RubyToColor(VALUE rbObj, Color* color);
Void strb_RubyToRect(VALUE rbObj, Rect* rect);
Void strb_RubyToVector2(VALUE rbObj, Vector2* vec2);
Void strb_RubyToVector3(VALUE rbObj, Vector3* vec3);

#ifdef DEBUG
  #include <assert.h>
  void strb_TestInput(void);
#endif

#ifndef HAVE_RUBY_ENCODING_H
  #define rb_intern_str(str) ID2SYM(rb_intern(StringValueCStr(str)))
#endif

#endif

