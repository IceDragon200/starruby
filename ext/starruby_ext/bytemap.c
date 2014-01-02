/*
 * StarRuby Bytemap
 */
#include "starruby.prv.h"
#include "bytemap.h"
#include "rect.h"

volatile VALUE rb_cBytemap = Qundef;

static inline void
strb_Bytemap_check_disposed(VALUE rb_vBytemap)
{
  Bytemap* bytemap;
  Data_Get_Struct(rb_vBytemap, Bytemap, bytemap);
  if(!bytemap->data) {
    rb_raise(rb_eStarRubyError,
             "cannot modify disposed %s", rb_obj_classname(rb_vBytemap));
  }
}

/* Stratos Functions
 */
static bool
stratos_Bytemap_is_disposed(Bytemap* bytemap)
{
  return (bytemap->data == NULL ? Qtrue : Qfalse);
}

static uint64_t
stratos_Bytemap_calc_stride(uint32_t width)
{
  return width * sizeof(uint8_t);
}

static bool
stratos_Bytemap_alloc_data(Bytemap* bytemap, uint32_t width, uint32_t height)
{
  bytemap->width  = width;
  bytemap->height = height;
  bytemap->stride = stratos_Bytemap_calc_stride(bytemap->width);
  bytemap->size   = bytemap->width * bytemap->height;
  bytemap->data   = ALLOC_N(uint8_t, bytemap->size);
  return true;
}

/* Ruby Functions
 */
static void
Bytemap_free_data(Bytemap* bytemap)
{
  if(bytemap->data) {
    free(bytemap->data);
    bytemap->data = NULL;
  }
}

static void
Bytemap_free(Bytemap* bytemap)
{
  Bytemap_free_data(bytemap);
  free(bytemap);
}

static VALUE
Bytemap_alloc(VALUE klass)
{
  Bytemap* bytemap = ALLOC(Bytemap);
  bytemap->width  = 0;
  bytemap->height = 0;
  bytemap->stride = 0;
  bytemap->size   = 0;
  bytemap->data   = NULL;
  return Data_Wrap_Struct(klass, NULL, Bytemap_free, bytemap);
}

static VALUE
Bytemap_dispose(VALUE self)
{
  rb_check_frozen(self);
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  if(bytemap->data) {
    Bytemap_free_data(bytemap);
    return Qtrue;
  } else {
    return Qfalse;
  }
}

static VALUE
Bytemap_is_disposed(VALUE self)
{
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  return stratos_Bytemap_is_disposed(bytemap);
}

static VALUE
Bytemap_width(VALUE self)
{
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  return INT2FIX(bytemap->width);
}

static VALUE
Bytemap_height(VALUE self)
{
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  return INT2FIX(bytemap->height);
}

static VALUE
Bytemap_size(VALUE self)
{
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  return INT2FIX(bytemap->size);
}

static VALUE
Bytemap_aref(VALUE self, VALUE rb_vX, VALUE rb_vY)
{
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  int32_t x = NUM2INT(rb_vX);
  int32_t y = NUM2INT(rb_vY);
  if(BYTEMAP_OOR(bytemap, x, y)) {
    return INT2FIX(0);
  }
  return INT2FIX(bytemap->data[x + y * bytemap->stride]);
}

static VALUE
Bytemap_aset(VALUE self, VALUE rb_vX, VALUE rb_vY, VALUE rb_vValue)
{
  rb_check_frozen(self);
  Bytemap* bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  int32_t x = NUM2INT(rb_vX);
  int32_t y = NUM2INT(rb_vY);
  int32_t v = NUM2INT(rb_vValue);
  if(BYTEMAP_OOR(bytemap, x, y)) {
    return Qfalse;
  }
  bytemap->data[x + y * bytemap->stride] = MINMAXU255(v);
  return Qtrue;
}

static VALUE
Bytemap_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE rb_vWidth;
  VALUE rb_vHeight;
  VALUE rb_vDefault;
  int64_t width;
  int64_t height;
  uint8_t vdefault;
  Bytemap* bytemap;
  rb_scan_args(argc, argv, "21", &rb_vWidth, &rb_vHeight, &rb_vDefault);
  width    = NUM2INT(rb_vWidth);
  height   = NUM2INT(rb_vHeight);
  if(NIL_P(rb_vDefault)) {
    vdefault = 0;
  } else {
    int32_t tmpv = NUM2INT(rb_vDefault);
    vdefault = MINMAXU255(tmpv);
  }
  if(0 >= width) {
    rb_raise(rb_eArgError, "width must be greater than 0");
    return Qfalse;
  } else if(0 >= height) {
    rb_raise(rb_eArgError, "height must be greater than 0");
    return Qfalse;
  }
  Data_Get_Struct(self, Bytemap, bytemap);
  stratos_Bytemap_alloc_data(bytemap, width, height);
  for(uint64_t i = 0; i < bytemap->size; i++) {
    bytemap->data[i] = vdefault;
  }
  return Qtrue;
}

