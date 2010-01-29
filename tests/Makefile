# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SRCS=sha_tests.c verify_data.c digest_utility.c signature_digest.c
OBJS=$(SRCS:.c=.o)
LIBS=$(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a

tests:	sha_tests verify_data signature_digest

sha_tests:	sha_tests.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ $(LIBS)

verify_data:	verify_data.c digest_utility.o
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ digest_utility.o $(LIBS)

signature_digest:	signature_digest.c digest_utility.o
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ digest_utility.o $(LIBS)

digest_utility.o:	digest_utility.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ 
clean:
	rm -f $(OBJS) sha_tests verify_data
