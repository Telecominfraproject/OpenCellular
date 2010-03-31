# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../
CC ?= gcc
CXX ?= g++
INCLUDES += -I./include \
		-I../cryptolib/include \
		-I../common/include \
		-I../misclibs/include \
		-I../vfirmware/include\
		-I../vkernel/include
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror $(INCLUDES)
LIBS = $(TOP)/misclibs/file_keys.o \
	$(TOP)/misclibs/signature_digest.o \
	$(TOP)/vfirmware/firmware_image.o \
	$(TOP)/vfirmware/firmware_image_fw.o \
	$(TOP)/vkernel/kernel_image.o \
	$(TOP)/vkernel/kernel_image_fw.o
FIRMWARELIBS = $(TOP)/cryptolib/libcrypto.a $(TOP)/common/libcommon.a

TARGET_BINS = dumpRSAPublicKey \
		firmware_utility \
		kernel_utility \
		signature_digest_utility \
		verify_data

all: $(TARGET_BINS)

dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ -lcrypto

firmware_utility: firmware_utility.cc $(LIBS) $(FIRMWARELIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(FIRMWARELIBS) $(LIBS) $(TOP)/common/libcommon.a \
	-lcrypto

kernel_utility: kernel_utility.cc $(LIBS) $(FIRMWARELIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(FIRMWARELIBS) $(LIBS) $(TOP)/common/libcommon.a \
	-lcrypto

signature_digest_utility: signature_digest_utility.c $(LIBS) $(FIRMWARELIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARELIBS) -lcrypto

verify_data: verify_data.c $(LIBS) $(FIRMWARELIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARELIBS) -lcrypto

clean:
	rm -f $(TARGET_BINS)