static VALUE
Bytemap_initialize_copy(VALUE self, VALUE rb_vSrcBytemap)
{
  Bytemap* dst_bytemap;
  Bytemap* src_bytemap;
  Data_Get_Struct(self, Bytemap, dst_bytemap);
  Data_Get_Struct(rb_vSrcBytemap, Bytemap, src_bytemap);
  stratos_Bytemap_alloc_data(dst_bytemap, src_bytemap->width, src_bytemap->height);
  MEMCPY(dst_bytemap->data, src_bytemap->data, uint8_t, dst_bytemap->size);
  return Qtrue;
}

static VALUE
Bytemap_dump(VALUE self, VALUE rb_vDepth)
{
  Bytemap* bytemap;
  VALUE header_ary;
  VALUE data_ary;
  VALUE dump_ary;
  VALUE header_str;
  VALUE data_str;
  Data_Get_Struct(self, Bytemap, bytemap);
  header_ary = rb_ary_new();
  data_ary   = rb_ary_new();
  dump_ary   = rb_ary_new();
  rb_ary_push(header_ary, INT2FIX(bytemap->width));
  rb_ary_push(header_ary, INT2FIX(bytemap->height));
  rb_ary_push(header_ary, INT2FIX(bytemap->stride));
  rb_ary_push(header_ary, INT2FIX(bytemap->size));
  for(uint32_t i = 0; i < bytemap->size; i++) {
    rb_ary_push(data_ary, INT2FIX(bytemap->data[i]));
  }
  header_str = rb_funcall(header_ary, ID_pack, 1, rb_str_new2("N4\0"));
  data_str = rb_funcall(data_ary, ID_pack, 1, rb_str_new2("C*\0"));
  rb_ary_push(dump_ary, header_str);
  rb_ary_push(dump_ary, data_str);
  return rb_marshal_dump(dump_ary, Qnil);
}

static VALUE
Bytemap_load(VALUE klass, VALUE dump_str)
{
  Bytemap* bytemap;
  uint32_t size;
  VALUE header_ary;
  VALUE data_ary;
  VALUE dump_ary;
  VALUE header_str;
  VALUE data_str;
  VALUE rb_vWidth;
  VALUE rb_vHeight;
  VALUE rb_vStride;
  VALUE rb_vSize;
  VALUE rb_vBytemap;
  dump_ary   = rb_marshal_load(dump_str);
  header_str = rb_ary_entry(dump_ary, 0);
  data_str   = rb_ary_entry(dump_ary, 1);
  header_ary = rb_funcall(header_str, ID_unpack, 1, rb_str_new2("N4\0"));
  data_ary   = rb_funcall(data_str, ID_unpack, 1, rb_str_new2("C*\0"));
  rb_vWidth  = rb_ary_entry(header_ary, 0);
  rb_vHeight = rb_ary_entry(header_ary, 1);
  rb_vStride = rb_ary_entry(header_ary, 2);
  rb_vSize   = rb_ary_entry(header_ary, 3);
  rb_vBytemap = rb_class_new_instance(2, (VALUE[]){ rb_vWidth, rb_vHeight },
                                      klass);
  Data_Get_Struct(rb_vBytemap, Bytemap, bytemap);
  size = NUM2INT(rb_vSize);
  bytemap->width  = NUM2INT(rb_vWidth);
  bytemap->height = NUM2INT(rb_vHeight);
  bytemap->stride = NUM2INT(rb_vStride);
  bytemap->size   = NUM2INT(rb_vSize);
  for(uint32_t i = 0; i < size; i++) {
    bytemap->data[i] = NUM2INT(rb_ary_entry(data_ary, i));
  }
  return rb_vBytemap;
}

/* Conversion Functions */
static VALUE
Bytemap_to_rect(VALUE self)
{
  Bytemap* bytemap;
  VALUE rb_vRect;
  Data_Get_Struct(self, Bytemap, bytemap);
  rb_vRect = rb_class_new_instance(4, (VALUE[]){INT2FIX(0), INT2FIX(0),
                                                INT2FIX(bytemap->width),
                                                INT2FIX(bytemap->height)},
                                   rb_cRect);
  return rb_vRect;
}

