#!/usr/bin/env rake
# starruby/Rakefile
#
require 'fileutils'
require 'rubygems'
require 'rubygems/package_task'
require 'rake'
require 'rake/clean'
require 'rdoc/task'
require 'mkrf/rakehelper'

CLEAN.include("ext/Makefile",
              "ext/mkmf.log", "ext/*.so",
              "ext/*.bundle", "lib/*.so", "lib/**/*.so",
              "lib/*.bundle", "ext/*.o{,bj}",
              "ext/*.lib", "ext/*.exp",
              "ext/*.pdb")

task :symbol_table do
  ruby 'ext/_symbols_builder.rb'
end

task :starruby_ext do
  Dir.chdir("ext") do
    ruby("extconf.rb") && sh("make")
  end
  FileUtils.mkdir_p("lib/mri")
  FileUtils.cp("ext/starruby_ext.so", "lib/mri/starruby_ext.so")
end

# for gem building
task :extension => [:starruby_ext]
task :build => :extension
task :default => [:symbol_table, :extension]