require 'mkmf'

have_library('z')
have_library('SDL')
have_library('freetype')
have_library('SDL_mixer')
have_library('SDL_image')
have_library('SDL_ttf')
have_library('png')
have_library('jpeg')
have_library('GL')
with_config('sdl-config', 'sdl-config')

$CFLAGS += " -std=c99 -fPIC "
$static = true
create_makefile('starruby_ext')