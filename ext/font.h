#ifndef STARRUBY_FONT_H
#define STARRUBY_FONT_H

#ifdef HAVE_FONTCONFIG_FONTCONFIG_H
  #include <fontconfig/fontconfig.h>
#endif

typedef struct FontFileInfo {
  VALUE rbFontNameSymbol;
  VALUE rbFileNameSymbol;
  int ttcIndex;
  struct FontFileInfo* next;
} FontFileInfo;

typedef struct {
  int32_t size;
  bool is_bold;
  bool is_italic;
  bool is_underline;
  TTF_Font* sdlFont;
} Font;

void Font_free(Font*);
inline void
strb_CheckFont(VALUE rbFont)
{
  Check_Type(rbFont, T_DATA);
  if (RDATA(rbFont)->dfree != (RUBY_DATA_FUNC)Font_free) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected StarRuby::Font)",
             rb_obj_classname(rbFont));
  }
}

#endif
