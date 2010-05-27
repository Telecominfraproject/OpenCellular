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


LIB_SRCS := $(shell find $(LIBDIR) -iname '*.c')
LIB_OBJS := $(LIB_SRCS:%.c=%.o)

STUB_SRCS := $(shell find $(STUBDIR) -iname '*.c')
STUB_OBJS := $(STUB_SRCS:%.c=%.o)


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
