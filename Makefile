# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export FIRMWARE_ARCH
export MOCK_TPM

# This Makefile normally builds in a 'build' subdir, but use
#
#    make BUILD=<dir>
#
# to put the output somewhere else

#
# Provide default CC and CFLAGS for firmware builds; if you have any -D flags,
# please add them after this point (e.g., -DVBOOT_DEBUG).
#
# TODO(crosbug.com/16808) We hard-code u-boot's compiler flags here just
# temporarily. As we are still investigating which flags are necessary for
# maintaining a compatible ABI, etc. between u-boot and vboot_reference.
#
# As a first step, this makes the setting of CC and CFLAGS here optional, to
# permit a calling script or Makefile to set these.
#
# Flag ordering: arch, then -f, then -m, then -W
DEBUG_FLAGS := $(if ${DEBUG},-g -O0,-Os)
COMMON_FLAGS := -nostdinc -pipe \
	-ffreestanding -fno-builtin -fno-stack-protector \
	-Werror -Wall -Wstrict-prototypes $(DEBUG_FLAGS)

ifeq ($(FIRMWARE_ARCH), arm)
CC ?= armv7a-cros-linux-gnueabi-gcc
CFLAGS ?= -march=armv5 \
	-fno-common -ffixed-r8 \
	-msoft-float -marm -mabi=aapcs-linux -mno-thumb-interwork \
	$(COMMON_FLAGS)
endif
ifeq ($(FIRMWARE_ARCH), i386)
CC ?= i686-pc-linux-gnu-gcc
# Drop -march=i386 to permit use of SSE instructions
CFLAGS ?= \
	-ffunction-sections -fvisibility=hidden -fno-strict-aliasing \
	-fomit-frame-pointer -fno-toplevel-reorder -fno-dwarf2-cfi-asm \
	-mpreferred-stack-boundary=2 -mregparm=3 \
	$(COMMON_FLAGS)
endif
ifeq ($(FIRMWARE_ARCH), x86_64)
CFLAGS ?= $(COMMON_FLAGS) \
	-fvisibility=hidden -fno-strict-aliasing -fomit-frame-pointer
endif

CC ?= gcc
CXX ?= g++

# Fix compiling directly on host (outside of emake)
ifeq ($(ARCH),)
export ARCH=amd64
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

export CC CXX CFLAGS

export TOP = $(shell pwd)
export FWDIR=$(TOP)/firmware
export HOSTDIR=$(TOP)/host
ifeq ($(FIRMWARE_ARCH),)
export INCLUDES = -I$(FWDIR)/include -I$(FWDIR)/stub/include
else
export INCLUDES = -I$(FWDIR)/include -I$(FWDIR)/arch/$(FIRMWARE_ARCH)/include
endif

export BUILD ?= ${TOP}/build
export FWLIB = ${BUILD}/vboot_fw.a
export HOSTLIB = ${BUILD}/vboot_host.a
export DUMPKERNELCONFIGLIB = ${BUILD}/libdump_kernel_config.a

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
	[ -z "$(FIRMWARE_ARCH)" ] && $(MAKE) -C utility update_tlcl_structures; \
	for i in $(SUBDIRS); do \
		make -C $$i; \
	done

libcgpt_cc:
	mkdir -p ${BUILD}/cgpt ${BUILD}/firmware/lib/cgptlib ${BUILD}/firmware/stub 
	$(MAKE) -C cgpt libcgpt_cc

cgptmanager_tests: libcgpt_cc
	mkdir -p ${BUILD}/tests
	$(MAKE) -C tests cgptmanager_tests

libdump_kernel_config:
	mkdir -p ${BUILD}/utility
	$(MAKE) -C utility $(DUMPKERNELCONFIGLIB)

clean:
	/bin/rm -rf ${BUILD}

install:
	$(MAKE) -C utility install
	$(MAKE) -C cgpt install

runtests:
	$(MAKE) -C tests runtests

runcgptmanagertests:
	$(MAKE) -C tests runcgptmanagertests

rbtest:
	$(MAKE) -C tests rbtest

runbmptests:
	$(MAKE) -C tests runbmptests
