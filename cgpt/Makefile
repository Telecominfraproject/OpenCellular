# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_ROOT := ${BUILD}/cgpt

INCLUDES = -I$(FWDIR)/lib/cgptlib/include -I$(FWDIR)/include
LIBS = ${HOSTLIB}
LDLIBS += -luuid
LDFLAGS += -static

DESTDIR ?= /usr/bin

PROGNAME = ${BUILD_ROOT}/cgpt

LIB_CGPT_CC = ${BUILD_ROOT}/libcgpt-cc.a

ALL_SRCS = \
	cgpt.c \
	cgpt_create.c \
	cgpt_add.c \
	cgpt_boot.c \
	cgpt_show.c \
	cgpt_repair.c \
	cgpt_prioritize.c \
	cgpt_find.c \
	cmd_show.c \
	cmd_repair.c \
	cmd_create.c \
	cmd_add.c \
	cmd_boot.c \
	cmd_find.c \
	cmd_prioritize.c \
	cgpt_common.c

LIB_CGPT_CC_SRCS = \
	CgptManager.cc \
	cgpt_create.c \
	cgpt_add.c \
	cgpt_boot.c \
	cgpt_show.c \
	cgpt_repair.c \
	cgpt_prioritize.c \
	cgpt_common.c \
	../firmware/lib/cgptlib/crc32.c \
	../firmware/lib/cgptlib/cgptlib_internal.c \
	../firmware/stub/utility_stub.c

main: $(PROGNAME)

include ../build.mk

LIB_CGPT_CC_OBJS = $(filter %.o, \
	$(LIB_CGPT_CC_SRCS:%.c=${BUILD_ROOT}/%.o) \
	$(LIB_CGPT_CC_SRCS:%.cc=${BUILD_ROOT}/%.o))
LIB_CGPT_CC_DEPS = $(LIB_CGPT_CC_OBJS:%.o=%.o.d)

libcgpt_cc: $(LIB_CGPT_CC)

$(LIB_CGPT_CC): $(LIB_CGPT_CC_OBJS)
	rm -f $@
	ar qc $@ $^

$(PROGNAME): $(ALL_OBJS) $(LIBS)
	$(CC) -o $(PROGNAME) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

install: $(PROGNAME)
	mkdir -p $(DESTDIR)
	cp -f $^ $(DESTDIR)
	chmod a+rx $(patsubst ${BUILD_ROOT}/%,$(DESTDIR)/%,$^)

.PHONY: all install
