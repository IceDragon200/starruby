#!/usr/bin/env ruby
fn = ARGV[0]
`gcc -I/usr/local/include/ruby-2.0.0 -I/usr/local/include/ruby-2.0.0/x86_64-linux -I/usr/local/lib/ruby/2.0.0/x86_64-linux -I/usr/local/lib/ruby/site_ruby/2.0.0 -I. -L/usr/local/lib -L/usr/lib -lSDL -I/usr/include/SDL #{fn}.c -E -o #{fn}.E.c`

