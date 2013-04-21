require 'rubygems'
require 'mkmf'
require '/home/icy/Dropbox/code/Git/ruby-mkrf/lib/mkrf.rb'
#load '/home/icy/Dropbox/code/Git/ruby-mkrf/lib/mkrf/generator.rb'

def get_gem_path(gem_name)
  conf     = { ruby_version: RUBY_VERSION }
  gemspath = "/usr/local/lib/ruby/gems/%<ruby_version>s/gems/" % conf
  dirs     = Dir.glob(File.join(gemspath, gem_name + "*"))
  return dirs.max
end

class Mkrf::Generator

  def pkg_config(name)
    cflags << ' '  + `pkg-config #{name} --cflags`.chomp
    ldshared << ' ' + `pkg-config #{name} --libs`.chomp
  end

end

Mkrf::Generator.new( 'starruby', ["*.c", "*.cpp"] ) do |g|

  use_svg = true

  g.cc   = 'clang'
  g.cppc = 'gcc'

  get_gem_path('cairo')

  g.cflags << ' -std=c99'
  g.pkg_config('cairo')
  g.cflags << ' -I/home/icy/Dropbox/code/Git/Kyameru/src'
  g.cflags << " -I#{get_gem_path('cairo')}/lib"

  if use_svg
    g.cflags += " -D CAN_LOAD_SVG "
    g.pkg_config('glib-2.0')
    g.pkg_config('gdk-pixbuf-2.0')
    g.pkg_config('librsvg-2.0')
  end

  # remove error warnings
  g.cflags.gsub!(/-Werror=(\S+)/, '')

  g.include_library("png")
  #g.include_library("cairo")
  g.include_library("zlib")

  if g.include_header("fontconfig/fontconfig.h")
    g.include_library("fontconfig", "FcInit")
  end

  sdl_config = with_config('sdl-config', 'sdl-config')
  g.cflags << ' ' + `#{sdl_config} --cflags`.chomp
  g.ldshared << ' ' + `#{sdl_config} --libs`.chomp
  g.include_library('SDL', 'SDL_main')

  if g.include_library("SDL_mixer","Mix_OpenAudio") then
    g.cflags << " -D HAVE_SDL_MIXER "
  end

  if g.include_library("SGE", "sge_Line") then
    g.cflags << " -D HAVE_SGE "
  end

  if g.include_library("SDL_image", "IMG_Load") then
    g.cflags << " -D HAVE_SDL_IMAGE "
  end

  if g.include_library("SDL_ttf", "TTF_Init") then
    g.cflags << " -D HAVE_SDL_TTF "
  end

  g.include_library('GL')

end
