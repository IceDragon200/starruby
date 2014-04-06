require 'cairo'
if RbConfig::CONFIG['ruby_install_name'] == 'ruby'
  require_relative 'mri/starruby_ext'
elsif RbConfig::CONFIG['ruby_install_name'] == 'rbx'
  require_relative 'rbx/starruby_ext'
else
  raise LoadError, "cannot determine ruby_install_name"
end
class StarRuby::Texture

  alias :get_pixel :[]
  alias :set_pixel :[]=

  def subsample(x, y, w, h)
    texture = StarRuby::Texture.new(w, h)
    texture.render_texture(self, 0, 0,
                           src_x: x, src_y: y, src_width: w, src_height: h)
    return texture
  end

  def mirror
    rotate(ROTATE_HORZ)
  end

end