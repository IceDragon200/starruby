#
# starruby/Rakefile
#
require 'rubygems'
require 'rubygems/package_task'
require 'rake'
require 'rake/clean'
require 'rdoc/task'
require 'mkrf/rakehelper'

CLEAN.include("ext/starruby/Rakefile", "ext/starruby/mkrf.log", "ext/starruby/*.so",
              "ext/starruby/*.bundle", "lib/*.so", "lib/*.bundle", "ext/starruby/*.o{,bj}",
              "ext/starruby/*.lib", "ext/starruby/*.exp", "ext/starruby/*.pdb",
              "pkg","html")

setup_extension('starruby', 'starruby')

# for gem building
task :extension => :default

task :default => [:starruby]


