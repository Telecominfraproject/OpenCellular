# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

FWTOP := $(shell pwd)
LIBDIR = $(FWTOP)/lib
STUBDIR = $(FWTOP)/stub
TESTDIR = $(FWTOP)/linktest
BUILD_ROOT := ${BUILD}/$(shell basename ${FWTOP})
LIBS = $(FWLIB)  # Firmware library must be self-contained

# Disable rollback TPM when compiling locally, since otherwise
# load_kernel_test attempts to talk to the TPM.
ifeq ($(FIRMWARE_ARCH),)
CFLAGS += -DDISABLE_ROLLBACK_TPM
endif

INCLUDES = \
	-I$(FWTOP)/include \
	-I$(LIBDIR)/include \
	-I$(LIBDIR)/cgptlib/include \
	-I$(LIBDIR)/cryptolib/include \
	-I$(LIBDIR)/tpm_lite/include

ifeq ($(FIRMWARE_ARCH),)
INCLUDES += -I$(STUBDIR)/include
else
INCLUDES += -I$(FWDIR)/arch/$(FIRMWARE_ARCH)/include
endif

# find ./lib -iname '*.c' | sort
LIB_SRCS = \
	./lib/cgptlib/cgptlib.c \
	./lib/cgptlib/cgptlib_internal.c \
	./lib/cgptlib/crc32.c \
	./lib/cryptolib/padding.c \
	./lib/cryptolib/rsa.c \
	./lib/cryptolib/rsa_utility.c \
	./lib/cryptolib/sha1.c \
	./lib/cryptolib/sha2.c \
	./lib/cryptolib/sha_utility.c \
	./lib/rollback_index.c \
	./lib/stateful_util.c \
	./lib/tpm_lite/tlcl.c \
	./lib/utility.c \
	./lib/vboot_common.c \
	./lib/vboot_firmware.c \
	./lib/vboot_kernel.c

LIB_OBJS = $(LIB_SRCS:%.c=${BUILD_ROOT}/%.o)

STUB_SRCS = \
	./stub/boot_device_stub.c \
	./stub/load_firmware_stub.c \
	./stub/tpm_lite_stub.c \
	./stub/utility_stub.c

STUB_OBJS = $(STUB_SRCS:%.c=${BUILD_ROOT}/%.o)

ALL_SRCS = ${LIB_SRCS} ${STUB_SRCS} version.c

ifeq ($(FIRMWARE_ARCH),)
test : $(STUB_OBJS) $(FWLIB) update-version
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_ROOT)/a.out \
		$(TESTDIR)/main.c $(STUB_OBJS) $(LIBS)
else
test : $(FWLIB)
endif

# This is executed at every make, to see if anything has changed
update-version :
	find \( -name '*.[ch]' -o -name 'Makefile*' \) -a \! -name version.c \
		| sort | xargs cat | md5sum | cut -c 25-32 > \
	${BUILD_ROOT}/x.tmp && \
	echo "char* VbootVersion = \"VBOOv=$$(cat ${BUILD_ROOT}/x.tmp)\";" > \
		${BUILD_ROOT}/version.tmp && \
	(cmp -s ${BUILD_ROOT}/version.tmp version.c || \
		( echo "** Updating version.c **" &&  \
		  cp ${BUILD_ROOT}/version.tmp version.c))

include ../common.mk

$(FWLIB) : $(LIB_OBJS)
	rm -f $@
	ar qc $@ $^
