# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export CC ?= gcc
export CXX ?= g++
export CFLAGS = -Wall -Werror -DCHROMEOS_ENVIRONMENT
ifeq (${DEBUG},)
CFLAGS += -O3
else
CFLAGS += -O0 -g -DVBOOT_DEBUG
endif
ifeq (${DISABLE_NDEBUG},)
CFLAGS += -DNDEBUG
endif

export TOP = $(shell pwd)
export FWDIR=$(TOP)/firmware
export HOSTDIR=$(TOP)/host
export INCLUDES = -I$(FWDIR)/include -I$(FWDIR)/stub/include

export BUILD = ${TOP}/build
export FWLIB = ${BUILD}/vboot_fw.a
export HOSTLIB = ${BUILD}/vboot_host.a

SUBDIRS = firmware host utility cgpt tests tests/tpm_lite

all:
	set -e; \
	for d in $(shell find ${SUBDIRS} -name '*.c' -exec  dirname {} \; |\
		 sort -u); do \
		newdir=${BUILD}/$$d; \
		if [ ! -d $$newdir ]; then \
			mkdir -p $$newdir; \
		fi; \
	done; \
	make -C utility update_tlcl_structures; \
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

rbtest:
	$(MAKE) -C tests rbtest
