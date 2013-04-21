#ifndef STARRUBY_GAME_H_
#define STARRUBY_GAME_H_

typedef struct {
  UInteger error;
  UInteger before;
  UInteger before2;
  Integer counter;
} GameTimer;

typedef struct {
  Boolean is_disposed;
  Boolean isFullscreen;
  Boolean isWindowClosing;
  Boolean isVsync;
  Integer windowScale;
  Integer fps;
  Double realFps;
  GameTimer timer;
  VALUE screen;
  SDL_Surface* sdlScreen;
  GLuint glScreen;
} Game;

#endif
