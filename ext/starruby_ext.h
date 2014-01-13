#ifndef STARRUBY_H
#define STARRUBY_H

#include "starruby.prv.h"

void Init_starruby(void);
VALUE strb_InitializeAudio(VALUE rb_mSub);
VALUE strb_InitializeBytemap(VALUE rb_mSub);
VALUE strb_InitializeColor(VALUE rb_mSub);
VALUE strb_InitializeFont(VALUE rb_mSub);
VALUE strb_InitializeGame(VALUE rb_mSub);
VALUE strb_InitializeInput(VALUE rb_mSub);
VALUE strb_InitializeMatrix(VALUE rb_mSub);
VALUE strb_InitializePlane(VALUE rb_mSub);
VALUE strb_InitializeRect(VALUE rb_mSub);
VALUE strb_InitializeStarRubyError(VALUE rb_mSub);
VALUE strb_InitializeSymbols(VALUE rb_mSub);
VALUE strb_InitializeTable(VALUE rb_mSub);
VALUE strb_InitializeTexture(VALUE rb_mSub);
VALUE strb_InitializeTextureTool(VALUE rb_mSub);
VALUE strb_InitializeTone(VALUE rb_mSub);
VALUE strb_InitializeTransition(VALUE rb_mSub);
VALUE strb_InitializeVector(VALUE rb_mSub);
VALUE strb_InitializeSprite(VALUE rb_mSub);

#endif
