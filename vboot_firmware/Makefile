# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LIBNAME = vboot_fw.a

CC ?= gcc
CFLAGS = -Wall -DNDEBUG -O3 -Werror

FWTOP := $(shell pwd)
LIBDIR = $(FWTOP)/lib
STUBDIR = $(FWTOP)/stub
TESTDIR = $(FWTOP)/linktest

INC = \
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
	./lib/firmware_image_fw.c \
	./lib/kernel_image_fw.c \
	./lib/load_kernel_fw.c \
	./lib/rollback_index.c \
	./lib/stateful_util.c

LIB_OBJS = $(LIB_SRCS:%.c=%.o)

# find ./stub -iname '*.c' | sort
STUB_SRCS = \
	./stub/boot_device_stub.c \
	./stub/tlcl.c \
	./stub/utility_stub.c

STUB_OBJS = $(STUB_SRCS:%.c=%.o)


test : $(LIBNAME)
	$(CC) $(CFLAGS) $(INC) -o $(TESTDIR)/a.out $(TESTDIR)/main.c $(LIBNAME)

$(LIBNAME) : $(LIB_OBJS) $(STUB_OBJS)
	rm -f $@
	ar qc $@ $^

%o : %c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean: FORCE
	rm -f $(LIBNAME) $(LIB_OBJS) $(STUB_OBJS) $(TESTDIR)/a.out

FORCE:


.PHONY: FORCE
