require 'mkmf'

sdl2 = false
have_library('z')
#have_library('freetype')
if sdl2
  have_library('SDL2')
  have_library('SDL2_mixer')
  have_library('SDL2_image')
  have_library('SDL2_ttf')
else
  have_library('SDL')
  have_library('SDL_mixer')
  have_library('SDL_image')
  have_library('SDL_ttf')
end
have_library('png')
have_library('jpeg')
have_library('GL')
#have_library('cairo')
#have_header("cairo/cairo.h")
if sdl2
  with_config('sdl2-config', 'sdl2-config')
else
  with_config('sdl-config', 'sdl-config')
end

$CFLAGS += " -std=c99 -fPIC "
$CFLAGS += " -mfpmath=sse -msse2 "
$static = true
create_header
create_makefile('starruby_ext')