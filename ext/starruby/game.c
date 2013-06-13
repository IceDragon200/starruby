#include "starruby.prv.h"
#include "game.h"

#if STRB_COLOR_MODE == STRB_COLOR_MODE_RGBA
#define STRB_GL_COLOR_MODE GL_RGBA
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_BGRA
#define STRB_GL_COLOR_MODE GL_BGRA
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ARGB
#define STRB_GL_COLOR_MODE GL_ARGB
#elif STRB_COLOR_MODE == STRB_COLOR_MODE_ABGR
#define STRB_GL_COLOR_MODE GL_ABGR
#endif

//#define STRB_USE_RENDER_CALLBACKS

volatile VALUE rb_cGame = Qundef;

inline static void
CheckDisposed(const Game* const game)
{
  if (!game) {
    rb_raise(rb_eRuntimeError, "can't modify disposed StarRuby::Game");
  }
}

inline static int
Power2(int x)
{
  int result = 1;
  while (result < x) {
    result <<= 1;
  }
  return result;
}

static VALUE Game_s_current(VALUE);

void
strb_GetRealScreenSize(int* width, int* height)
{
  volatile VALUE rbCurrent = Game_s_current(rb_cGame);
  if (!NIL_P(rbCurrent)) {
    const Game* game;
    Data_Get_Struct(rbCurrent, Game, game);
    *width  = game->sdlScreen->w;
    *height = game->sdlScreen->h;
  } else {
    *width  = 0;
    *height = 0;
  }
}

void
strb_GetScreenSize(int* width, int* height)
{
  volatile VALUE rbCurrent = Game_s_current(rb_cGame);
  if (!NIL_P(rbCurrent)) {
    const Game* game;
    Data_Get_Struct(rbCurrent, Game, game);
    volatile VALUE rbScreen = game->screen;
    const Texture* screen;
    Data_Get_Struct(rbScreen, Texture, screen);
    if (!strb_Texture_is_disposed(screen)) {
      *width  = screen->width;
      *height = screen->height;
    } else {
      *width  = 0;
      *height = 0;
    }
  } else {
    *width  = 0;
    *height = 0;
  }
}

int
strb_GetWindowScale(void)
{
  volatile VALUE rbCurrent = Game_s_current(rb_cGame);
  if (!NIL_P(rbCurrent)) {
    const Game* game;
    Data_Get_Struct(rbCurrent, Game, game);
    return game->windowScale;
  } else {
    return 1;
  }
}

static VALUE Game_dispose(VALUE);
static VALUE Game_frame_rate(VALUE);
static VALUE Game_frame_rate_eq(VALUE, VALUE);
static VALUE Game_screen(VALUE);
static VALUE Game_title(VALUE);
static VALUE Game_title_eq(VALUE, VALUE);
static VALUE Game_fps(VALUE);
static VALUE Game_update_screen(VALUE);
static VALUE Game_update_state(VALUE);
static VALUE Game_wait(VALUE);
static VALUE Game_window_closing(VALUE);

static VALUE
Game_s_current(VALUE self)
{
  return rb_iv_get(self, "current");
}

static VALUE
RunGame(VALUE rbGame)
{
  //const Game* game;
  //Data_Get_Struct(rbGame, Game, game);
  while (true) {
    Game_update_state(rbGame);
    if (RTEST(Game_window_closing(rbGame))) {
      break;
    }
    rb_yield(rbGame);
    Game_update_screen(rbGame);
    Game_wait(rbGame);
  }
  return Qnil;
}

static VALUE
RunGameEnsure(VALUE rbGame)
{
  Game_dispose(rbGame);
  return Qnil;
}

static VALUE
Game_s_run(int argc, VALUE* argv, VALUE self)
{
  volatile VALUE rbGame = rb_class_new_instance(argc, argv, rb_cGame);
  rb_ensure(RunGame, rbGame, RunGameEnsure, rbGame);
  return Qnil;
}

static VALUE
Game_s_ticks(VALUE self)
{
  return INT2NUM(SDL_GetTicks());
}

static void
Game_mark(Game* game)
{
  if (game && !NIL_P(game->screen)) {
    rb_gc_mark(game->screen);
  }
}

static void
Game_free(Game* game)
{
  // should NOT to call SDL_FreeSurface
  if (game) {
    if (game->sdlScreen) {
      //SDL_FreeSurface(game->sdlScreen);
      game->sdlScreen = NULL;
    }
    //if (game->sdlScreenBuffer) {
    //  SDL_FreeSurface(game->sdlScreenBuffer);
    //  game->sdlScreenBuffer = NULL;
    //}
  }
  free(game);
}

