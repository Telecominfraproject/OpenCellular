# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../
SRCS = rsa.c sha1.c sha2.c padding.c rsa_utility.c sha_utility.c
OBJS = $(SRCS:.c=.o)
CFLAGS += -DUNROLL_LOOPS -DHAVE_ENDIAN_H -DHAVE_LITTLE_ENDIAN
INCLUDES += -I./include/ -I$(TOP)/common/include/

all: libcrypto.a

libcrypto.a: $(OBJS)
	ar rs libcrypto.a $(OBJS)

padding.c: $(TOP)/scripts/genpadding.sh
	$(TOP)/scripts/genpadding.sh >$@

.c.o: $(OBJS)
	$(CC) $(CFLAGS) -ansi $(INCLUDES) -c $< -o $@

clean: 
	rm -f $(OBJS) libcrypto.a
