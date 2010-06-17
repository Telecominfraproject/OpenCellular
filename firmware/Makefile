# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

FWTOP := $(shell pwd)
LIBDIR = $(FWTOP)/lib
STUBDIR = $(FWTOP)/stub
TESTDIR = $(FWTOP)/linktest
BUILD_ROOT := ${BUILD}/$(shell basename ${FWTOP})

INCLUDES = \
	-I$(FWTOP)/include \
	-I$(LIBDIR)/include \
	-I$(LIBDIR)/cgptlib/include \
	-I$(LIBDIR)/cryptolib/include


# find ./lib -iname '*.c' | sort
LIB_SRCS = \
	./lib/cgptlib/cgptlib.c \
	./lib/cgptlib/cgptlib_internal.c \
	./lib/cgptlib/crc32.c \
	./lib/cryptolib/padding.c \
	./lib/cryptolib/rsa.c \
	./lib/cryptolib/rsa_utility.c \
	./lib/cryptolib/sha1.c \
	./lib/cryptolib/sha2.c \
	./lib/cryptolib/sha_utility.c \
	./lib/rollback_index.c \
	./lib/stateful_util.c \
	./lib/vboot_common.c \
	./lib/vboot_firmware.c \
	./lib/vboot_kernel.c

STUB_SRCS = \
	./stub/boot_device_stub.c \
	./stub/load_firmware_stub.c \
	./stub/tlcl.c \
	./stub/utility_stub.c

ALL_SRCS = ${LIB_SRCS} ${STUB_SRCS}

test : $(FWLIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_ROOT)/a.out \
		$(TESTDIR)/main.c $(FWLIB)

include ../common.mk

$(FWLIB) : $(ALL_OBJS)
	rm -f $@
	ar qc $@ $^