static VALUE
Game_alloc(VALUE klass)
{
  // do not call rb_raise in this function
  Game* game = ALLOC(Game);
  game->is_disposed     = false;
  game->windowScale     = 1;
  game->isFullscreen    = false;
  game->screen          = Qnil;
  game->sdlScreen       = NULL;
  //game->sdlScreenBuffer = NULL;
  game->glScreen        = 0;
  game->fps             = 0;
  game->frame_rate      = 60;
  game->timer.error     = 0;
  game->timer.before    = SDL_GetTicks();
  game->timer.before2   = game->timer.before;
  game->timer.counter   = 0;
  game->isWindowClosing = false;
  game->isVsync         = false;
  return Data_Wrap_Struct(klass, Game_mark, Game_free, game);;
}

static void
InitializeScreen(Game* game)
{
  const int32_t bpp = 32;

  VALUE rbScreen = game->screen;
  const Texture* screen;
  Data_Get_Struct(rbScreen, Texture, screen);
  strb_TextureCheckDisposed(screen);
  const int width  = screen->width;
  const int height = screen->height;
  int screenWidth = 0;
  int screenHeight = 0;

  Uint32 options = 0;
  //options |= SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_DOUBLEBUF | SDL_OPENGL;
  options |= SDL_OPENGL;

  if (game->isFullscreen) {
    options |= SDL_HWSURFACE | SDL_FULLSCREEN;
    game->windowScale = 1;
    SDL_Rect** modes = SDL_ListModes(NULL, options);
    if (!modes) {
      rb_raise(rb_eRuntimeError, "not supported fullscreen resolution");
    }
    if (modes != (SDL_Rect**)-1) {
      for (int i = 0; modes[i]; i++) {
        int realBpp = SDL_VideoModeOK(modes[i]->w, modes[i]->h, bpp, options);
        if (width <= modes[i]->w && height <= modes[i]->h && realBpp == bpp) {
          screenWidth  = modes[i]->w;
          screenHeight = modes[i]->h;
        } else {
          break;
        }
      }
      if (screenWidth == 0 || screenHeight == 0) {
        rb_raise(rb_eRuntimeError, "not supported fullscreen resolution");
      }
    } else {
      // any resolution are available
      screenWidth  = width;
      screenHeight = height;
    }
  } else {
    screenWidth  = width  * game->windowScale;
    screenHeight = height * game->windowScale;
    options |= SDL_SWSURFACE;
  }

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   sizeof(uint8_t));
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, sizeof(uint8_t));
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  sizeof(uint8_t));
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, sizeof(uint8_t));
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, game->isVsync ? true : false);

  game->sdlScreen = SDL_SetVideoMode(screenWidth, screenHeight,
                                     bpp, options);
  if (!game->sdlScreen) {
    rb_raise_sdl_error();
  }

  glEnable(GL_BLEND);                // Turn Blending on
  glEnable(GL_TEXTURE_2D);           //
  glDisable(GL_DEPTH_TEST);          // Turn Depth Testing off
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glMatrixMode(GL_PROJECTION);

  //Clearing the projection matrix...
  glLoadIdentity();

  //glOrtho(-1, 1, -1, 1, -1, 1);
  glOrtho(0.0, screenWidth, screenHeight, 0.0, -1.0, 1.0);
  //Now editing the model-view matrix.
  glMatrixMode(GL_MODELVIEW);

  //Clearing the model-view matrix.
  glLoadIdentity();

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glGenTextures(1, &game->glScreen);
  glBindTexture(GL_TEXTURE_2D, game->glScreen);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

