# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CFLAGS = -Wall -DNDEBUG
INCLUDES ?= -I../include/
TOP ?= ../

LIBS = $(TOP)/utils/firmware_image.o $(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a  \
	$(TOP)/utils/file_keys.o -lrt

tests: firmware_image_tests sha_tests sha_benchmark

sha_tests: sha_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

firmware_image_tests: firmware_image_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

sha_benchmark:	sha_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

clean:
	rm -f sha_tests sha_benchmark firmware_image_tests
