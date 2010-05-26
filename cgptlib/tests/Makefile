# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../../
CFLAGS += -Wall -DNDEBUG -Werror -ansi
LIBS = $(TOP)/cgptlib/libcgpt.a $(FWLIB)
OBJS = cgptlib_test.o quick_sort_test.o crc32_test.o
OUT = cgptlib_test

all: $(OUT)

$(OUT): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OUT) $(OBJS)
