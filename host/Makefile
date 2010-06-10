# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LIBNAME = vboot_host.a

CC ?= gcc
CFLAGS = -Wall -DNDEBUG -O3 -Werror

HOSTTOP := $(shell pwd)
LIBDIR = $(HOSTTOP)/lib
TESTDIR = $(HOSTTOP)/linktest

INCLUDES += \
	-I$(HOSTTOP)/include \
	-I$(FWDIR)/lib/include \
	-I$(FWDIR)/lib/cgptlib/include \
	-I$(FWDIR)/lib/cryptolib/include 

# find ./lib -iname '*.c' | sort
LIB_SRCS = \
	./lib/host_common.c \
	./lib/host_key.c \
	./lib/host_misc.c \
	./lib/host_signature.c 

LIB_OBJS = $(LIB_SRCS:%.c=%.o)

test : $(LIBNAME)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TESTDIR)/a.out $(TESTDIR)/main.c \
	$(LIBNAME) $(FWLIB) -lcrypto

$(LIBNAME) : $(LIB_OBJS) $(STUB_OBJS)
	rm -f $@
	ar qc $@ $^

%o : %c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean: FORCE
	rm -f $(LIBNAME) $(LIB_OBJS) $(STUB_OBJS) $(TESTDIR)/a.out

FORCE:


.PHONY: FORCE
