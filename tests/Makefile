# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SRCS=sha_tests.c
OBJS=$(SRCS:.c=.o)
LIBS=$(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a

tests:	sha_tests

sha_tests: sha_tests.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ $(LIBS)

clean:
	rm -f $(OBJS) sha_tests verify_data signature_digest
