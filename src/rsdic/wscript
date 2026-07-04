#!/usr/bin/python

VERSION = '0.0.1'
APPNAME = 'rsdic'

top = '.'

subdirs = 'lib tool'

def options(opt):
  opt.tool_options('compiler_cxx')
  opt.tool_options('unittest_gtest')
  opt.recurse(subdirs)

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('unittest_gtest')
  conf.env.CXXFLAGS += ['-O2', '-Wall', '-W', '-g']
  conf.recurse(subdirs)

def build(bld):
  bld(source = 'rsdic.pc.in',
      prefix = bld.env['PREFIX'],
      exec_prefix = '${prefix}',
      libdir = '${exec_prefix}/lib',
      includedir = '${prefix}/include',
      PACKAGE = APPNAME,
      VERSION = VERSION)
  bld.install_files('${PREFIX}/lib/pkgconfig', 'rsdic.pc')
  bld.recurse(subdirs)
