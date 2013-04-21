/*
  StarRuby datatypes
  */
#ifndef STARRUBY_DATATYPE_H
  #define STARRUBY_DATATYPE_H

  typedef struct {
    UByte blue, green, red;
  } Color24le;

  typedef struct {
    UByte blue, green, red, alpha;
  } Color32le;

  typedef struct
  {
    UByte blue, green, red;
    float alpha;
  } Color32leAf;

  typedef struct {
    UByte red, green, blue;
  } Color24be;

  typedef struct
  {
    UByte alpha, red, green, blue;
  } Color32be;

  /* Alpha Float */
  typedef struct
  {
    Float alpha;
    UByte red, green, blue;
  } Color32beAf;

  #if SDL_BYTEORDER == SDL_LIL_ENDIAN
    #define Color24 Color24le
    #define Color32 Color32le
    #define Color32Af Color32leAf
  #else
    #define Color24 Color24be
    #define Color32 Color32be
    #define Color32Af Color32beAf
  #endif

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
    UShort width, height;
    Boolean binded;            // for use with cairo surfaces
  } Texture;

  typedef struct {
    Integer size;
    Boolean is_bold;
    Boolean is_italic;
    Boolean is_underline;
    TTF_Font* sdlFont;
  } Font;

#endif
