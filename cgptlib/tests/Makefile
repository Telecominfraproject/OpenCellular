# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= cc
CFLAGS ?= -Wall -DNDEBUG -Werror -ansi
LIBS = ../cgpt.a ../../common/libcommon.a
OBJS = cgpt_test.o
OUT = cgpt_test

all: $(OUT)

$(OUT): $(OBJS) $(LIBS)
	$(CC) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OUT) $(OBJS)
