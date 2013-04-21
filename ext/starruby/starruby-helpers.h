#ifndef STARRUBY_HELPER_H
#define STARRUBY_HELPER_H

#undef ABS
#undef SGN
#undef MAX
#undef MIN
#undef MINMAX

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MINMAX(x, y, z) MIN(MAX((x), (z)), (y))
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))
#define SGN(x)       (((x) >= 0) ? 1 : -1)

#define MINMAXU255(x) MINMAX(x, 255, 0)
#define MINMAX255(x) MINMAX(x, 255, -255)
#define CLAMPU255 MINMAXU255
#define CLAMP255 MINMAX255
#define DIV255(x) ((x) / 255)

#define ALPHA(src, dst, a) DIV255(((dst) << 8) - (dst) + ((src) - (dst)) * (a))

#ifndef NUMERIC_P
  #define NUMERIC_P(_rbObj_) (TYPE(_rbObj_) == T_FIXNUM ? True : (TYPE(_rbObj_) == T_FLOAT ? True : (TYPE(_rbObj_) == T_BIGNUM ? True : False)))
#endif


#define toLong toBignum
#define toSLong toSBignum
#define toULong toUBignum

#define hrbRubyToBoolean(_rbObj_) toBoolean(RTEST(_rbObj_))
#define hrbBooleanToRuby(_bool_) ((_bool_) ? Qtrue : Qfalse)

#define hrbArrayLength(_rbArray_) (RARRAY_LEN(_rbArray_))
#define hrbCheckArraySize(_rbArray_, _operator_, _value_) (hrbArrayLength(_rbArray_) _operator_ (_value_))

#define hrbArrayEntry(_rbArray_, _index_) (rb_ary_entry((_rbArray_), (_index_)))

#define hrbArrayEntryAsByte(_rbArray_, _index_) toByte(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsSByte(_rbArray_, _index_) toSByte(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsUByte(_rbArray_, _index_) toUByte(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsShort(_rbArray_, _index_) toShort(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsSShort(_rbArray_, _index_) toSShort(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsUShort(_rbArray_, _index_) toUShort(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsInt(_rbArray_, _index_) NUM2INT(hrbArrayEntry(_rbArray_, _index_))
#define hrbArrayEntryAsSInt(_rbArray_, _index_) toSInt(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsUInt(_rbArray_, _index_) toUInt(NUM2INT(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsLong(_rbArray_, _index_) NUM2LONG(hrbArrayEntry(_rbArray_, _index_))
#define hrbArrayEntryAsSLong(_rbArray_, _index_) toSLong(NUM2LONG(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsULong(_rbArray_, _index_) toULong(NUM2LONG(hrbArrayEntry(_rbArray_, _index_)))

#define hrbArrayEntryAsFloat(_rbArray_, _index_) toFloat(NUM2DBL(hrbArrayEntry(_rbArray_, _index_)))
#define hrbArrayEntryAsDouble(_rbArray_, _index_) NUM2DBL(hrbArrayEntry(_rbArray_, _index_))

#define hrbArrayEntryAsBoolean(_rbArray_, _index_) hrbRubyToBoolean(hrbArrayEntry(_rbArray_, _index_))

#endif

/*
static VALUE
1755 rb_ary_length(VALUE ary)
1756 {
1757     long len = RARRAY_LEN(ary);
1758     return LONG2NUM(len);
1759 }
  */
