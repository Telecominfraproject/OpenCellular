# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export FIRMWARE_ARCH

export CC ?= gcc
export CXX ?= g++
export CFLAGS = -Wall -Werror

ifeq (${DEBUG},)
CFLAGS += -O3
else
CFLAGS += -O0 -g
endif

# Override CC and CFLAGS only if FIRMWARE_CONFIG_PATH is not empty, but we
# wish to preserve -D flags (so move all -D flags after this).
ifneq (${FIRMWARE_CONFIG_PATH},)
include ${FIRMWARE_CONFIG_PATH}
endif

ifeq ($(FIRMWARE_ARCH),)
CFLAGS += -DCHROMEOS_ENVIRONMENT
endif

ifneq (${DEBUG},)
CFLAGS += -DVBOOT_DEBUG
endif

ifeq (${DISABLE_NDEBUG},)
CFLAGS += -DNDEBUG
endif

export TOP = $(shell pwd)
export FWDIR=$(TOP)/firmware
export HOSTDIR=$(TOP)/host
ifeq ($(FIRMWARE_ARCH),)
export INCLUDES = -I$(FWDIR)/include -I$(FWDIR)/stub/include
else
export INCLUDES = -I$(FWDIR)/include -I$(FWDIR)/arch/$(FIRMWARE_ARCH)/include
endif

export BUILD = ${TOP}/build
export FWLIB = ${BUILD}/vboot_fw.a
export HOSTLIB = ${BUILD}/vboot_host.a

ifeq ($(FIRMWARE_ARCH),)
SUBDIRS = firmware host utility cgpt tests tests/tpm_lite
else
SUBDIRS = firmware
endif

all:
	set -e; \
	for d in $(shell find ${SUBDIRS} -name '*.c' -exec  dirname {} \; |\
		 sort -u); do \
		newdir=${BUILD}/$$d; \
		if [ ! -d $$newdir ]; then \
			mkdir -p $$newdir; \
		fi; \
	done; \
	[ -z "$(FIRMWARE_ARCH)" ] && make -C utility update_tlcl_structures; \
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
