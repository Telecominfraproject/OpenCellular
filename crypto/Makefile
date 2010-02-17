# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SRCS = rsa.c sha1.c sha2.c padding.c rsa_utility.c sha_utility.c
OBJS = $(SRCS:.c=.o)

all: libcrypto.a

libcrypto.a: $(OBJS)
	ar rs libcrypto.a $(OBJS)

padding.c: genpadding.sh
	./genpadding.sh >$@

.c.o: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean: 
	rm -f $(OBJS) libcrypto.a
