require_relative 'starruby.so'

def main(*argsv)
  # Model holds the state of this game
  #model      = FallingBlocks::Model.new(ARGV[0].to_i)
  # Controller changes the state of the model
  #controller = FallingBlocks::Controller.new(model)
  # View renders the screen
  #view       = FallingBlocks::View.new(model)

  puts "Attempting to create texture"
  texture1 = StarRuby::Texture.new(32, 32)

  puts "Attempting to load png into texture"
  texture2 = StarRuby::Texture.load("ruby.png")

  StarRuby::Game.run(320, 240,
                     :title => "Test",
                     :window_scale => 2) do |game|
    # If ESC key is pressed, quit this game
    #break if Input.keys(:keyboard).include?(:escape)

    game.screen.render_texture(texture2, 0, 0)
    #texture2#.render_texture()
    # Update the controller
    #controller.update
    # Update the view
    #view.update(game.screen)
    # Start the gabage collection at each frame
    #GC.start
  end
end

begin
  main(ARGV.dup)
rescue Exception => ex  
  raise ex
end
