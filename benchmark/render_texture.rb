require 'benchmark'
require_relative '../lib/starruby'
include StarRuby

src_texture = Texture.new(800, 600)
TextureTool.noise(src_texture, src_texture.rect, 0.5, true, false)
dst_texture = Texture.new(800, 600)
n = 60

color = Color.new(128, 87, 56, 87)
tone = Tone.new(-67, 217, 0, 24)

modes = [:none, :alpha, :add, :subtract, :multiply, :src_mask, :dst_mask, :clear]
Benchmark.bmbm do |x|
  modes.each do |mode|
    x.report("#{mode}") do
      n.times { dst_texture.render_texture(src_texture, 0, 0, blend_type: mode) }
    end
    x.report("#{mode}+color") do
      n.times { dst_texture.render_texture(src_texture, 0, 0, blend_type: mode, color: color) }
    end
    x.report("#{mode}+tone") do
      n.times { dst_texture.render_texture(src_texture, 0, 0, blend_type: mode, tone: tone) }
    end
    x.report("#{mode}+color+tone") do
      n.times { dst_texture.render_texture(src_texture, 0, 0, blend_type: mode, color: color, tone: tone) }
    end
  end
end