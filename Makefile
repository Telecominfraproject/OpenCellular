# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export CC ?= gcc
export CXX ?= g++
export CFLAGS = -Wall -DNDEBUG -O3 -Werror
export TOP = $(shell pwd)
export FWDIR=$(TOP)/vboot_firmware
export HOSTDIR=$(TOP)/host
export INCLUDES = \
	-I$(FWDIR)/include \
	-I$(TOP)/misclibs/include

export BUILD = ${TOP}/build
export FWLIB = ${BUILD}/vboot_fw.a
export HOSTLIB= ${BUILD}/vboot_host.a

SUBDIRS = vboot_firmware misclibs host vfirmware vkernel utility cgpt tests

all:
	set -e; \
	for d in $(shell find ${SUBDIRS} -name '*.c' -exec  dirname {} \; |\
		 sort -u); do \
		newdir=${BUILD}/$$d; \
		if [ ! -d $$newdir ]; then \
			mkdir -p $$newdir; \
		fi; \
	done && \
	for i in $(SUBDIRS); do \
		make -C $$i; \
	done

clean:
	/bin/rm -rf ${BUILD}

install:
	$(MAKE) -C utility install
	$(MAKE) -C cgpt install

runtests:
	$(MAKE) -C tests runtests
