
# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../../
CC ?= cc
# Include /usr/include from inside the chroot, so that we get a version
# of endian.h which contains endian-conversion macros - htole32(), etc.
INCLUDES += -I$(TOP)/common/include -I$(TOP)/../../../chroot/usr/include/
CFLAGS += -Wall -Werror -ggdb
LIBS += $(FWLIB)

DESTDIR ?= /opt/bin

all: cgpt

cgpt: cgpt.o cgpt_add_modify_delete.o cgpt_attribute.o cgpt_dev.o \
      cgpt_options.o cgpt_repair.o cgpt_show.o cgpt_tofix.o $(LIBS)
	$(CC) -o cgpt $(CFLAGS) $^

.c.o: $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f cgpt *.o *~

install: cgpt
	mkdir -p $(DESTDIR)
	cp -f $^ $(DESTDIR)
	chmod a+rx $(patsubst %,$(DESTDIR)/%,$^)
