# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export CC ?= gcc
export CFLAGS = -Wall -DNDEBUG -O3 -Werror
export TOP = $(shell pwd)
export FWDIR=$(TOP)/vboot_firmware
export INCLUDES = \
	-I$(FWDIR)/include \
	-I$(TOP)/misclibs/include

export FWLIB=$(FWDIR)/vboot_fw.a

SUBDIRS=vboot_firmware misclibs vfirmware vkernel utility tests

all:
	set -e; \
	for i in $(SUBDIRS); do \
		make -C $$i; \
	done

clean:
	set -e; \
	for i in $(SUBDIRS); do \
		make -C $$i clean; \
	done
