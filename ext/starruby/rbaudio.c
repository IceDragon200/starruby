#include "starruby.prv.h"
#include "audio.h"

#define MAX_CHANNEL_COUNT 16

//static bool isEnabled = false;
//static bool bgmLoop = false;
//static uint8_t bgmVolume = 255;
//static Mix_Music* sdlBgm = NULL;
//static Uint32 sdlBgmStartTicks = 0;
//static Uint32 sdlBgmLastPausedPosition = 0;

static volatile VALUE rbChunkCache = Qundef;
static volatile VALUE rbMusicCache = Qundef;

static volatile VALUE symbol_loop     = Qundef;
static volatile VALUE symbol_panning  = Qundef;
static volatile VALUE symbol_position = Qundef;
static volatile VALUE symbol_time     = Qundef;
static volatile VALUE symbol_volume   = Qundef;

static VALUE
Audio_bgm_position(VALUE self)
{
  return INT2NUM(0);
}

static VALUE
Audio_bgm_volume(VALUE self)
{
  return INT2NUM(0);
}

static VALUE
Audio_bgm_volume_set(VALUE self, VALUE rbVal)
{
  // NUM2INT(rbVal);
  return Qnil;
}

static VALUE
Audio_play_bgm(int argc, VALUE* argv, VALUE self)
{
  return Qnil;
}

static VALUE
Audio_play_se(int argc, VALUE* argv, VALUE self)
{
  return Qnil;
}

static VALUE
Audio_is_playing_bgm(VALUE self)
{
  return Qfalse;
}

static VALUE
Audio_playing_se_count(VALUE self)
{
  return INT2NUM(0);
}

static VALUE
Audio_stop_all_ses(int argc, VALUE* argv, VALUE self)
{
  return Qnil;
}

static VALUE
Audio_stop_bgm(int argc, VALUE* argv, VALUE self)
{
  return Qnil;
}

void strb_InitializeSdlAudio()
{
  // do nothing
}

VALUE
strb_InitializeAudio(VALUE rb_mStarRuby)
{
  VALUE rb_mAudio = rb_define_module_under(rb_mStarRuby, "Audio");
  rb_define_module_function(rb_mAudio, "bgm_position",
                            Audio_bgm_position, 0);
  rb_define_module_function(rb_mAudio, "bgm_volume",
                            Audio_bgm_volume, 0);
  rb_define_module_function(rb_mAudio, "bgm_volume=",
                            Audio_bgm_volume_set, 1);
  rb_define_module_function(rb_mAudio, "play_bgm",
                            Audio_play_bgm, -1);
  rb_define_module_function(rb_mAudio, "play_se",
                            Audio_play_se, -1);
  rb_define_module_function(rb_mAudio, "playing_bgm?",
                            Audio_is_playing_bgm, 0);
  rb_define_module_function(rb_mAudio, "playing_se_count",
                            Audio_playing_se_count, 0);
  rb_define_module_function(rb_mAudio, "stop_all_ses",
                            Audio_stop_all_ses, -1);
  rb_define_module_function(rb_mAudio, "stop_bgm",
                            Audio_stop_bgm, -1);

  rb_define_const(rb_mAudio, "MAX_SE_COUNT", INT2FIX(MAX_CHANNEL_COUNT));

  symbol_loop     = ID2SYM(rb_intern("loop"));
  symbol_panning  = ID2SYM(rb_intern("panning"));
  symbol_position = ID2SYM(rb_intern("position"));
  symbol_time     = ID2SYM(rb_intern("time"));
  symbol_volume   = ID2SYM(rb_intern("volume"));

  Audio_bgm_volume_set(rb_mAudio, INT2FIX(255));

  rbMusicCache = rb_iv_set(rb_mAudio, "music_cache", rb_hash_new());
  rbChunkCache = rb_iv_set(rb_mAudio, "chunk_cache", rb_hash_new());

  return rb_mAudio;
}

void strb_FinalizeAudio()
{

}
