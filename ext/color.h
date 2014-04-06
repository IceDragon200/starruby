/*

 */
#ifndef STARRUBY_COLOR_H
#define STARRUBY_COLOR_H

#include <stdint.h>
#include <ruby.h>

#define STRB_COLOR_MODE_RGBA 0
#define STRB_COLOR_MODE_BGRA 1
#define STRB_COLOR_MODE_ARGB 2
#define STRB_COLOR_MODE_ABGR 3

//#if SDL_BYTEORDER == SDL_LIL_ENDIAN
////# warning LIL_ENDIAN System
//# define STRB_COLOR_MODE STRB_COLOR_MODE_BGRA
//#else
////# warning BIG_ENDIAN System
//# define STRB_COLOR_MODE STRB_COLOR_MODE_RGBA
//#endif
#define STRB_COLOR_MODE STRB_COLOR_MODE_RGBA

//#define STRB_COLOR_MODE STRB_COLOR_MODE_ABGR

//#define STRB_COLOR_MODE STRB_COLOR_MODE_RGBA

#if STRB_COLOR_MODE == STRB_COLOR_MODE_RGBA
# define STRB_GL_COLOR_MODE GL_RGBA
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_BGRA
# define STRB_GL_COLOR_MODE GL_BGRA
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ARGB
# define STRB_GL_COLOR_MODE GL_ARGB
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ABGR
# define STRB_GL_COLOR_MODE GL_ABGR
#endif

typedef struct {
#if STRB_COLOR_MODE == STRB_COLOR_MODE_RGBA
  uint8_t red, green, blue, alpha;
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_BGRA
  uint8_t blue, green, red, alpha;
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ARGB
  uint8_t alpha, red, green, blue;
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ABGR
  uint8_t alpha, blue, green, red;
#else
  #error Invalid Color Mode
#endif
} Color32;

#define CairoColor32 Color32
#define Color Color32

typedef union {
  Color color;
  uint32_t value;
  uint8_t channels[4];
} Pixel;

void strb_RubyToColor(VALUE rbObj, Color* color);

#endif
