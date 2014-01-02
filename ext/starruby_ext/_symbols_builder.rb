#
# starruby/ext/starruby/_symbols_builder.rb
# vr 1.0.0
symbols = [
  :add,
  :alpha,
  :angle,
  :background,
  :basename,
  :blend_type,
  :blur,
  :bold,
  :camera_height,
  :camera_pitch,
  :camera_roll,
  :camera_x,
  :camera_y,
  :camera_yaw,
  :center_x,
  :center_y,
  :clear,
  :color,
  :compact,
  [:compact_bang, :compact!],
  :cursor,
  :destroy,
  :delay,
  :device_number,
  :dispose,
  :divide,
  :down,
  :draw,
  :duration,
  :expand_path,
  :extname,
  :frame_rate,
  :fps,
  :fullscreen,
  :gamepad,
  :glob,
  :hash,
  :height,
  :initialize,
  :inspect,
  :intersection_x,
  :intersection_y,
  :interval,
  :io_length,
  :italic,
  :keyboard,
  :left,
  :length,
  :loop,
  :mask,
  :src_mask,
  :dst_mask,
  :matrix,
  :middle,
  :mouse,
  :multiply,
  :new,
  :none,
  :pack,
  :palette,
  :panning,
  :position,
  :render,
  :pre_render,
  :post_render,
  :right,
  :saturation,
  :scale,
  :scale_vec2,
  :scale_vec3,
  :scale_x,
  :scale_y,
  :size,
  :src_height,
  :src_rect,
  :src_width,
  :src_x,
  :src_y,
  :sub,
  :subtract,
  :time,
  :title,
  :to_a,
  :to_h,
  :to_i,
  :to_f,
  :to_s,
  :tone,
  :ttc_index,
  :unpack,
  :up,
  :view_angle,
  :volume,
  :vsync,
  :width,
  :window_scale,
  :x,
  :y,
  :get_size,
  :underline,
  [:strip_bang, :strip!],
  [:file_question, :file?],
  [:array_get, :[]],
  [:array_set, :[]=],
  [:compare, '<=>'],
].uniq

##
# slim_mode
#   if enabled, all symbols are defined as macros instead of actual internal
#   objects
slim_mode = false
#flag = "volatile"
#flag = "static"
flag = ""

get_symbol_param = ->(symbol) do
  symbol.is_a?(Array) ? symbol : [symbol, symbol]
end

mk_symbol_const  = ->(symbol) do
  var_name, rb_name = get_symbol_param.(symbol)
  if slim_mode
    ["#define ID_#{var_name.to_s} rb_intern(\"#{rb_name.to_s}\")",
     "#define symbol_#{var_name.to_s} ID2SYM(ID_#{var_name.to_s})"]
  else
    ["#{flag} VALUE ID_#{var_name.to_s};",
     "#{flag} VALUE symbol_#{var_name.to_s};"]
   end
end

mk_symbol_undef  = ->(symbol) do
  var_name, rb_name = get_symbol_param.(symbol)
  if slim_mode
    []
  else
    ["#{flag} VALUE ID_#{var_name.to_s} = Qundef;",
     "#{flag} VALUE symbol_#{var_name.to_s} = Qundef;"]
  end
end

mk_symbol_assign = ->(symbol) do
  var_name, rb_name = get_symbol_param.(symbol)
  if slim_mode
    []
  else
    ["ID_#{var_name.to_s} = rb_intern(\"#{rb_name.to_s}\");",
     "symbol_#{var_name.to_s} = ID2SYM(ID_#{var_name.to_s});"]
  end
end

### warning_header
warning = <<__EOF__
/*
 * StarRuby Ruby Symbols
 * WARNING This file was generated, do not modify, else all changes will be lost
 */
__EOF__

### header_contents
file_contents_h = <<__EOF__
#{warning}
#ifndef STARRUBY_SYMBOLS_H_
#define STARRUBY_SYMBOLS_H_

#include <ruby.h>

#{symbols.map(&mk_symbol_const).flatten.sort.join("\n")}

#endif
__EOF__

### main_contents
file_contents_c = <<__EOF__
#{warning}
#include "starruby.prv.h"
#include "rb_symbols.h"

#{symbols.map(&mk_symbol_undef).flatten.sort.join("\n")}

VALUE strb_InitializeSymbols(VALUE rb_mStarRuby)
{
  #{symbols.map(&mk_symbol_assign).flatten.sort.join("\n  ")}
  return Qnil;
}
__EOF__

### output
dir = File.dirname(__FILE__)
File.write(File.join(dir, 'include/rb_symbols.h'), file_contents_h)
File.write(File.join(dir, 'rb_symbols.c'), file_contents_c)
puts "Built Symbol Table. There are a total of #{symbols.size} symbols"
