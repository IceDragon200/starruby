#include "starruby.prv.h"
#include "font.h"
#include "font/search-font.inc.c"

volatile VALUE rb_cFont = Qundef;

#define DEFAULT_FONT_SIZE 20

static VALUE Font_s_exist(VALUE self, VALUE rbFilePath)
{
  volatile VALUE rbRealFilePath = Qnil;
  SearchFont(rbFilePath, (VALUE*)&rbRealFilePath, Null);
  return !NIL_P(rbRealFilePath) ? Qtrue : Qfalse;
}

static Void Font_free(Font* font)
{
  if (TTF_WasInit()) {
    TTF_CloseFont(font->sdlFont);
  }
  font->sdlFont = NULL;
  free(font);
}

static VALUE Font_alloc(VALUE klass)
{
  Font* font         = ALLOC(Font);
  font->size         = DEFAULT_FONT_SIZE;
  font->is_bold      = False;
  font->is_italic    = False;
  font->is_underline = False;
  font->sdlFont      = Null;
  return Data_Wrap_Struct(klass, Null, Font_free, font);
}

Void strb_FontRefreshStyle(Font *font)
{
  const int style = TTF_STYLE_NORMAL |
                    (font->is_bold ? TTF_STYLE_BOLD : 0) |
                    (font->is_italic ? TTF_STYLE_ITALIC : 0) |
                    (font->is_underline ? TTF_STYLE_UNDERLINE : 0);
  TTF_SetFontStyle(font->sdlFont, style);
}

/* TODO
     Change args to a hash
 */
static VALUE Font_initialize(Size argc, VALUE* argv, VALUE self)
{
  VALUE rbPath,
        rbRealFilePath,
        rbSize,
        rbOptions,
        val;
  Boolean bold = False,
          italic = False,
          underline = False;
  Integer ttcIndex = -1;
  rb_scan_args(argc, argv, "21", &rbPath, &rbSize, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  }
  SearchFont(rbPath, (VALUE*)&rbRealFilePath, &ttcIndex);

  if (NIL_P(rbRealFilePath)) {
    String tmppath = StringValueCStr(rbPath);
    rb_raise(rb_path2class("Errno::ENOENT"), "%s", tmppath);
    return Qnil;
  }

  Check_Type(rbOptions, T_HASH);
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_bold))) {
    bold = RTEST(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_italic))) {
    italic = RTEST(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_underline))) {
    underline = RTEST(val);
  }
  if (!NIL_P(val = rb_hash_aref(rbOptions, symbol_ttc_index))) {
    ttcIndex = NUM2INT(val);
  }
  if (ttcIndex == -1) {
    ttcIndex = 0;
  }

  const String path   = StringValueCStr(rbRealFilePath);
  const Integer size = NUM2INT(rbSize);
  Font* font;
  Data_Get_Struct(self, Font, font);
  font->size         = size;
  font->is_bold      = bold;
  font->is_italic    = italic;
  font->is_underline = underline;
  font->sdlFont = TTF_OpenFontIndex(path, size, ttcIndex);
  if (!font->sdlFont) {
    rb_raise(rb_eStarRubyError, "%s (%s)", TTF_GetError(), path);
  }

  strb_FontRefreshStyle(font);

  return Qnil;
}

static VALUE Font_bold(VALUE self)
{
  const Font* font;
  Data_Get_Struct(self, Font, font);
  return (TTF_GetFontStyle(font->sdlFont) & TTF_STYLE_BOLD) ? Qtrue : Qfalse;
}

static VALUE Font_bold_set(VALUE self, VALUE rbBool)
{
  Font* font;
  Data_Get_Struct(self, Font, font);
  font->is_bold = RTEST(rbBool);
  strb_FontRefreshStyle(font);
  return Qnil;
}

static VALUE Font_italic(VALUE self)
{
  const Font* font;
  Data_Get_Struct(self, Font, font);
  return (TTF_GetFontStyle(font->sdlFont) & TTF_STYLE_ITALIC) ? Qtrue : Qfalse;
}

static VALUE Font_italic_set(VALUE self, VALUE rbBool)
{
  Font* font;
  Data_Get_Struct(self, Font, font);
  font->is_italic = RTEST(rbBool);
  strb_FontRefreshStyle(font);
  return Qnil;
}

static VALUE Font_underline(VALUE self)
{
  const Font* font;
  Data_Get_Struct(self, Font, font);
  return (TTF_GetFontStyle(font->sdlFont) & TTF_STYLE_UNDERLINE) ? Qtrue : Qfalse;
}

