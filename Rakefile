#!/usr/bin/env rake
# starruby/Rakefile
#
require 'rubygems'
require 'rubygems/package_task'
require 'rake'
require 'rake/clean'
require 'rdoc/task'
require 'mkrf/rakehelper'

CLEAN.include("ext/starruby_ext/Rakefile",
              "ext/starruby_ext/mkrf.log", "ext/starruby_ext/*.so",
              "ext/starruby_ext/*.bundle", "lib/*.so", "lib/**/*.so",
              "lib/*.bundle", "ext/starruby_ext/*.o{,bj}",
              "ext/starruby_ext/*.lib", "ext/starruby_ext/*.exp",
              "ext/starruby_ext/*.pdb")

setup_extension('starruby_ext', 'starruby_ext')

task :symbol_table do
  ruby 'ext/starruby_ext/_symbols_builder.rb'
end

# for gem building
task :extension => [:starruby_ext]
task :build => :extension
task :default => [:symbol_table, :extension]