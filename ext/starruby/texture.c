/*
  StarRuby Texture
 */
#include <png.h>

#include "vector.h"
#include "rect.h"
#include "starruby.prv.h"
#include "texture.prv.h"

#include "texture/inline.inc.c"
#include "texture/alloc.inc.c"
#ifdef HAVE_CAIRO
  #include "texture/cairo-bind.inc.c"
#endif
#include "texture/blur.inc.c"
#include "texture/rotate.inc.c"
#include "texture/crop.inc.c"
#include "texture/recolor.inc.c"
#include "texture/bucket_fill.inc.c"
#include "texture/mask.inc.c"
#include "texture/hue.inc.c"
#include "texture/pixel.inc.c"
#include "texture/pixel-blend.inc.c"
#include "texture/clear.inc.c"
#include "texture/query.inc.c"
#include "texture/render.inc.c"
#include "texture/render-texture.inc.c"
#include "texture/render-text.inc.c"
#include "texture/dump.inc.c"

#ifdef STRB_CAN_LOAD_PNG
  #include "texture/load-png.inc.c"
#endif
#ifdef STRB_CAN_LOAD_SVG
  #include "texture/load-svg.inc.c"
#endif
#ifdef STRB_CAN_LOAD_JPG
  #include "texture/load-jpg.inc.c"
#endif
#ifdef STRB_CAN_LOAD_BMP
  #include "texture/load-bmp.inc.c"
#endif

#ifdef STRB_CAN_SAVE_PNG
  #include "texture/save-png.inc.c"
#endif
#ifdef STRB_CAN_SAVE_JPG
  #include "texture/save-jpg.inc.c"
#endif
#ifdef STRB_CAN_SAVE_BMP
  #include "texture/save-bmp.inc.c"
#endif
#ifdef STRB_CAN_SAVE_TGA
  #include "texture/save-tga.inc.c"
#endif

#include "texturetool/texturetool.c"

#include "texture/initialize.inc.c"
#include "texture/copy.inc.c"
#include "texture/perspective.inc.c"
#include "texture/fill.inc.c"
#include "texture/gradient_fill_rect.inc.c"
#include "texture/clip_rect.inc.c"

volatile VALUE rb_cTexture = Qundef;

