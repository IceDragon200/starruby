require 'rubygems'
require 'mkmf'
require '/home/icy/Dropbox/code/Git/ruby-mkrf/lib/mkrf.rb'
#load '/home/icy/Dropbox/code/Git/ruby-mkrf/lib/mkrf/generator.rb'

Mkrf::Generator.new( 'starruby', ["*.c", "*.cpp"] ) do |g|
  g.cc   = 'clang'
  g.cppc = 'gcc'

  g.cflags << ' -std=c99'
  g.cflags << ' -I/usr/include/cairo/'

  # remove error warnings
  g.cflags.gsub!(/-Werror=(\S+)/, '')

  g.include_library("png")
  g.include_library("cairo")
  g.include_library("zlib")
  #g.include_header("zlib")

  if g.include_header("fontconfig/fontconfig.h")
    g.include_library("fontconfig", "FcInit")
  end

  sdl_config = with_config('sdl-config', 'sdl-config')
  g.cflags += ' ' + `#{sdl_config} --cflags`.chomp
  g.ldshared += ' ' + `#{sdl_config} --libs`.chomp
  g.include_library('SDL', 'SDL_main')

  #if g.include_library("SDL_mixer","Mix_OpenAudio") then
  #  g.cflags += " -D HAVE_SDL_MIXER "
  #end

  if g.include_library("SGE", "sge_Line") then
    g.cflags += " -D HAVE_SGE "
  end

  if g.include_library("SDL_image", "IMG_Load") then
    g.cflags += " -D HAVE_SDL_IMAGE "
  end

  if g.include_library("SDL_ttf", "TTF_Init") then
    g.cflags += " -D HAVE_SDL_TTF "
  end

  g.include_library('GL')

end
