#ifndef STARRUBY_ERRORS_H_
#define STARRUBY_ERRORS_H_

static inline VALUE
rb_GetErrno(char* name)
{
  return rb_const_get(rb_const_get(rb_cObject, rb_intern("Errno")),
                      rb_intern(name));
}

#define rb_raise_sdl_error() \
  rb_raise(rb_eStarRubyError, "%s", SDL_GetError())
#define rb_raise_sdl_mix_error() \
  rb_raise(rb_eStarRubyError, "%s", Mix_GetError())
#define rb_raise_sdl_ttf_error() \
  rb_raise(rb_eStarRubyError, "%s", TTF_GetError())
#define rb_raise_opengl_error() \
  rb_raise(rb_eStarRubyError, "OpenGL Error: 0x%x", glGetError());

#endif
