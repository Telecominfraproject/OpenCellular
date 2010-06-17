# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

HOSTTOP := $(shell pwd)
TESTDIR = $(HOSTTOP)/linktest
BUILD_ROOT := ${BUILD}/$(shell basename ${HOSTTOP})

INCLUDES += \
	-I$(HOSTTOP)/include \
	-I$(FWDIR)/lib/include \
	-I$(FWDIR)/lib/cgptlib/include \
	-I$(FWDIR)/lib/cryptolib/include

# find ./lib -iname '*.c' | sort
ALL_SRCS = \
	./lib/file_keys.c \
	./lib/host_common.c \
	./lib/host_key.c \
	./lib/host_keyblock.c \
	./lib/host_misc.c \
	./lib/host_signature.c \
	./lib/signature_digest.c

test : $(HOSTLIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_ROOT)/a.out $(TESTDIR)/main.c \
	$(HOSTLIB) $(FWLIB) -lcrypto

include ../common.mk

$(HOSTLIB) : $(ALL_OBJS)
	rm -f $@
	ar qc $@ $^
