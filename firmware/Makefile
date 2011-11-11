# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This Makefile is designed to be called from the top-level vboot_reference one

FWTOP := $(shell pwd)
LIBDIR = $(FWTOP)/lib
STUBDIR = $(FWTOP)/stub
TESTDIR = $(FWTOP)/linktest
BUILD_ROOT := ${BUILD}/$(shell basename ${FWTOP})
LIBS = $(FWLIB)  # Firmware library must be self-contained

# Disable rollback TPM when compiling locally, since otherwise
# load_kernel_test attempts to talk to the TPM.
ifeq ($(FIRMWARE_ARCH),)
$(warning FIRMWARE_ARCH not defined, assuming local compile)
CFLAGS += -DDISABLE_ROLLBACK_TPM
endif

# TPM-specific flags.  These depend on the particular TPM we're targeting for.
# They are needed here only for compiling parts of the firmware code into
# user-level tests.

# TPM_BLOCKING_CONTINUESELFTEST is defined if TPM_ContinueSelfTest blocks until
# the self test has completed.

CFLAGS += -DTPM_BLOCKING_CONTINUESELFTEST

# TPM_MANUAL_SELFTEST is defined if the self test must be started manually
# (with a call to TPM_ContinueSelfTest) instead of starting automatically at
# power on.
#
# We sincerely hope that TPM_BLOCKING_CONTINUESELFTEST and TPM_MANUAL_SELFTEST
# are not both defined at the same time.  (See comment in code.)

# CFLAGS += -DTPM_MANUAL_SELFTEST

#
# Unrolling loops in cryptolib makes it faster
#

ifeq ($(FIRMWARE_ARCH),i386)
CFLAGS += -DUNROLL_LOOPS
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
	./lib/stateful_util.c \
	./lib/utility.c \
	./lib/utility_string.c \
	./lib/vboot_api_init.c \
	./lib/vboot_api_firmware.c \
	./lib/vboot_api_kernel.c \
	./lib/vboot_audio.c \
	./lib/vboot_common.c \
	./lib/vboot_display.c \
	./lib/vboot_firmware.c \
	./lib/vboot_kernel.c \
	./lib/vboot_nvstorage.c

ifeq ($(MOCK_TPM),)
LIB_SRCS += \
	./lib/rollback_index.c \
	./lib/tpm_bootmode.c \
	./lib/tpm_lite/tlcl.c
else
LIB_SRCS += \
	./lib/mocked_rollback_index.c \
	./lib/mocked_tpm_bootmode.c \
	./lib/tpm_lite/mocked_tlcl.c
endif

LIB_OBJS = $(LIB_SRCS:%.c=${BUILD_ROOT}/%.o)

STUB_SRCS = \
	./stub/tpm_lite_stub.c \
	./stub/utility_stub.c \
	./stub/vboot_api_stub.c \
	./stub/vboot_api_stub_disk.c

STUB_OBJS = $(STUB_SRCS:%.c=${BUILD_ROOT}/%.o)

ALL_SRCS = ${LIB_SRCS} ${STUB_SRCS}

ifeq ($(FIRMWARE_ARCH),)
test : $(STUB_OBJS) $(FWLIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_ROOT)/a.out \
		$(TESTDIR)/main.c $(STUB_OBJS) $(LIBS)
else
test : $(FWLIB)
endif

include ../common.mk

$(FWLIB) : $(LIB_OBJS)
	rm -f $@
	ar qc $@ $^
