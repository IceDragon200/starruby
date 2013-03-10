/*
  StarRuby Table (old RGX base)
  */
#include "starruby.prv.h"

typedef struct {
  uint16_t dim, xsize, ysize, zsize, size;
  int32_t *data;
} RGXTable;

#define GET_TABLE(self, ptr)        \
  RGXTable *ptr; Data_Get_Struct(self, RGXTable, ptr)
#define XYZ_TO_INDEX(x, y, z, xsize, ysize) \
  (x + (y * xsize) + (z * xsize * ysize))

#define STRICT_TABLE false

#if STRICT_TABLE
  #define UNSTRICT_PROTECT
#endif

#if !STRICT_TABLE
  #define UNSTRICT_PROTECT \
    if(x >= xsize || y >= ysize || z >= zsize || index >= size) { \
      return INT2FIX(0);                                          \
    }
#endif

static volatile VALUE rb_cTable = Qundef;

static void
Table_free(RGXTable* table)
{
  free(table->data);
  table->data = NULL;
  free(table);
}

static VALUE
Table_alloc(VALUE klass)
{
  RGXTable* table = ALLOC(RGXTable);
  table->dim         = 0;
  table->xsize       = 1;
  table->ysize       = 1;
  table->zsize       = 1;
  table->size        = 1;
  table->data        = NULL;
  return Data_Wrap_Struct(klass, 0, Table_free, table);
}

static void
tb_check_size(int xsize, int ysize, int zsize)
{
  if (xsize < 0) {
    rb_raise(rb_eArgError, "xsize less than 0");
  }
  if (ysize < 0) {
    rb_raise(rb_eArgError, "ysize less than 0");
  }
  if (zsize < 0) {
    rb_raise(rb_eArgError, "zsize less than 0");
  }
}

static void
tb_check_index(int x, int y, int z, int xsize, int ysize, int zsize)
{
  if (x < 0) {
    rb_raise(rb_eArgError, "x less than 0");
  }
  if (y < 0) {
    rb_raise(rb_eArgError, "y less than 0");
  }
  if (z < 0) {
    rb_raise(rb_eArgError, "z less than 0");
  }
  #if STRICT_TABLE
    if (x >= xsize) {
      rb_raise(rb_eArgError, "x greater than or equal to xsize");
    }
    if (y >= ysize) {
      rb_raise(rb_eArgError, "y greater than or equal to ysize");
    }
    if (z >= zsize) {
      rb_raise(rb_eArgError, "z greater than or equal to zsize");
    }
  #endif
}

#define TABLE_SIZE_ARGV(xsize, ysize, zsize, rbXsize, rbYsize, rbZsize) \
  volatile VALUE rbXsize, rbYsize, rbZsize;   \
  rb_scan_args(argc, argv, "12",              \
               &rbXsize, &rbYsize, &rbZsize); \
  if(argc == 1)                               \
  {                                           \
    rbYsize = INT2FIX(1);                     \
    rbZsize = INT2FIX(1);                     \
  }                                           \
  else if(argc == 2)                          \
  {                                           \
    rbZsize = INT2FIX(1);                     \
  }                                           \
  const int xsize = FIX2INT(rbXsize);         \
  const int ysize = FIX2INT(rbYsize);         \
  const int zsize = FIX2INT(rbZsize);         \
  tb_check_size(xsize, ysize, zsize);


// Get
static VALUE
rb_tb_get(int argc, VALUE* argv, VALUE self)
{
  GET_TABLE(self, source_tb);

  volatile VALUE rbX, rbY, rbZ;
  rb_scan_args(argc, argv, "12",
               &rbX, &rbY, &rbZ);

  if(argc == 1)
  {
    rbY = INT2FIX(0);
    rbZ = INT2FIX(0);
  }
  else if(argc == 2)
  {
    rbZ = INT2FIX(0);
  }
  const int x = FIX2INT(rbX);
  const int y = FIX2INT(rbY);
  const int z = FIX2INT(rbZ);

  const int xsize = source_tb->xsize;
  const int ysize = source_tb->ysize;
  const int zsize = source_tb->zsize;
  const int size = source_tb->size;

  tb_check_index(x, y, z, xsize, ysize, zsize);

  int index = XYZ_TO_INDEX(x, y, z, xsize, ysize);

  UNSTRICT_PROTECT;

  return INT2FIX(source_tb->data[index]);
}

#define TABLE_CLAMP_VALUE(value) (MAX(MIN((value), 0x7FFF), -0x7FFF))

