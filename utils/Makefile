# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CXX ?= g++
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
INCLUDES ?= -I../include/
TOP ?= ../

LIBS =  firmware_image.o kernel_image.o signature_digest.o file_keys.o \
	rollback_index.o

FIRMWARELIBS = $(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a

all: dumpRSAPublicKey verify_data file_keys.o signature_digest.o \
	firmware_image.o kernel_image.o signature_digest.o \
	signature_digest_utility firmware_utility kernel_utility \
	rollback_index.o

dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $< -o $@ -lcrypto

verify_data: verify_data.c $(LIBS) $(FIRMWARELIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARELIBS) -lcrypto

signature_digest_utility: signature_digest_utility.c $(LIBS) $(FIRMWARELIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARELIBS) -lcrypto

firmware_utility: firmware_utility.cc $(LIBS) $(FIRMWARELIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(FIRMWARELIBS) $(LIBS) $(TOP)/common/libcommon.a \
	-lcrypto

kernel_utility: kernel_utility.cc $(LIBS) $(FIRMWARELIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(FIRMWARELIBS) $(LIBS) $(TOP)/common/libcommon.a \
	-lcrypto

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

firmware_image.o: firmware_image.c
	$(CC) -ansi $(CFLAGS) $(INCLUDES) -c $< -o $@

kernel_image.o: kernel_image.c
	$(CC) -ansi $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f dumpRSAPublicKey verify_data signature_digest firmware_utility \
	kernel_utility signature_digest_utility $(LIBS)