static VALUE
Game_initialize(int argc, VALUE* argv, VALUE self)
{
  if (!NIL_P(Game_s_current(rb_cGame))) {
    rb_raise(rb_eStarRubyError, "already run");
  }

  volatile VALUE rbWidth, rbHeight, rbOptions;
  rb_scan_args(argc, argv, "21", &rbWidth, &rbHeight, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  } else {
    Check_Type(rbOptions, T_HASH);
  }
  Game* game;
  Data_Get_Struct(self, Game, game);

  if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
    rb_raise_sdl_error();
  }

  const int width  = NUM2INT(rbWidth);
  const int height = NUM2INT(rbHeight);

  volatile VALUE rbFps = rb_hash_aref(rbOptions, symbol_fps);
  Game_frame_rate_eq(self, !NIL_P(rbFps) ? rbFps : INT2FIX(60));

  volatile VALUE rbTitle = rb_hash_aref(rbOptions, symbol_title);
  Game_title_eq(self, !NIL_P(rbTitle) ? rbTitle : rb_str_new2(""));

  bool cursor = false;

  volatile VALUE val;
  Check_Type(rbOptions, T_HASH);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_cursor))) {
    cursor = RTEST(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_fullscreen))) {
    game->isFullscreen = RTEST(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_window_scale))) {
    game->windowScale = NUM2INT(val);
    if (game->windowScale < 1) {
      rb_raise(rb_eArgError, "invalid window scale: %d",
               game->windowScale);
    }
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_vsync))) {
    game->isVsync = RTEST(val);
  }

  SDL_ShowCursor(cursor ? SDL_ENABLE : SDL_DISABLE);

  volatile VALUE rbScreen =
    rb_class_new_instance(2, (VALUE[]){INT2NUM(width), INT2NUM(height)},
                          rb_cTexture);
  game->screen = rbScreen;

  InitializeScreen(game);

  rb_iv_set(rb_cGame, "current", self);

  return Qnil;
}

static VALUE
Game_dispose(VALUE self)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  if (game->is_disposed) {
    rb_raise(rb_eStarRubyError, "%s has already been disposed",
             rb_obj_classname(self));
  } else {
    volatile VALUE rbScreen = game->screen;
    if (!NIL_P(rbScreen)) {
      rb_funcall(rbScreen, ID_dispose, 0);
      game->screen = Qnil;
    }
    if (game->glScreen) {
      glDeleteTextures(1, &game->glScreen);
      game->glScreen = 0;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    rb_iv_set(rb_cGame, "current", Qnil);
    game->is_disposed = true;
  }
  return Qnil;
}

static VALUE
Game_disposed(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  return game->is_disposed ? Qtrue : Qfalse;
}

static VALUE
Game_frame_rate(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return INT2NUM(game->frame_rate);
}

static VALUE
Game_frame_rate_eq(VALUE self, VALUE rbFrameRate)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  game->frame_rate = NUM2INT(rbFrameRate);
  return Qnil;
}

static VALUE
Game_fullscreen(VALUE self)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return game->isFullscreen ? Qtrue : Qfalse;
}

static VALUE
Game_fullscreen_eq(VALUE self, VALUE rbFullscreen)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  game->isFullscreen = RTEST(rbFullscreen);
  InitializeScreen(game);
  return Qnil;
}

static VALUE
Game_fps(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return rb_float_new(game->fps);
}

static VALUE
Game_screen(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return game->screen;
}

static VALUE
Game_title(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return rb_iv_get(self, "title");
}

static VALUE
Game_title_eq(VALUE self, VALUE rbTitle)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  Check_Type(rbTitle, T_STRING);
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_WM_SetCaption(StringValueCStr(rbTitle), NULL);
  }
  return rb_iv_set(self, "title", rb_str_dup(rbTitle));
}

#ifdef STRB_USE_RENDER_CALLBACKS
static VALUE
Game_pre_render(VALUE self)
{
  return Qnil;
}

static VALUE
Game_post_render(VALUE self)
{
  return Qnil;
}
#endif

static VALUE
Game_update_screen(VALUE self)
{
  const Game* game;
  const Texture* texture;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);

  // Rendering callback function
#ifdef STRB_USE_RENDER_CALLBACKS
  rb_funcall(self, ID_pre_render, 0);
#endif

  Data_Get_Struct(game->screen, Texture, texture);
  strb_TextureCheckDisposed(texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               texture->width, texture->height,
               0, STRB_GL_COLOR_MODE, GL_UNSIGNED_BYTE, texture->pixels);

  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_QUADS);
  {
    glTexCoord2f(0.0, 0.0);
    glVertex3i(0, 0, 0);
    glTexCoord2f(1.0, 0.0);
    glVertex3i(game->sdlScreen->w, 0, 0);
    glTexCoord2f(1.0, 1.0);
    glVertex3i(game->sdlScreen->w, game->sdlScreen->h, 0);
    glTexCoord2f(0.0, 1.0);
    glVertex3i(0, game->sdlScreen->h, 0);
  }
  glEnd();

  SDL_GL_SwapBuffers();

  // Rendering callback function
