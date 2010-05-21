# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -Werror -ansi
INCLUDES += -Itests
SUBDIRS = tests

all: libcgpt.a
	set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i; \
	done

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

libcgpt.a: cgptlib.o quick_sort.o crc32.o
	$(AR) rs $@ $^

clean:
	set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i clean; \
	done
	rm -f *.o *~ *.a
