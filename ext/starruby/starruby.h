#ifndef STARRUBY_H
  #define STARRUBY_H

  void Init_starruby(void);

  VALUE strb_InitializeAudio(VALUE rb_mStarRuby);
  VALUE strb_InitializeColor(VALUE rb_mStarRuby);
  VALUE strb_InitializeContext(VALUE rb_mStarRuby);
  VALUE strb_InitializeFont(VALUE rb_mStarRuby);
  VALUE strb_InitializeGame(VALUE rb_mStarRuby);
  VALUE strb_InitializeInput(VALUE rb_mStarRuby);
  VALUE strb_InitializeMatrix(VALUE rb_mStarRuby);
  VALUE strb_InitializeRect(VALUE rb_mStarRuby);
  VALUE strb_InitializeStarRubyError(VALUE rb_mStarRuby);
  VALUE strb_InitializeTexture(VALUE rb_mStarRuby);
  VALUE strb_InitializeTextureTool(VALUE rb_mStarRuby);
  VALUE strb_InitializeTransition(VALUE rb_mStarRuby);
  VALUE strb_InitializeVector(VALUE rb_mStarRuby);

#endif
