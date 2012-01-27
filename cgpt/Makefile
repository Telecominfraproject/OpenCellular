# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -static
LDFLAGS += -luuid
BUILD_ROOT := ${BUILD}/cgpt

INCLUDES = -I$(FWDIR)/lib/cgptlib/include -I$(FWDIR)/include
LIBS = ${HOSTLIB}

DESTDIR ?= /usr/bin

PROGNAME = ${BUILD_ROOT}/cgpt

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

main: $(PROGNAME)

include ../common.mk

$(PROGNAME): $(ALL_OBJS) $(LIBS)
	$(CC) -o $(PROGNAME) $(CFLAGS) $^ $(LDFLAGS)

install: $(PROGNAME)
	mkdir -p $(DESTDIR)
	cp -f $^ $(DESTDIR)
	chmod a+rx $(patsubst ${BUILD_ROOT}/%,$(DESTDIR)/%,$^)

.PHONY: all install
