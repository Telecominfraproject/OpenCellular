# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../
CC ?= gcc
CXX ?= g++
INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I../misclibs/include \
		-I../vfirmware/include\
		-I../vboot_firmware/include\
		-I../vkernel/include
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror $(INCLUDES)
LIBS = $(TOP)/misclibs/file_keys.o \
	$(TOP)/misclibs/signature_digest.o \
	$(TOP)/vfirmware/firmware_image.o \
	$(TOP)/vkernel/kernel_image.o
SUBDIRS = cgpt

DESTDIR ?= /opt/bin

TARGET_BINS = dumpRSAPublicKey \
		firmware_utility \
		gbb_utility \
		kernel_utility \
		signature_digest_utility \
		verify_data

all: $(TARGET_BINS) subdirs

.PHONY: subdirs
subdirs:
	set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i $(MAKECMDGOALS); \
	done

dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ -lcrypto

firmware_utility: firmware_utility.cc $(LIBS) $(FWLIB)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(LIBS) $(FWLIB) -lcrypto

gbb_utility: gbb_utility.cc 
	$(CXX) -DWITH_UTIL_MAIN $(CFLAGS) $(INCLUDES) $< -o $@

kernel_utility: kernel_utility.cc $(LIBS) $(FWLIB)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(LIBS) $(FWLIB) -lcrypto

signature_digest_utility: signature_digest_utility.c $(LIBS) $(FWLIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FWLIB) -lcrypto

verify_data: verify_data.c $(LIBS) $(FWLIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FWLIB) -lcrypto

clean:
	set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i clean; \
	done
	rm -f $(TARGET_BINS)

install: $(TARGET_BINS) subdirs
	mkdir -p $(DESTDIR)
	cp -f $(TARGET_BINS) $(DESTDIR)
	chmod a+rx $(patsubst %,$(DESTDIR)/%,$(TARGET_BINS))