#ifdef STRB_USE_RENDER_CALLBACKS
  rb_funcall(self, ID_post_render, 0);
#endif

  return Qnil;
}

void strb_GameUpdateEvents(Game* game)
{
  SDL_Event event;
  if ((SDL_PollEvent(&event) && event.type == SDL_QUIT) &&
      !game->isWindowClosing) {
    game->isWindowClosing = true;
  }
}

static VALUE
Game_update_events(VALUE self)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  strb_GameUpdateEvents(game);
  return Qnil;
}

static VALUE
Game_update_state(VALUE self)
{
  Game_update_events(self);
  strb_UpdateInput();
  return Qnil;
}

static VALUE
Game_wait(VALUE self)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  GameTimer* gameTimer = &(game->timer);

  const uint32_t frame_rate = game->frame_rate;

  Uint32 now;
  while (true) {
    now = SDL_GetTicks();
    Uint32 diff = (now - gameTimer->before) * frame_rate + gameTimer->error;
    if (1000 <= diff) {
      gameTimer->error = MIN(diff - 1000, 1000);
      gameTimer->before = now;
      break;
    }
    SDL_Delay(1);
  }

  gameTimer->counter++;
  if (1000 <= now - gameTimer->before2) {
    game->fps = gameTimer->counter * 1000.0 / (now - gameTimer->before2);
    gameTimer->counter = 0;
    gameTimer->before2 = SDL_GetTicks();
  }

  return Qnil;
}

static VALUE
Game_window_closing(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return game->isWindowClosing ? Qtrue : Qfalse;
}

static VALUE
Game_window_scale(VALUE self)
{
  const Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  return INT2FIX(game->windowScale);
}

static VALUE
Game_window_scale_eq(VALUE self, VALUE rbWindowScale)
{
  Game* game;
  Data_Get_Struct(self, Game, game);
  CheckDisposed(game);
  game->windowScale = NUM2INT(rbWindowScale);
  InitializeScreen(game);
  return Qnil;
}

VALUE
strb_InitializeGame(VALUE _rb_mStarRuby)
{
  rb_mStarRuby = _rb_mStarRuby;

  rb_cGame = rb_define_class_under(rb_mStarRuby, "Game", rb_cObject);
  rb_define_singleton_method(rb_cGame, "current",   Game_s_current,   0);
  rb_define_singleton_method(rb_cGame, "run",       Game_s_run,       -1);
  rb_define_singleton_method(rb_cGame, "ticks",     Game_s_ticks,     0);
  rb_define_alloc_func(rb_cGame, Game_alloc);
  rb_define_private_method(rb_cGame, "initialize", Game_initialize, -1);
  rb_define_method(rb_cGame, "dispose",         Game_dispose,         0);
  rb_define_method(rb_cGame, "disposed?",       Game_disposed,        0);
  rb_define_method(rb_cGame, "frame_rate",      Game_frame_rate,      0);
  rb_define_method(rb_cGame, "frame_rate=",     Game_frame_rate_eq,   1);
  rb_define_method(rb_cGame, "fullscreen?",     Game_fullscreen,      0);
  rb_define_method(rb_cGame, "fullscreen=",     Game_fullscreen_eq,   1);
  rb_define_method(rb_cGame, "fps",             Game_fps,             0);
  rb_define_method(rb_cGame, "screen",          Game_screen,          0);
  rb_define_method(rb_cGame, "title",           Game_title,           0);
  rb_define_method(rb_cGame, "title=",          Game_title_eq,        1);
  rb_define_method(rb_cGame, "update_screen",   Game_update_screen,   0);
  rb_define_method(rb_cGame, "update_state",    Game_update_state,    0);
  rb_define_method(rb_cGame, "update_events",   Game_update_events,   0);
  rb_define_method(rb_cGame, "wait",            Game_wait,            0);
  rb_define_method(rb_cGame, "window_closing?", Game_window_closing,  0);
  rb_define_method(rb_cGame, "window_scale",    Game_window_scale,    0);
  rb_define_method(rb_cGame, "window_scale=",   Game_window_scale_eq, 1);

  // Rendering callback functions
#ifdef STRB_USE_RENDER_CALLBACKS
  rb_define_method(rb_cGame, "pre_render",      Game_pre_render,      0);
  rb_define_method(rb_cGame, "post_render",     Game_post_render,     0);
#endif

  return rb_cGame;
}
