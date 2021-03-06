#!/usr/bin/env ruby

require "starruby"
include StarRuby

class Sprite
  @@texture = Texture.load("images/star")
  MAX_X = 320 - @@texture.width
  MAX_Y = 240 - @@texture.height

  attr_reader :x
  attr_reader :y

  def initialize
    @x = rand(MAX_X)
    @y = rand(MAX_Y)
    @vx = rand(2) * 2 - 1
    @vy = rand(2) * 2 - 1
  end

  def texture
    @@texture
  end

  def update
    @x += @vx
    @y += @vy
    if @x < 0
      @x = -@x
      @vx = 1
    end
    if @y < 0
      @y = -@y
      @vy = 1
    end
    if MAX_X <= @x
      @x = -(@x - MAX_X) + MAX_X
      @vx = -1
    end
    if MAX_Y <= @y
      @y = -(@y - MAX_Y) + MAX_Y
      @vy = -1
    end
  end
end

sprites = Array.new(200) {Sprite.new}

Game.run(320, 240, :title => "Sprites (Click to speed up!)") do |game|
  break if Input.keys(:keyboard).include?(:escape)
  if Input.keys(:mouse).include?(:left)
    game.frame_rate = 100000
  else
    game.frame_rate = 30
  end
  sprites.each do |sprite|
    sprite.update
  end
  game.screen.clear
  sprites.each do |sprite|
    game.screen.render_texture(sprite.texture, sprite.x, sprite.y)
  end
end
