require 'rubygems'
require 'mkmf'
require 'mkrf'

Mkrf::Generator.new( 'starruby' ) do |g|
  g.cflags << ' -std=c99'

  # remove error warnings
  g.cflags.gsub!(/-Werror=(\S+)/, '')

  g.include_library("png")
  g.include_header("zlib")

  if g.include_header("fontconfig/fontconfig.h")
    g.include_library("fontconfig", "FcInit")
  end

  sdl_config = with_config('sdl-config', 'sdl-config')
  g.cflags += ' ' + `#{sdl_config} --cflags`.chomp
  g.ldshared += ' ' + `#{sdl_config} --libs`.chomp
  g.include_library('SDL', 'SDL_main')

  if g.include_library("SDL_mixer","Mix_OpenAudio") then
    g.cflags += " -D HAVE_SDL_MIXER "
  end

  if g.include_library("SGE","sge_Line") then
    g.cflags += " -D HAVE_SGE "
  end

  if g.include_library("SDL_image","IMG_Load") then
    g.cflags += " -D HAVE_SDL_IMAGE "
  end

  if g.include_library("SDL_ttf","TTF_Init") then
    g.cflags += " -D HAVE_SDL_TTF "
  end

  g.include_library('GL')

end
