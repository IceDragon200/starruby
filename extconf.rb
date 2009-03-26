require "mkmf"

case CONFIG["arch"]
when /mingw32/
  $CFLAGS += " -DWIN32 -DUNICODE -D_UNICODE"
when /mswin32|cygwin|bccwin32|interix|djgpp/
  raise "not supported arch: #{CONFIG["arch"]}"
end

$CFLAGS     += " " + `env libpng-config --cflags`.chomp
$CFLAGS     += " " + `env sdl-config --cflags`.chomp
$LOCAL_LIBS += " " + `env libpng-config --libs`.chomp
$LOCAL_LIBS += " " + `env sdl-config --libs`.chomp

have_header("png.h") or exit(false)
have_header("zlib.h") or exit(false)
have_library("SDL_mixer", "Mix_OpenAudio") or exit(false)
have_library("SDL_ttf", "TTF_Init") or exit(false)

if have_header("fontconfig/fontconfig.h")
  have_library("fontconfig", "FcInit") or exit(false)
end

$CFLAGS += " -finline-functions -Wall -W -Wpointer-arith -Wno-unused-parameter"
$CFLAGS += " -pedantic -std=c99 -funit-at-a-time"
# TODO: use gcc -dumpspecs
if RUBY_PLATFORM !~ /^powerpc/ and CONFIG["arch"] !~ /darwin/
  $CFLAGS += " -mfpmath=sse -msse2"
end
if CONFIG["arch"] !~ /darwin/
  $LDFLAGS += " -Wl,--no-undefined"
end

if arg_config("--debug", false)
  $CFLAGS += " -DDEBUG -O0 -g -pg"
end

create_makefile("starruby", "./src")

if CONFIG["arch"] =~ /mingw32/
  str = open("./Makefile", "rb") do |fp|
    str = fp.read
    drive_name = Dir.pwd[/^(.+:)/].upcase
    str.gsub(/ \/msys\//, " #{drive_name}/msys/")
  end
  str
  open("./Makefile", "wb") do |fp|
    fp.write(str)
  end
end
