#!/usr/bin/env ruby
require_relative 'lib/rbstarruby'

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
