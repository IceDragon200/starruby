/*
  StarRuby datatypes
  */
#ifndef STARRUBY_DATATYPE_H
  #define STARRUBY_DATATYPE_H

  typedef struct {
    uint8_t blue, green, red;
  } Color24le;

  typedef struct {
    uint8_t blue, green, red, alpha;
  } Color32le;

  typedef struct
  {
    uint8_t blue, green, red;
    float alpha;
  } Color32leAf;

  typedef struct {
    uint8_t red, green, blue;
  } Color24be;

  typedef struct
  {
    uint8_t alpha, red, green, blue;
  } Color32be;

  /* Alpha Float */
  typedef struct
  {
    float alpha;
    uint8_t red, green, blue;
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
    uint8_t saturation;
    int16_t red, green, blue;
  } Tone;

  typedef union {
    Color color;
    uint32_t value;
  } Pixel;

  // StarRuby Texture
  typedef struct {
    uint16_t width, height;
    bool binded; // for use with cairo surfaces
    Pixel* pixels;
  } Texture;

  typedef struct {
    int32_t size;
    TTF_Font* sdlFont;
  } Font;

#endif
