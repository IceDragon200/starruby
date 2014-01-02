require 'cairo'
if RbConfig::CONFIG['ruby_install_name'] == 'ruby'
  require_relative 'mri/starruby_ext'
elsif RbConfig::CONFIG['ruby_install_name'] == 'rbx'
  require_relative 'rbx/starruby_ext'
else
  raise LoadError, "cannot determine ruby_install_name"
end