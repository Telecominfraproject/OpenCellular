# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CFLAGS = -Wall
INCLUDES ?= -I../include/
TOP ?= ../

LIBS = $(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a -lrt

tests: sha_tests sha_benchmark

sha_tests: sha_tests.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ $(LIBS)

sha_benchmark:	sha_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $^ -o $@ $(LIBS)

clean:
	rm -f sha_tests sha_benchmark
