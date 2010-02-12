# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CFLAGS=-Wall -ggdb
INCLUDES ?= -I../include/
TOP ?= ../


LIBS=-lcrypto
FIRMWARELIBS=$(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a

all:	dumpRSAPublicKey verify_data signature_digest firmware_utility

dumpRSAPublicKey:	dumpRSAPublicKey.c
		$(CC) $(CFLAGS) $(LIBS) $< -o $@

verify_data:	verify_data.c file_keys.o
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ file_keys.o $(FIRMWARELIBS) 

signature_digest:	signature_digest.c
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@ $(FIRMWARELIBS)

firmware_utility:	firmware_utility.c firmware_image.o file_keys.o
	$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) $< -o $@  firmware_image.o $(FIRMWARELIBS)

file_keys.o:	file_keys.c
		$(CC) $(CFLAGS) -DNDEBUG $(INCLUDES) -c $< -o $@

firmware_image.o:	firmware_image.c
		$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f dumpRSAPublicKey verify_data signature_digest firmware_image.o file_keys.o

