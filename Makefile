name := gfxinit

ifeq ($(MAKECMDGOALS),gfx_test)
prefixed-name	:= gfx_test
link-type	:= program
GFXINIT_TEST	:= y
endif

gfxinit-deplibs := libhw

libhw-dir ?= ../libhwbase/dest
include $(libhw-dir)/Makefile

gfx_test: $(binary)

.PHONY: gfx_test