static VALUE Font_underline_set(VALUE self, VALUE rbBool)
{
  Font* font;
  Data_Get_Struct(self, Font, font);
  font->is_underline = RTEST(rbBool);
  strb_FontRefreshStyle(font);
  return Qnil;
}

static VALUE Font_get_size(VALUE self, VALUE rbText)
{
  const Font* font;
  Data_Get_Struct(self, Font, font);
  const String text = StringValueCStr(rbText);
  Integer width, height;
  if (TTF_SizeUTF8(font->sdlFont, text, &width, &height)) {
    rb_raise_sdl_ttf_error();
  }
  volatile VALUE rbSize = rb_assoc_new(INT2NUM(width), INT2NUM(height));
  OBJ_FREEZE(rbSize);
  return rbSize;
}

static VALUE
Font_name(VALUE self)
{
  const Font* font;
  Data_Get_Struct(self, Font, font);
  return rb_str_new2(TTF_FontFaceFamilyName(font->sdlFont));
}

static VALUE
Font_size(VALUE self)
{
  const Font* font;
  Data_Get_Struct(self, Font, font);
  return INT2NUM(font->size);
}

static VALUE
Font_size_set(VALUE self, VALUE rbSize)
{
  Font* font;
  Data_Get_Struct(self, Font, font);
  font->size = NUM2INT(rbSize);
  return Qnil;
}

#define ADD_INFO(currentInfo, _rbFontNameSymbol, \
                 _rbFileNameSymbol, _ttcIndex)   \
  do {                                           \
    FontFileInfo* info = ALLOC(FontFileInfo);    \
    info->rbFontNameSymbol = _rbFontNameSymbol;  \
    info->rbFileNameSymbol = _rbFileNameSymbol;  \
    info->ttcIndex         = _ttcIndex;          \
    info->next             = NULL;               \
    currentInfo->next = info;                    \
    currentInfo = info;                          \
  } while (false)

