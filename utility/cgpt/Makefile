# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../../
CC ?= cc
INCLUDES += -I$(TOP)/common/include -I$(TOP)/../../../chroot/usr/include/
CFLAGS += -Wall -Werror -ggdb
LIBS += $(TOP)/cgptlib/libcgpt.a $(FWLIB)

all: cgpt

cgpt: cgpt.o cgpt_attribute.o cgpt_options.o cgpt_repair.o cgpt_show.o $(LIBS)
	$(CC) -o cgpt $(CFLAGS) $^

.c.o: $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f cgpt *.o *~
