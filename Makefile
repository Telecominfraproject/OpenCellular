# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export FIRMWARE_ARCH
export MOCK_TPM

export CC ?= gcc
export CXX ?= g++
export CFLAGS = -Wall -Werror

ifeq (${DEBUG},)
CFLAGS += -O3
else
CFLAGS += -O0 -g
endif

#
# TODO We hard-code u-boot's compiler flags here just temporarily. As we are
# still investigating which flags are necessary for maintaining a compatible
# ABI, etc. between u-boot and vboot_reference.
#
# Override CC and CFLAGS for firmware builds; if you have any -D flags, please
# add them after this point (e.g., -DVBOOT_DEBUG).
#
ifeq ($(FIRMWARE_ARCH), "arm")
CC = armv7a-cros-linux-gnueabi-gcc
CFLAGS = -g -Os -fno-common -ffixed-r8 -msoft-float -fno-builtin \
	-ffreestanding -nostdinc \
	-isystem /usr/lib/gcc/armv7a-cros-linux-gnueabi/4.4.3/gcc/armv7a-cros-linux-gnueabi/4.4.3/include \
	-pipe -marm -mabi=aapcs-linux -mno-thumb-interwork -march=armv5 \
	-Werror -Wall -Wstrict-prototypes -fno-stack-protector
endif
ifeq ($(FIRMWARE_ARCH), "i386")
CC = i686-pc-linux-gnu-gcc
CFLAGS = -g -Os -ffunction-sections -fvisibility=hidden -fno-builtin \
	-ffreestanding -nostdinc \
	-isystem /usr/lib/gcc/i686-pc-linux-gnu/4.4.3/gcc/i686-pc-linux-gnu/4.4.3/include \
	-pipe -fno-strict-aliasing -Wstrict-prototypes -mregparm=3 \
	-fomit-frame-pointer -ffreestanding -fno-toplevel-reorder \
	-fno-stack-protector -mpreferred-stack-boundary=2 -fno-dwarf2-cfi-asm \
	-march=i386 -Werror -Wall -Wstrict-prototypes -fno-stack-protector
endif

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

runbmptests:
	$(MAKE) -C tests runbmptests
