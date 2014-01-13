#!/usr/bin/env ruby
#require 'cairo'
require_relative 'lib/starruby'

p (b = StarRuby::Bytemap.new(24, 24, 4))
p (b.to_rect.to_s)
p (b.to_a)
p (b.dup)
p (dump = Marshal.dump(b))
p (b = Marshal.load(dump))
p (b[0, 0])
p (b[24, 0])
p (b[24, 24])
p (c1 = StarRuby::Color.new()).to_s
p (c2 = StarRuby::Color.new(12, 12, 12, 255)).to_s
p (c3 = StarRuby::Color.new(c2)).to_s
p c1 == c3
p c2 == c3
p c1.red = c2.red
p c1.red
p c1.to_s
p dump = Marshal.dump(c1)
p (load = Marshal.load(dump)).to_s

p (t1 = StarRuby::Tone.new()).to_s
p (t2 = StarRuby::Tone.new(12, 24, 32, 14)).to_s
p (t3 = StarRuby::Tone.new(t2)).to_s
p t1 == t3
p t2 == t3
t1.clone

p (r1 = StarRuby::Rect.new()).to_s
p (r2 = StarRuby::Rect.new(0, 0, 32, 32)).to_s
p (r3 = StarRuby::Rect.new(r2)).to_s
p r1 == r3
p r2 == r3
r1.clone

puts ">> Table Test"
p (tb1 = StarRuby::Table.new(1)).to_s
p (tb2 = StarRuby::Table.new(1, 2)).to_s
p (tb3 = StarRuby::Table.new(1, 2, 3)).to_s
p vec = StarRuby::Vector3I.new(0, 1, 1)
p tb3[vec]

### Texture test
puts ">> Texture Test"
p texture = StarRuby::Texture.new(32, 32)
p new_texture = texture.dup
p texture.dispose
p new_texture.dispose

### Transition test
puts ">> Transition Test"
p transition = StarRuby::Transition.new(32, 32)
p new_transition = transition.dup
p transition.dispose
p new_transition.dispose

### Vector test
puts ">> Vector Test"
p vector2f0 = StarRuby::Vector2F.new(1, 2)
p vector2f1 = StarRuby::Vector2F.new(vector2f0)
p vector2f2 = StarRuby::Vector2F.new(vector2f1.x, vector2f1.y)

p vector3f0 = StarRuby::Vector3F.new(1, 2, 3)
p vector3f1 = StarRuby::Vector3F.new(vector3f0)
p vector3f2 = StarRuby::Vector3F.new(vector3f1.x, vector3f1.y, vector3f1.z)

puts "Comparing Vectors"
p vector2f0 > 2
p 2 < vector2f0

p vector3f0 * 3
p 3 * vector3f0
p vector3f0 * vector3f1

p vector3f2.to_vec3i

vector2f1.x = 14
p vector2f1.x

### Rect test
puts ">> Rect Test"
p rect0 = StarRuby::Rect.new
p rect1 = StarRuby::Rect.new(rect0)
p rect2 = StarRuby::Rect.new(2, 2, 16, 16)

### Matirx test
puts ">> Matrix Test"
p matrix0 = StarRuby::MatrixI.new([3, 3, 3], 0)
p matrix1 = StarRuby::MatrixI.new([3, 3, 3], 1)
p matrix2 = StarRuby::MatrixI.new([2, 2, 2], 2)

puts "Negation Test"
p(( matrix2)[0, 0, 0])
p((-matrix2)[0, 0, 0])
p(( matrix2)[0, 0, 0])
p((+matrix2)[0, 0, 0])
p(( matrix2)[0, 0, 0])

coords = [0, 0, 0]
coords2 = [2, 2, 2]
matrix0[*coords] = 3
p matrix0[*coords]
p matrix0[*coords2]

matrix0c = -matrix0
p matrix0c[*coords]
matrix0a = matrix0c + matrix1
p matrix1[*coords]
p matrix0a[*coords]

12.times do
  matrix0aa = matrix0.add_at!([0, 0, 0], matrix2, [0, 0, 0], [1, 1, 1])
  p [matrix0[0, 0, 0], matrix2[0, 0, 0], matrix0aa[0, 0, 0]]
end