static VALUE
Bytemap_to_a(VALUE self)
{
  Bytemap* bytemap;
  VALUE rb_vArray;
  Data_Get_Struct(self, Bytemap, bytemap);
  rb_vArray = rb_ary_new();
  for(uint64_t i = 0; i < bytemap->size; i++) {
    rb_ary_push(rb_vArray, INT2FIX(bytemap->data[i]));
  }
  return rb_vArray;
}

/* Drawing Functions
 */
static VALUE
Bytemap_fill(VALUE self, VALUE rb_vValue)
{
  rb_check_frozen(self);
  Bytemap* bytemap;
  uint8_t value = CLAMPU255(NUM2INT(rb_vValue));
  Data_Get_Struct(self, Bytemap, bytemap);
  for(uint64_t i = 0; i < bytemap->size; i++) {
    bytemap->data[i] = value;
  }
  return self;
}

static VALUE
Bytemap_fill_rect(VALUE self, VALUE rb_vRect, VALUE rb_vValue)
{
  rb_check_frozen(self);
  Bytemap* bytemap;
  Rect dst_rect;
  Rect src_rect;
  uint8_t* data_p;
  uint32_t padding;
  int32_t tmpvalue;
  uint8_t value;
  Data_Get_Struct(self, Bytemap, bytemap);
  src_rect = (Rect){ 0, 0, bytemap->width, bytemap->height };
  strb_RubyToRect(rb_vRect, &dst_rect);
  if(!strb_modify_rect_in_rect(&src_rect, &dst_rect)) {
    return Qnil;
  }
  tmpvalue = NUM2INT(rb_vValue);
  value = CLAMPU255(tmpvalue);
  data_p = &bytemap->data[dst_rect.x + dst_rect.y * bytemap->width];
  padding = bytemap->width - dst_rect.width;
  for(uint32_t j = 0; j < (uint32_t)dst_rect.height; j++, data_p += padding) {
    for(uint32_t i = 0; i < (uint32_t)dst_rect.width; i++, data_p++) {
      *data_p = value;
    }
  }
  return self;
}

static VALUE
Bytemap_invert_bang(VALUE self)
{
  rb_check_frozen(self);
  Bytemap *bytemap;
  Data_Get_Struct(self, Bytemap, bytemap);
  strb_Bytemap_check_disposed(self);
  uint8_t *data_p = bytemap->data;
  for(uint32_t i = 0; i < bytemap->size; i++, data_p++) {
    *data_p = 255 - *data_p;
  }
  return self;
}

static VALUE
Bytemap_invert(VALUE self)
{
  VALUE rb_vBytemap = rb_obj_dup(self);
  Bytemap_invert_bang(rb_vBytemap);
  return rb_vBytemap;
}

VALUE strb_InitializeBytemap(VALUE rb_mSub)
{
  rb_cBytemap = rb_define_class_under(rb_mSub, "Bytemap", rb_cObject);
  rb_define_alloc_func(rb_cBytemap, Bytemap_alloc);
  rb_define_method(rb_cBytemap, "_dump", Bytemap_dump, 1);
  rb_define_singleton_method(rb_cBytemap, "_load", Bytemap_load, 1);
  /* Standard Functions */
  rb_define_private_method(rb_cBytemap, "initialize", Bytemap_initialize, -1);
  rb_define_private_method(rb_cBytemap, "initialize_copy",
                           Bytemap_initialize_copy, 1);
  rb_define_method(rb_cBytemap, "dispose",    Bytemap_dispose, 0);
  rb_define_method(rb_cBytemap, "disposed?",  Bytemap_is_disposed, 0);
  rb_define_method(rb_cBytemap, "width",      Bytemap_width, 0);
  rb_define_method(rb_cBytemap, "height",     Bytemap_height, 0);
  rb_define_method(rb_cBytemap, "size",       Bytemap_size, 0);
  rb_define_method(rb_cBytemap, "[]",         Bytemap_aref, 2);
  rb_define_method(rb_cBytemap, "[]=",        Bytemap_aset, 3);
  /* Conversion Functions */
  rb_define_method(rb_cBytemap, "to_rect",    Bytemap_to_rect, 0);
  rb_define_method(rb_cBytemap, "to_a",       Bytemap_to_a, 0);
  /* Drawing Functions */
  rb_define_method(rb_cBytemap, "fill",       Bytemap_fill, 1);
  rb_define_method(rb_cBytemap, "fill_rect",  Bytemap_fill_rect, -1);
  rb_define_method(rb_cBytemap, "invert!",    Bytemap_invert_bang, 0);
  rb_define_method(rb_cBytemap, "invert",     Bytemap_invert, 0);

  return rb_cBytemap;
}
