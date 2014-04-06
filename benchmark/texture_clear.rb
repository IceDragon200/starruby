require 'benchmark'
require_relative '../lib/starruby'
include StarRuby

texture = Texture.new(800, 600)

n = 60 * 60
Benchmark.bmbm do |x|
  x.report("texture.clear") do
    n.times { texture.clear }
  end
end