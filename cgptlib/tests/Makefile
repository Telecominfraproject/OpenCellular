# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -DNDEBUG -Werror -ansi
LIBS = ../cgpt.a ../../common/libcommon.a
OBJS = cgptlib_test.o quick_sort_test.o
OUT = cgptlib_test

all: $(OUT)

$(OUT): $(OBJS) $(LIBS)
	$(CC) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OUT) $(OBJS)