// Set
static VALUE
rb_tb_set(int argc, VALUE* argv, VALUE self)
{
  GET_TABLE(self, source_tb);

  volatile VALUE rbX, rbY, rbZ, rbValue;

  rb_scan_args(argc, argv, "22",
               &rbX, &rbY, &rbZ, &rbValue);

  if(argc == 2)
  {
    rbValue = rbY;
    rbY = INT2FIX(0);
    rbZ = INT2FIX(0);
  }
  else if(argc == 3)
  {
    rbValue = rbZ;
    rbZ = INT2FIX(0);
  }
  const int x = FIX2INT(rbX);
  const int y = FIX2INT(rbY);
  const int z = FIX2INT(rbZ);
  const int value = FIX2INT(rbValue);

  const int xsize = source_tb->xsize;
  const int ysize = source_tb->ysize;
  const int zsize = source_tb->zsize;
  const int size = source_tb->size;

  tb_check_index(x, y, z, xsize, ysize, zsize);

  int index = XYZ_TO_INDEX(x, y, z, xsize, ysize);

  UNSTRICT_PROTECT;

  source_tb->data[index] = TABLE_CLAMP_VALUE(value);

  return INT2FIX(source_tb->data[index]);
}

// Ruby Interface
// Resize
static VALUE
rb_tb_resize(int argc, VALUE* argv, VALUE self)
{
  TABLE_SIZE_ARGV(xsize, ysize, zsize, rbXsize, rbYsize, rbZsize);
  GET_TABLE(self, source_tb);

  const int orgsize = source_tb->size;
  const int size = xsize * ysize * zsize;

  int *orgtable = source_tb->data;
  int *temp = realloc(orgtable, size * sizeof(int));

  if(temp != NULL)
  {
    // table has expanded : Pad with 0s
    if(orgsize < size)
    {
      int i;
      for(i = orgsize; i < size; i++)
      {
        temp[i] = 0;
      }
    }
    // source_tb->dim
    source_tb->xsize = xsize;
    source_tb->ysize = ysize;
    source_tb->zsize = zsize;
    source_tb->size  = size;
    source_tb->data = temp;
  }
  else
  {
    free(orgtable);
    rb_raise(rb_eException, "Table could not be resized");
    return Qnil;
  }
  return self;
}

static VALUE rb_tb_datasize(VALUE self)
{
  GET_TABLE(self, source_tb);
  return INT2FIX(source_tb->size);
}

// Dimension
static VALUE rb_tb_dim(VALUE self)
{
  GET_TABLE(self, source_tb);
  return INT2FIX(source_tb->dim);
}

// nsize
static VALUE rb_tb_xsize(VALUE self)
{
  GET_TABLE(self, source_tb);
  return INT2FIX(source_tb->xsize);
}

static VALUE rb_tb_ysize(VALUE self)
{
  GET_TABLE(self, source_tb);
  return INT2FIX(source_tb->ysize);
}

static VALUE rb_tb_zsize(VALUE self)
{
  GET_TABLE(self, source_tb);
  return INT2FIX(source_tb->zsize);
}

// #initialize
static VALUE
rb_tb_initialize(int argc, VALUE* argv, VALUE self)
{
  TABLE_SIZE_ARGV(xsize, ysize, zsize, rbXsize, rbYsize, rbZsize);
  GET_TABLE(self, table);

  const int size = xsize * ysize * zsize;

  table->dim   = argc;
  table->xsize = xsize;
  table->ysize = ysize;
  table->zsize = zsize;
  table->size  = size;
  table->data  = ALLOC_N(int, size);
  MEMZERO(table->data, int, size);

  return Qnil;
}

static VALUE
rb_tb_initialize_copy(VALUE self, VALUE rbTable)
{
  GET_TABLE(rbTable, src_table);
  GET_TABLE(self, trg_table);

  trg_table->dim = src_table->dim;
  trg_table->xsize = src_table->xsize;
  trg_table->ysize = src_table->ysize;
  trg_table->zsize = src_table->zsize;
  trg_table->size = src_table->size;

  const int size = src_table->size;

  trg_table->data = ALLOC_N(int, size);
  MEMCPY(trg_table->data, src_table->data, int, size);

  return Qnil;
}

static VALUE
rb_tb_to_a(VALUE self)
{
  GET_TABLE(self, source_tb);

  int size = source_tb->size;

  int* tb_data = source_tb->data;

  VALUE ary = rb_ary_new();

  int i;
  for(i = 0; i < size; i++){
    rb_ary_push(ary, INT2FIX(tb_data[i]));
  }

  return ary;
}

