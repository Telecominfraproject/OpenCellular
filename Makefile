# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export CC ?= gcc
export CFLAGS = -Wall -DNDEBUG -O3 -Werror
export TOP = $(shell pwd)
export FWDIR=$(TOP)/vboot_firmware
export HOSTDIR=$(TOP)/host
export INCLUDES = \
	-I$(FWDIR)/include \
	-I$(TOP)/misclibs/include

export FWLIB=$(FWDIR)/vboot_fw.a
export HOSTLIB=$(HOSTDIR)/vboot_host.a

SUBDIRS=vboot_firmware misclibs host vfirmware vkernel utility tests

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

install:
	$(MAKE) -C utility install

runtests:
	$(MAKE) -C tests runtests
