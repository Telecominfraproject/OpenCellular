# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
TOP ?= ..
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
CFLAGS += -static
LDFLAGS += -luuid
FWDIR=$(TOP)/vboot_firmware

INCLUDES = -I$(FWDIR)/lib/cgptlib/include
LIBS = $(FWDIR)/vboot_fw.a

DESTDIR ?= /usr/bin

PROGNAME = cgpt

OBJS= \
	cgpt.o \
	cmd_show.o \
	cmd_repair.o \
	cmd_create.o \
	cmd_add.o \
	cmd_boot.o \
	cgpt_common.o


all: $(PROGNAME)

$(PROGNAME): $(OBJS) $(LIBS)
	$(CC) -o $(PROGNAME) $(CFLAGS) $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(PROGNAME) *.o *~

install: $(PROGNAME)
	mkdir -p $(DESTDIR)
	cp -f $^ $(DESTDIR)
	chmod a+rx $(patsubst %,$(DESTDIR)/%,$^)

.PHONY: all clean install
