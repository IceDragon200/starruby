#ifndef STARRUBY_HELPER_H
#define STARRUBY_HELPER_H

#undef ABS
#undef SGN
#undef MAX
#undef MIN
#undef MINMAX

/* abs */
#ifndef ABS
  #define ABS(x) (((x) >= 0) ? (x) : (-(x)))
#endif
/* signum */
#ifndef SGN
  #define SGN(x) (((x) >= 0) ? 1 : -1)
#endif
/* max */
#ifndef MAX
  #define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
/* min */
#ifndef MIN
  #define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
/* minmax */
#ifndef MINMAX
  #define MINMAX(x, y, z) MIN(MAX((x), (z)), (y))
#endif

#define MINMAXU255(x) MINMAX(x, 255, 0)
#define MINMAX255(x) MINMAX(x, 255, -255)
#define CLAMPU255 MINMAXU255
#define CLAMP255 MINMAX255
//#define DIV255(x) ((x) >> 8)
#define DIV255(x) ((x) / 255)
#define FAST_DIV255(x) ((x) >> 8)
#define TONE_DIV255 FAST_DIV255

#ifndef PI
  #ifdef M_PI
    #define PI M_PI
  #else
    #define PI (3.1415926535897932384626433832795)
  #endif
#endif

/* XNA */
//#define ALPHA(src, dst, alpha) (DIV255((src) * ((alpha))) + DIV255((dst) * (0xFF - (alpha))))
/* StarRuby */
//#define ALPHA(src, dst, a) DIV255(((dst) << 8) - (dst) + ((src) - (dst)) * (a))
/* Jet */
#define ALPHA(src, dst, a) (((a * (src - dst)) >> 8) + dst)
#define TONE_ALPHA(src, dst, a) DIV255(((dst) << 8) - (dst) + ((src) - (dst)) * (a))

#ifndef NUMERIC_P
  #define NUMERIC_P(_rbObj_) (TYPE(_rbObj_) == T_FIXNUM ? true : (TYPE(_rbObj_) == T_FLOAT ? true : (TYPE(_rbObj_) == T_BIGNUM ? true : false)))
#endif

/* Ruby conversion helpers */
#ifndef CBOOL2RVAL
  #define CBOOL2RVAL(_x_) ((_x_) ? Qtrue : Qfalse)
#endif

#ifndef RVAL2CBOOL
  #define RVAL2CBOOL(_x_) (RTEST(_x_))
#endif

#ifndef DBL2FIX
  #define DBL2FIX(n) INT2FIX((int32_t)(n))
#endif

/* Texture Helpers */
#define TexturePosInBound(texture_ptr, x, y) ((x) >= 0 && (y) >= 0 && (x) < (texture_ptr)->width && (y) < (texture_ptr)->height)
#define TextureGetPixel(texture_ptr, x, y) ((texture_ptr)->pixels[(x) + (y) * (texture_ptr)->width])
#define TextureGetPixelColor(texture_ptr, x, y) TextureGetPixel(texture_ptr, x, y).color
#define TextureGetPixelValue(texture_ptr, x, y) TextureGetPixel(texture_ptr, x, y).value
#define TextureSetPixel(texture_ptr, x, y, n) (texture_ptr)->pixels[(x) + (y) * (texture_ptr)->width] = (n)
#define TextureSetPixelColor(texture_ptr, x, y, n) TextureGetPixel(texture_ptr, x, y).color = (n)
#define TextureSetPixelValue(texture_ptr, x, y, n) TextureGetPixel(texture_ptr, x, y).value = (n)
#define TexturePixelRGBMatch(px1, px2) ((px1)->color.red == (px2)->color.red && (px1)->color.green == (px2)->color.green && (px1)->color.blue == (px2)->color.blue)
#define TexturePixelRGBAMatch(px1, px2) (TexturePixelRGBMatch(px1, px2) && (px1)->color.alpha == (px2)->color.alpha)

/* Ruby array helpers */
#define hrbArrayLength(_rbArray_) (RARRAY_LEN(_rbArray_))
#define hrbCheckArraySize(_rbArray_, _operator_, _value_) (hrbArrayLength(_rbArray_) _operator_ (_value_))

/*
#define hrbRubyToBoolean(_rbObj_) ((bool)RTEST(_rbObj_))
#define hrbBooleanToRuby(_bool_) ((_bool_) ? Qtrue : Qfalse)
#define hrbArrayEntry(_rbArray_, _index_) (rb_ary_entry((_rbArray_), (_index_)))

#define hrbArrayEntryAsByte(_rbArray_, _index_) ((char)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsSByte(_rbArray_, _index_) ((int8_t)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsUByte(_rbArray_, _index_) ((uint8_t)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsShort(_rbArray_, _index_) ((short)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsSShort(_rbArray_, _index_) ((int16_t)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsUShort(_rbArray_, _index_) ((uint16_t)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsInt(_rbArray_, _index_) ((int)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsSInt(_rbArray_, _index_) ((int32_t)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsUInt(_rbArray_, _index_) ((uint32_t)NUM2INT(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsLong(_rbArray_, _index_) ((long)NUM2LONG(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsSLong(_rbArray_, _index_) ((uint64_t)NUM2LONG(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsULong(_rbArray_, _index_) ((uint64_t)NUM2LONG(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsFloat(_rbArray_, _index_) ((float)NUM2DBL(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsDouble(_rbArray_, _index_) ((double)NUM2DBL(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsBoolean(_rbArray_, _index_) hrbRubyToBoolean(hrbArrayEntry(_rbArray_, _index_))
*/

#endif
