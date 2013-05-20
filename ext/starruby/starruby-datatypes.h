/*
  StarRuby datatypes
  */
#ifndef STARRUBY_DATATYPE_H
#define STARRUBY_DATATYPE_H

#define STRB_COLOR_MODE_RGBA 0
#define STRB_COLOR_MODE_BGRA 1
#define STRB_COLOR_MODE_ARGB 2
#define STRB_COLOR_MODE_ABGR 3
#define STRB_COLOR_MODE STRB_COLOR_MODE_BGRA

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

#endif
} Color32;

#define CairoColor32 Color32
#define Color Color32

typedef struct {
  uint8_t saturation;
  short red, green, blue;
} Tone;

typedef union {
  Color color;
  uint32_t value;
} Pixel;

// StarRuby Texture
typedef struct {
  Pixel* pixels;
  int32_t width, height;
  VALUE clip_rect;           // Clipping support
  bool binded;            // for use with cairo surfaces
} Texture;

typedef struct {
  int32_t size;
  bool is_bold;
  bool is_italic;
  bool is_underline;
  TTF_Font* sdlFont;
} Font;

// Struct
typedef struct strb_rect
{
  int32_t x, y, width, height;
} Rect;

typedef struct {
  double x, y;
} Vector2;

typedef struct {
  double x, y, z;
} Vector3;

typedef struct {
  int32_t x, y;
} Point2I;

typedef struct {
  int32_t x, y, z;
} Point3I;

#endif
