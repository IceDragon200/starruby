require 'rubygems'
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
  ## TODO
  # 1. Allow usage of strict SDL, strict OpenGL or mixed
  # 2. Restore libpng support
  # 3. Add JPEG loading / saving
  # 4. Add BMP loading / saving
  # 5. Add TGA loading / saving
  # 6. Add custom Texture loading (Texture#from_data(width, height, byte_format, byte_size, data))
  # 7. Fix SVG loading to support custom width/height

  #g.cflags.gsub!('-fno-fast-math', '-ffast-math')
  #g.cflags.gsub!('-O3', '-Ofast')

  # CONFIG
  use_svg       = false
  use_sdl_mixer = true
  use_openal    = false
  use_opengl    = true
  use_cairo     = false # PNG loading support
  use_libpng    = true  # alternative PNG loading also saving support
  use_libjpeg   = false
  # TODO
  use_internal_libbmp = false

  # Uses clang for fast compilation otherwise gcc is utilized
  fast_compile  = false

  # end config

  g.cc   = fast_compile ? 'clang' : 'gcc'
  g.cxxc = fast_compile ? 'clang' : 'gcc'

  # Kyameru Datatype support
  g.cflags << ' -I/home/icy/Dropbox/code/Git/Kyameru/src'

  # Can we use any form of audio?
  raise("Cannot use SDL_mixer 'and' OpenAL at the same time") if use_sdl_mixer && use_openal
  use_audio = use_openal || use_sdl_mixer

  # Using the c99 standard
  g.cflags << ' -std=c99'

  # Enable Audio Support
  if use_audio
    g.add_define('STRB_USE_AUDIO')
    if use_openal
      if g.include_library('AL')
        g.add_define('HAVE_OPENAL')
      end
    elsif use_sdl_mixer
      if g.include_library("SDL_mixer", "Mix_OpenAudio")
        g.add_define('HAVE_SDL_MIXER')
      end
    end
  end

  ## SDL Configuration
  #
  #sdl_config = with_config('sdl-config', 'sdl-config')
  #g.cflags << ' ' + `#{sdl_config} --cflags`.chomp
  #g.ldshared << ' ' + `#{sdl_config} --libs`.chomp
  g.pkg_config('sdl')
  #g.include_library('SDL', 'SDL_main')

  # Do we have SDL Image Loading support
  #   And why haven't I used this? :(
  if g.include_library("SDL_image", "IMG_Load")
    g.add_define('HAVE_SDL_IMAGE')
  end

  if g.include_library("SGE", "sge_Line") then
    g.add_define('HAVE_SGE')
  end

  if g.include_library("SDL_ttf", "TTF_Init") then
    g.add_define('HAVE_SDL_TTF')
  end

  # Use cairo // Alternative PNG loading support
  if use_cairo && g.include_library('cairo')
    g.pkg_config('cairo')
    g.add_define('HAVE_CAIRO')
    g.add_define('STRB_CAN_LOAD_PNG')
    g.cflags << " -I#{get_gem_path('cairo')}/lib"
  end

  # LIBPNG
  if use_libpng && g.include_library('png')
    g.add_define('HAVE_LIBPNG')
    g.add_define('STRB_CAN_LOAD_PNG')
    g.add_define('STRB_CAN_SAVE_PNG')
  end

  # LIBJPEG
  if use_libjpeg && g.include_library('jpeg')
    g.add_define('HAVE_LIBJPEG')
    g.add_define('STRB_CAN_LOAD_JPEG')
    g.add_define('STRB_CAN_SAVE_JPEG')
  end

  # LIBBMP
  if use_internal_libbmp
    g.add_define('USE_INTERNAL_LIBBMP')
    g.add_define('STRB_CAN_LOAD_BMP')
    g.add_define('STRB_CAN_SAVE_BMP')
  end

  if use_opengl && g.include_library('GL')
    g.add_define('HAVE_OPENGL')
  end

  if use_svg
    g.add_define('STRB_CAN_LOAD_SVG')
    g.pkg_config('glib-2.0')
    g.pkg_config('gdk-pixbuf-2.0')
    g.pkg_config('librsvg-2.0')
  end

  # remove error warnings
  g.cflags.gsub!(/-Werror=(\S+)/, '')

  # zlib compression, un-compression

  if g.pkg_config('zlib')#g.include_library("zlib")
    g.add_define('HAVE_ZLIB')
  end

  if g.include_header("fontconfig/fontconfig.h")
    g.include_library("fontconfig", "FcInit")
    g.add_define('HAVE_FONTCONFIG')
  end

end
