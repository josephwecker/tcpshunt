#!/usr/bin/env python
# encoding: utf-8
APPNAME = 'tcpshunt'
VERSION = '0.0.1'

top     = '.'
out     = '.build'

from waflib import Configure
Configure.autoconfig = True


def build(bld):
    pass

def options(opt):
    opt.load('compiler_c')
    opt.load('gnu_dirs')

def configure(conf):
    import waflib.extras.cpuinfo as cpuinfo
    import platform
    conf.load('compiler_c')

