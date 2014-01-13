#ifndef STARRUBY_GAME_H_
#define STARRUBY_GAME_H_

#include "color.h"
#include "tone.h"
#include "texture.h"
#include "vector.h"

typedef struct {
  uint32_t error;
  uint32_t before;
  uint32_t before2;
  int32_t counter;
} GameTimer;

typedef struct {
  bool is_disposed;
  bool isFullscreen;
  bool isWindowClosing;
  bool isVsync;
  int32_t windowScale;
  int32_t frame_rate;
  double fps;
  GameTimer timer;
  VALUE screen;
  //VALUE sprites;
  SDL_Surface* sdlScreen;
  GLuint glScreen;
} Game;

#endif
