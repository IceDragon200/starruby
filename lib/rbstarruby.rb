#
# starruby/lib/starruby.rb
#
direc = File.dirname(__FILE__)
%w(starruby chipmunk cairo).each do |fn|
  require File.join(direc, fn)
end
