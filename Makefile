# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export CC ?= gcc
export CFLAGS = -Wall -DNDEBUG -O3 -Werror
export TOP = $(shell pwd)
export INCLUDES = \
	-I$(TOP)/common/include \
	-I$(TOP)/cryptolib/include \
	-I$(TOP)/misclibs/include

SUBDIRS=common cryptolib misclibs vfirmware vkernel utility tests

all:
	for i in $(SUBDIRS); do \
	( cd $$i ; $(MAKE)) ; \
	done

clean:
	for i in $(SUBDIRS); do \
	( cd $$i ; make clean) ; \
	done