static VALUE
rb_tb_clear(VALUE self)
{
  GET_TABLE(self, source_tb);

  source_tb->data = ALLOC_N(int, source_tb->size);

  return self;
}

static VALUE
rb_tb_dump(VALUE self, VALUE depth)
{
  const ID sym_pack = rb_intern("pack");
  // dim, xsize, ysize, zsize, size, data
  GET_TABLE(self, table);

  volatile VALUE header_ary = rb_ary_new();
  rb_ary_push(header_ary, INT2FIX(table->dim));
  rb_ary_push(header_ary, INT2FIX(table->xsize));
  rb_ary_push(header_ary, INT2FIX(table->ysize));
  rb_ary_push(header_ary, INT2FIX(table->zsize));
  rb_ary_push(header_ary, INT2FIX(table->size));

  VALUE header_str = rb_funcall(
    header_ary, sym_pack, 1, rb_str_new2("l5\0"));

  const int size = table->size;
  int* data = table->data;
  volatile VALUE vstr;

  for(int i = 0; i < size; i++)
  {
    int d = data[i];

    VALUE ary = rb_ary_new();
    rb_ary_push(ary, INT2FIX(d));

    vstr = rb_funcall(
      ary, sym_pack, 1, rb_str_new2("s\0"));

    rb_str_concat(header_str, vstr);
  }

  return header_str;
}

static VALUE
rb_tb_load(VALUE klass, VALUE rbStr)
{
  const ID sym_unpack = rb_intern("unpack");
  const ID sym_get = rb_intern("[]");

  VALUE rbTmp, rbTmp2;
  VALUE rbDim, rbNx, rbNy, rbNz, rbSize;

  // dim, nx, ny, nz, nsize = *s[0, 20].unpack('L5')
  rbTmp  = rb_funcall(rbStr, sym_get, 2, INT2NUM(0), INT2NUM(20));
  rbTmp2 = rb_funcall(rbTmp, sym_unpack, 1, rb_str_new2("l5\0"));

  rbDim  = rb_ary_entry(rbTmp2, 0);
  rbNx   = rb_ary_entry(rbTmp2, 1);
  rbNy   = rb_ary_entry(rbTmp2, 2);
  rbNz   = rb_ary_entry(rbTmp2, 3);
  rbSize = rb_ary_entry(rbTmp2, 4);

  const int size = NUM2INT(rbSize);
  VALUE rbData = rb_funcall(
    rbStr, sym_get, 2, INT2NUM(20), INT2NUM(size * 2));
  VALUE rbAry = rb_funcall(rbData, sym_unpack, 1, rb_str_new2("s*\0"));

  int argc = FIX2INT(rbDim);
  VALUE argv[3] = { rbNx, rbNy, rbNz };

  VALUE rbTable = rb_class_new_instance(argc, argv, klass);
  GET_TABLE(rbTable, ctable);
  int* data = ctable->data;

  for(int i = 0; i < size; i++) {
    volatile VALUE rbN = rb_ary_entry(rbAry, i);

    if(rbN == Qnil)
      rbN = INT2NUM(0);

    data[i] = NUM2INT(rbN);
  }

  return rbTable;
}

void strb_InitializeTable(VALUE rb_mStarRuby)
{
  rb_cTable = rb_define_class_under(rb_mStarRuby, "Table", rb_cObject);
  rb_define_method(rb_cTable, "initialize_copy", rb_tb_initialize_copy, 1);

  rb_define_method(rb_cTable, "_dump", rb_tb_dump, 1);
  rb_define_singleton_method(rb_cTable, "_load", rb_tb_load, 1);

  rb_define_alloc_func(rb_cTable, Table_alloc);

  rb_define_private_method(rb_cTable, "initialize", rb_tb_initialize, -1);

  rb_define_method(rb_cTable, "xsize", rb_tb_xsize, 0);
  rb_define_method(rb_cTable, "ysize", rb_tb_ysize, 0);
  rb_define_method(rb_cTable, "zsize", rb_tb_zsize, 0);

  rb_define_method(rb_cTable, "[]", rb_tb_get, -1);
  rb_define_method(rb_cTable, "[]=", rb_tb_set, -1);
  rb_define_method(rb_cTable, "resize", rb_tb_resize, -1);

  // Extended
  rb_define_method(rb_cTable, "clear", rb_tb_clear, 0);
  rb_define_method(rb_cTable, "dimension", rb_tb_dim, 0);
  rb_define_method(rb_cTable, "datasize", rb_tb_datasize, 0);

  rb_define_method(rb_cTable, "to_a", rb_tb_to_a, 0);
}
