# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

FWDIR ?= ../vboot_firmware/
TOP ?= ../
CC ?= gcc
CXX ?= g++
INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I$(HOSTDIR)/include \
		-I../misclibs/include \
		-I../vfirmware/include\
		-I../vboot_firmware/include\
		-I../vkernel/include
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror $(INCLUDES)
LIBS = $(TOP)/misclibs/file_keys.o \
	$(TOP)/misclibs/signature_digest.o \
	$(TOP)/vfirmware/firmware_image.o \
	$(TOP)/vkernel/kernel_image.o \
	$(HOSTLIB) \
	$(FWLIB)

DESTDIR ?= /usr/bin

TARGET_BINS = dumpRSAPublicKey \
		firmware_utility \
		gbb_utility \
		kernel_utility \
		load_kernel_test \
		signature_digest_utility \
		vbutil_key \
		vbutil_kernel \
		vbutil_keyblock \
		verify_data

all: $(TARGET_BINS)

dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ -lcrypto

firmware_utility: firmware_utility.cc $(LIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(LIBS) -lcrypto

gbb_utility: gbb_utility.cc 
	$(CXX) -DWITH_UTIL_MAIN $(CFLAGS) $(INCLUDES) $< -o $@

load_kernel_test: load_kernel_test.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

kernel_utility: kernel_utility.cc $(LIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(LIBS) -lcrypto

signature_digest_utility: signature_digest_utility.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

vbutil_kernel: vbutil_kernel.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

vbutil_key: vbutil_key.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

vbutil_keyblock: vbutil_keyblock.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

verify_data: verify_data.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

clean:
	rm -f $(TARGET_BINS)

install: $(TARGET_BINS)
	mkdir -p $(DESTDIR)
	cp -f $(TARGET_BINS) $(DESTDIR)
	chmod a+rx $(patsubst %,$(DESTDIR)/%,$(TARGET_BINS))

%o : %c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
