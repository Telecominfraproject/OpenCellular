name := gfxinit

gfxinit-deplibs := libhw

libhw-dir ?= ../libhwbase/dest
include $(libhw-dir)/Makefile
