#ifndef STARRUBY_FONT_H
  #define STARRUBY_FONT_H

  #ifdef HAVE_FONTCONFIG_FONTCONFIG_H
    #include <fontconfig/fontconfig.h>
  #endif
  #ifdef WIN32
    static volatile VALUE rbWindowsFontDirPathSymbol = Qundef;
  #endif

  static VALUE rbFontCache = Qundef;

  static volatile VALUE symbol_bold      = Qundef;
  static volatile VALUE symbol_italic    = Qundef;
  static volatile VALUE symbol_ttc_index = Qundef;

  typedef struct FontFileInfo {
    VALUE rbFontNameSymbol;
    VALUE rbFileNameSymbol;
    int ttcIndex;
    struct FontFileInfo* next;
  } FontFileInfo;
  static FontFileInfo* fontFileInfos;

#endif