VALUE strb_InitializeTexture(VALUE rb_mStarRuby)
{
  rb_cTexture = rb_define_class_under(rb_mStarRuby, "Texture", rb_cObject);
#ifdef STRB_TEXTURE_LOAD_PNG
  rb_define_singleton_method(rb_cTexture, "load_png", Texture_s_load_png, -1);
  rb_define_singleton_method(rb_cTexture, "load", Texture_s_load_png, -1);
#endif
#ifdef STRB_TEXTURE_LOAD_BMP
  rb_define_singleton_method(rb_cTexture, "load_bmp", Texture_s_load_bmp, 1);
#endif
#ifdef STRB_TEXTURE_LOAD_JPG
  rb_define_singleton_method(rb_cTexture, "load_jpg", Texture_s_load_jpg, 1);
#endif
#ifdef STRB_TEXTURE_LOAD_SVG
  rb_define_singleton_method(rb_cTexture, "load_svg", Texture_s_load_svg, 3);
#endif

  rb_define_alloc_func(rb_cTexture, Texture_alloc);
  rb_define_private_method(rb_cTexture, "initialize", Texture_initialize, 2);
  rb_define_private_method(rb_cTexture, "initialize_copy",
                           Texture_initialize_copy, 1);
  rb_define_method(rb_cTexture, "[]",
                   Texture_aref, 2);
  rb_define_method(rb_cTexture, "[]=",
                   Texture_aset, 3);
  rb_define_method(rb_cTexture, "blur",
                   Texture_blur, 0);
  rb_define_method(rb_cTexture, "change_hue",
                   Texture_change_hue, 1);
  rb_define_method(rb_cTexture, "change_hue!",
                   Texture_change_hue_bang, 1);

  rb_define_method(rb_cTexture, "clear",
                   Texture_clear, 0);
  rb_define_method(rb_cTexture, "dispose",
                   Texture_dispose, 0);
  rb_define_method(rb_cTexture, "disposed?",
                   Texture_disposed, 0);
  rb_define_method(rb_cTexture, "dump",
                   Texture_dump, 1);
  rb_define_method(rb_cTexture, "fill",
                   Texture_fill, 1);
  rb_define_method(rb_cTexture, "fill_rect",
                   Texture_fill_rect, -1);
  rb_define_method(rb_cTexture, "clear_rect",
                   Texture_clear_rect, -1);
  rb_define_method(rb_cTexture, "gradient_fill_rect",
                   Texture_gradient_fill_rect, -1);
  rb_define_method(rb_cTexture, "height",
                   Texture_height, 0);

  rb_define_method(rb_cTexture, "render_in_perspective",
                   Texture_render_in_perspective, -1);
  rb_define_method(rb_cTexture, "render_line",
                   Texture_render_line, 5);
  rb_define_method(rb_cTexture, "render_pixel",
                   Texture_render_pixel, 3);
  rb_define_method(rb_cTexture, "render_rect",
                   Texture_render_rect, 5);
  rb_define_method(rb_cTexture, "render_text",
                   Texture_render_text, -1);
  rb_define_method(rb_cTexture, "render_texture",
                   Texture_render_texture, -1);
#ifdef STRB_TEXTURE_SAVE_PNG
  rb_define_method(rb_cTexture, "save_png",
                   Texture_save_png, 1);
  rb_define_alias(rb_cTexture, "save", "save_png");
#endif
#ifdef STRB_TEXTURE_SAVE_JPG
  rb_define_method(rb_cTexture, "save_jpg",
                   Texture_save_jpg, 1);
#endif
#ifdef STRB_TEXTURE_SAVE_BMP
  rb_define_method(rb_cTexture, "save_bmp",
                   Texture_save_bmp, 1);
#endif
#ifdef STRB_TEXTURE_SAVE_TGA
  rb_define_method(rb_cTexture, "save_tga",
                   Texture_save_tga, 1);
#endif
  rb_define_method(rb_cTexture, "size",
                   Texture_size, 0);
  rb_define_method(rb_cTexture, "transform_in_perspective",
                   Texture_transform_in_perspective, -1);
  rb_define_method(rb_cTexture, "undump",
                   Texture_undump, 2);
  rb_define_method(rb_cTexture, "width",
                   Texture_width, 0);

  rb_define_method(rb_cTexture, "rect",
                   Texture_rect, 0);

  rb_define_method(rb_cTexture, "rotate",
                   Texture_rotate, 1);

  rb_define_method(rb_cTexture, "crop",
                   Texture_crop, -1);
  rb_define_method(rb_cTexture, "recolor",
                   Texture_recolor, 3);
  rb_define_method(rb_cTexture, "bucket_fill",
                   Texture_bucket_fill, 3);
  rb_define_method(rb_cTexture, "mask",
                   Texture_mask, 6);

  rb_define_method(rb_cTexture, "clip_rect",
                   Texture_clip_rect, 0);
  rb_define_method(rb_cTexture, "clip_rect=",
                   Texture_clip_rect_eq, 1);

#ifdef HAVE_CAIRO
  rb_define_singleton_method(rb_cTexture, "bind_from_cairo",
                             Texture_s_bind_from_cairo, 1);
  rb_define_method(rb_cTexture, "bind_to_cairo", Texture_bind_cairo, 1);
  rb_define_method(rb_cTexture, "unbind", Texture_unbind, 0);
#endif

  rb_define_const(rb_cTexture, "ROTATE_NONE", INT2FIX(ROTATE_NONE));
  rb_define_const(rb_cTexture, "ROTATE_CW",   INT2FIX(ROTATE_CW));
  rb_define_const(rb_cTexture, "ROTATE_CCW",  INT2FIX(ROTATE_CCW));
  rb_define_const(rb_cTexture, "ROTATE_180",  INT2FIX(ROTATE_180));
  rb_define_const(rb_cTexture, "ROTATE_HORZ", INT2FIX(ROTATE_HORZ));
  rb_define_const(rb_cTexture, "ROTATE_VERT", INT2FIX(ROTATE_VERT));

  rb_define_const(rb_cTexture, "MASK_ALPHA",  INT2FIX(MASK_ALPHA));
  rb_define_const(rb_cTexture, "MASK_GRAY",   INT2FIX(MASK_GRAY));
  rb_define_const(rb_cTexture, "MASK_RED",    INT2FIX(MASK_RED));
  rb_define_const(rb_cTexture, "MASK_GREEN",  INT2FIX(MASK_GREEN));
  rb_define_const(rb_cTexture, "MASK_BLUE",   INT2FIX(MASK_BLUE));

  return rb_cTexture;
}
