# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export CC ?= gcc
export CFLAGS = -Wall -ansi -DNDEBUG
export TOP = $(shell pwd)
export INCLUDEDIR = $(TOP)/include
export INCLUDES = -I$(INCLUDEDIR)

SUBDIRS=common crypto utils tests

all:
	for i in $(SUBDIRS); do \
	( cd $$i ; $(MAKE)) ; \
	done

clean:
	for i in $(SUBDIRS); do \
	( cd $$i ; make clean) ; \
	done
