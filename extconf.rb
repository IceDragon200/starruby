#!/usr/bin/env ruby
# extconf.rb
# Created for use on Arch Linux
require 'mkmf'

have_header("png.h") or exit(false)
have_header("zlib.h") or exit(false)
have_library("SDL_mixer", "Mix_OpenAudio") or exit(false)
have_library("SDL_ttf",   "TTF_Init") or exit(false)
have_library("GL") or exit(false)

if have_header("fontconfig/fontconfig.h")
  have_library("fontconfig", "FcInit") or exit(false)
end

$CFLAGS     += " " + `env libpng-config --cflags`.chomp
$CFLAGS     += " " + `env sdl-config --cflags`.chomp
$LDFLAGS    += " " + `env libpng-config --ldflags`.chomp
$LOCAL_LIBS += " " + `env libpng-config --libs`.chomp
$LOCAL_LIBS += " " + `env sdl-config --libs`.chomp

$CFLAGS += " -finline-functions -funit-at-a-time"
#$CFLAGS += " -Wall -W -Wpointer-arith -Wunused-parameter"
#$CFLAGS.gsub!(/\-W(\w+)/, '')
$CFLAGS += " -std=c99"
$CFLAGS += " -pedantic"

#$CFLAGS += " -v"

create_makefile('starruby', './src')
