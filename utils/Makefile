# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CXX ?= g++
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
INCLUDES ?= -I../include/
TOP ?= ../

LIBS = file_keys.o \
	firmware_image.o \
	firmware_image_fw.o \
	kernel_image.o \
	kernel_image_fw.o \
	rollback_index.o \
	signature_digest.o 

FIRMWARELIBS = $(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a

TARGET_BINS = $(LIBS) \
		dumpRSAPublicKey \
		firmware_utility \
		kernel_utility \
		signature_digest_utility \
		verify_data

all: $(TARGET_BINS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $< -o $@ -lcrypto

firmware_image_fw.o: firmware_image_fw.c
	$(CC) $(CFLAGS) -ansi $(INCLUDES) -c $^ -o $@

firmware_utility: firmware_utility.cc $(LIBS) $(FIRMWARELIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(FIRMWARELIBS) $(LIBS) $(TOP)/common/libcommon.a \
	-lcrypto

kernel_image_fw.o: kernel_image_fw.c
	$(CC) $(CFLAGS) -ansi $(INCLUDES) -c $< -o $@

kernel_utility: kernel_utility.cc $(LIBS) $(FIRMWARELIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(FIRMWARELIBS) $(LIBS) $(TOP)/common/libcommon.a \
	-lcrypto

signature_digest_utility: signature_digest_utility.c $(LIBS) $(FIRMWARELIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARELIBS) -lcrypto

verify_data: verify_data.c $(LIBS) $(FIRMWARELIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARELIBS) -lcrypto

clean:
	rm -f $(TARGET_BINS) $(LIBS)

