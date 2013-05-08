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
  UByte red, green, blue, alpha;
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_BGRA
  UByte blue, green, red, alpha;
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ARGB
  UByte alpha, red, green, blue;
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ABGR
  UByte alpha, blue, green, red;
#else

#endif
} Color32;

#define CairoColor32 Color32
#define Color Color32

typedef struct {
  UByte saturation;
  SShort red, green, blue;
} Tone;

typedef union {
  Color color;
  UInteger value;
} Pixel;

// StarRuby Texture
typedef struct {
  Pixel* pixels;
  Integer width, height;
  VALUE clip_rect;           // Clipping support
  Boolean binded;            // for use with cairo surfaces
} Texture;

typedef struct {
  Integer size;
  Boolean is_bold;
  Boolean is_italic;
  Boolean is_underline;
  TTF_Font* sdlFont;
} Font;

// Struct
typedef struct strb_rect
{
  Integer x, y, width, height;
} Rect;

typedef struct {
  Double x, y;
} Vector2;

typedef struct {
  Double x, y, z;
} Vector3;

typedef struct {
  Integer x, y;
} Point2I;

typedef struct {
  Integer x, y, z;
} Point3I;

#endif