void
strb_InitializeSdlFont(void)
{
  if (TTF_Init()) {
    rb_raise_sdl_ttf_error();
  }
  fontFileInfos = ALLOC(FontFileInfo);
  fontFileInfos->rbFontNameSymbol = Qundef;
  fontFileInfos->rbFileNameSymbol = Qundef;
  fontFileInfos->ttcIndex         = -1;
  fontFileInfos->next             = NULL;
  FontFileInfo* currentInfo = fontFileInfos;
  (void)currentInfo;

#ifdef WIN32
  HKEY hKey;
  TCHAR* regPath =
    _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
  if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0,
                             KEY_READ, &hKey))) {
    DWORD fontNameBuffMaxLength;
    DWORD fileNameBuffMaxByteLength;
    RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                    &fontNameBuffMaxLength, &fileNameBuffMaxByteLength,
                    NULL, NULL);
    TCHAR fontNameBuff[fontNameBuffMaxLength + 1];
    BYTE fileNameByteBuff[fileNameBuffMaxByteLength];
    for (DWORD dwIndex = 0; ;dwIndex++) {
      ZeroMemory(fontNameBuff, sizeof(fontNameBuff));
      ZeroMemory(fileNameByteBuff, sizeof(fileNameByteBuff));
      DWORD fontNameBuffLength = sizeof(fontNameBuff) / sizeof(TCHAR);
      DWORD fileNameBuffByteLength = fileNameBuffMaxByteLength;
      LONG result = RegEnumValue(hKey, dwIndex,
                                 fontNameBuff, &fontNameBuffLength,
                                 NULL, NULL,
                                 fileNameByteBuff, &fileNameBuffByteLength);
      TCHAR* fileNameBuff = (TCHAR*)fileNameByteBuff;
      DWORD fileNameBuffLength = _tcslen(fileNameBuff);
      if (result == ERROR_SUCCESS) {
        const TCHAR* ext = &(fileNameBuff[fileNameBuffLength - 3]);
        if (tolower(ext[0]) == _T('t') &&
            tolower(ext[1]) == _T('t') &&
            (tolower(ext[2]) == _T('f') ||
             tolower(ext[2]) == _T('c'))) {
          TCHAR* fontName = fontNameBuff;
          const TCHAR* fileName = fileNameBuff;
          // A TTF font name must end with ' (TrueType)'.
          fontName[fontNameBuffLength - 11] = _T('\0');
          for (int i = fileNameBuffLength - 1; 0 <= i; i--) {
            if (fileName[i] == _T('\\')) {
              fileName += i + 1;
              break;
            }
          }
          int length =
            WideCharToMultiByte(CP_UTF8, 0,
                                fontName, -1,
                                NULL, 0,
                                NULL, NULL);
          char fontNameUTF8[length];
          WideCharToMultiByte(CP_UTF8, 0,
                              fontName, -1,
                              fontNameUTF8, length,
                              NULL, NULL);
          volatile VALUE rbFontName = rb_str_new2(fontNameUTF8);
          length =
            WideCharToMultiByte(CP_ACP, 0,
                                fileName, -1,
                                NULL, 0,
                                NULL, NULL);
          char fileNameANSI[length];
          WideCharToMultiByte(CP_ACP, 0,
                              fileName, -1,
                              fileNameANSI, length,
                              NULL, NULL);
          volatile VALUE rbFileName = rb_str_new2(fileNameANSI);
          if (strchr(StringValueCStr(rbFontName), '&')) {
            volatile VALUE rbArr = rb_str_split(rbFontName, "&");
            const int arrLength = RARRAY_LEN(rbArr);
            int ttcIndex = 0;
            for (int i = 0; i < arrLength; i++) {
              volatile VALUE rbFontName = rb_ary_entry(rbArr, i);
              rb_funcall(rbFontName, ID_strip_bang, 0);
              if (0 < RSTRING_LEN(rbFontName)) {
                volatile VALUE rbFontNameSymbol = rb_str_intern(rbFontName);
                volatile VALUE rbFileNameSymbol = rb_str_intern(rbFileName);
                ADD_INFO(currentInfo, rbFontNameSymbol, rbFileNameSymbol,
                         ttcIndex);
                ttcIndex++;
              }
            }
          } else {
            volatile VALUE rbFontNameSymbol = rb_str_intern(rbFontName);
            volatile VALUE rbFileNameSymbol = rb_str_intern(rbFileName);
            ADD_INFO(currentInfo, rbFontNameSymbol, rbFileNameSymbol, -1);
          }
        }
      } else {
        break;
      }
    }
    RegCloseKey(hKey);
  } else {
    rb_raise(rb_eStarRubyError,
             "Win32API error: %d", (int)GetLastError());
  }
  TCHAR szWindowsFontDirPath[MAX_PATH + 1];
  if (FAILED(SHGetFolderPath(NULL, CSIDL_FONTS, NULL,
                             SHGFP_TYPE_CURRENT,
                             szWindowsFontDirPath))) {
    rb_raise(rb_eStarRubyError,
             "Win32API error: %d", (int)GetLastError());
  }
  int length =
    WideCharToMultiByte(CP_UTF8, 0,
                        szWindowsFontDirPath, -1,
                        NULL, 0,
                        NULL, NULL);
  char szWindowsFontDirPathUTF8[length];
  WideCharToMultiByte(CP_UTF8, 0,
                      szWindowsFontDirPath, -1,
                      szWindowsFontDirPathUTF8, length,
                      NULL, NULL);
  volatile VALUE rbWindowsFontDirPath = rb_str_new2(szWindowsFontDirPathUTF8);
  rbWindowsFontDirPathSymbol = rb_str_intern(rbWindowsFontDirPath);
#endif
}

VALUE
strb_InitializeFont(VALUE rb_mStarRuby)
{
  rb_cFont = rb_define_class_under(rb_mStarRuby, "Font", rb_cObject);
  rb_define_singleton_method(rb_cFont, "exist?", Font_s_exist, 1);;
  rb_define_alloc_func(rb_cFont, Font_alloc);
  rb_define_private_method(rb_cFont, "initialize", Font_initialize, -1);
  rb_define_method(rb_cFont, "bold",       Font_bold,          0);
  rb_define_method(rb_cFont, "bold=",      Font_bold_set,      1);
  rb_define_method(rb_cFont, "italic",     Font_italic,        0);
  rb_define_method(rb_cFont, "italic=",    Font_italic_set,    1);
  rb_define_method(rb_cFont, "underline",  Font_underline,     0);
  rb_define_method(rb_cFont, "underline=", Font_underline_set, 1);
  rb_define_method(rb_cFont, "name",       Font_name,          0);
  //rb_define_method(rb_cFont, "name=",     Font_name_set,       1);
  rb_define_method(rb_cFont, "size",       Font_size,          0);
  rb_define_method(rb_cFont, "size=",    Font_size_set,   1);

  rb_define_method(rb_cFont, "get_size",   Font_get_size,      1);
  return rb_cFont;
}
