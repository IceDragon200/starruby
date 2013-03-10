#!/usr/bin/env ruby
require_relative 'lib/rbstarruby'

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

p (tb1 = StarRuby::Table.new(1)).to_s
p (tb2 = StarRuby::Table.new(1, 2)).to_s
p (tb3 = StarRuby::Table.new(1, 2, 3)).to_s

texture = StarRuby::Texture.new(32, 32)
texture

__END__
p vector2f0 = StarRuby::Vector2F.new
p vector2f1 = StarRuby::Vector2F.new(vector2f0)
p vector2f2 = StarRuby::Vector2F.new(vector2f1.x, vector2f1.y)

p vector3f0 = StarRuby::Vector3F.new
p vector3f1 = StarRuby::Vector3F.new(vector3f0)
p vector3f2 = StarRuby::Vector3F.new(vector3f1.x, vector3f1.y, vector3f1.z)

vector2f1.x = 14
p vector2f1.x

p rect0 = StarRuby::Rect.new
p rect1 = StarRuby::Rect.new(rect0)
p rect2 = StarRuby::Rect.new(2, 2, 16, 16)

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
  matrix0aa = matrix0.add_at([0, 0, 0], matrix2, [0, 0, 0], [1, 1, 1])
  p matrix0[0, 0, 0]
  p matrix2[0, 0, 0]
  p matrix0aa[0, 0, 0]
end
