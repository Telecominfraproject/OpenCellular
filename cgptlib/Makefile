# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -Werror -ansi
INCLUDES += -Itests
SUBDIRS = tests

all: cgpt.a
	for i in $(SUBDIRS); do \
	( cd $$i ; $(MAKE)) ; \
	done

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

cgpt.a: cgptlib.o quick_sort.o crc32.o
	$(AR) rs $@ $^

clean:
	for i in $(SUBDIRS); do \
	( $(MAKE) -C $$i clean ) ; \
	done
	rm -f *.o *~ *.a
