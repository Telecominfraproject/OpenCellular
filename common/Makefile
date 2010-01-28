# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SRCS=utility_stub.c
OBJS=$(SRCS:.c=.o)

all:	libcommon.a

libcommon.a:	$(OBJS)
	ar rs $@ $<

.c.o:	$(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean: 
	rm -f $(OBJS) libcommon.a
