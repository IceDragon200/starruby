static void SearchFont(VALUE rbFilePathOrName,
                       VALUE* volatile rbRealFilePath, int* ttcIndex)
{
  *rbRealFilePath = Qnil;
  if (ttcIndex != NULL) {
    *ttcIndex = -1;
  }
  *rbRealFilePath = strb_GetCompletePath(rbFilePathOrName, false);
  if (!NIL_P(*rbRealFilePath)) {
    return;
  }
  volatile VALUE rbFontNameSymbol = ID2SYM(rb_intern(rb_str_intern(rbFilePathOrName)));
  FontFileInfo* info = fontFileInfos;
  while (info) {
    if (info->rbFontNameSymbol == rbFontNameSymbol) {
      *rbRealFilePath = rb_str_new2(rb_id2name(SYM2ID(info->rbFileNameSymbol)));
#ifdef WIN32
      volatile VALUE rbTemp =
        rb_str_new2(rb_id2name(SYM2ID(rbWindowsFontDirPathSymbol)));
      *rbRealFilePath = rb_str_concat(rb_str_cat2(rbTemp, "\\"), *rbRealFilePath);
#endif
      if (ttcIndex != NULL) {
        *ttcIndex = info->ttcIndex;
      }
      return;
    }
    info = info->next;
  }
#ifdef HAVE_FONTCONFIG_FONTCONFIG_H
  if (!FcInit()) {
    FcFini();
    rb_raise(rb_eStarRubyError, "can't initialize fontconfig library");
    return;
  }
  int nameLength = RSTRING_LEN(rbFilePathOrName) + 1;
  char name[nameLength];
  strncpy(name, StringValueCStr(rbFilePathOrName), nameLength);
  char* delimiter = strchr(name, ',');
  char* style = NULL;
  if (delimiter) {
    *delimiter = '\0';
    style = delimiter + 1;
    char* nameTail = delimiter - 1;
    while (*nameTail == ' ') {
      *nameTail = '\0';
      nameTail--;
    }
    while (*style == ' ') {
      style++;
    }
  }
  FcPattern* pattern = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, name, NULL);
  if (style && 0 < strlen(style)) {
    FcPatternAddString(pattern, FC_STYLE, (FcChar8*)style);
  }
  FcObjectSet* objectSet = FcObjectSetBuild(FC_FAMILY, FC_FILE, NULL);
  FcFontSet* fontSet = FcFontList(NULL, pattern, objectSet);
  if (objectSet) {
    FcObjectSetDestroy(objectSet);
  }
  if (pattern) {
    FcPatternDestroy(pattern);
  }
  if (fontSet) {
    for (int i = 0; i < fontSet->nfont; i++) {
      FcChar8* fileName = NULL;
      if (FcPatternGetString(fontSet->fonts[i], FC_FILE, 0, &fileName) ==
          FcResultMatch) {
        FcChar8* fontName = FcNameUnparse(fontSet->fonts[i]);
        *rbRealFilePath = rb_str_new2((char*)fileName);
        volatile VALUE rbFontName = rb_str_new2((char*)fontName);
        free(fontName);
        fontName = NULL;
        if (ttcIndex != NULL && strchr(StringValueCStr(rbFontName), ',')) {
          *ttcIndex = 0;
        }
      }
    }
    FcFontSetDestroy(fontSet);
  }
  FcFini();
  if (!NIL_P(*rbRealFilePath)) {
    return;
  }
#endif
  return;
}
