#!/usr/bin/env rake
# starruby/Rakefile
#
require 'rubygems'
require 'rubygems/package_task'
require 'rake'
require 'rake/clean'
require 'rdoc/task'
require 'mkrf/rakehelper'

CLEAN.include("ext/starruby/Rakefile",
              "ext/starruby/mkrf.log", "ext/starruby/*.so",
              "ext/starruby/*.bundle", "lib/*.so",
              "lib/*.bundle", "ext/starruby/*.o{,bj}",
              "ext/starruby/*.lib", "ext/starruby/*.exp",
              "ext/starruby/*.pdb",
              "pkg")

setup_extension('starruby', 'starruby')
#setup_extension('cairo', 'cairo')
#setup_extension('chipmunk', 'chipmunk')

# for gem building
task :extension => [:starruby]#, :cairo, :chipmunk]
task :compile => :extension
task :default => [:clean, :extension]


