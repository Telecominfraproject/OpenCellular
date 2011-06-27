# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

HOSTTOP := $(shell pwd)
TESTDIR = $(HOSTTOP)/linktest
BUILD_ROOT := ${BUILD}/$(shell basename ${HOSTTOP})

INCLUDES += \
	-I$(HOSTTOP)/include \
	-I$(HOSTTOP)/arch/$(ARCH)/include \
	-I$(FWDIR)/lib/include \
	-I$(FWDIR)/lib/cgptlib/include \
	-I$(FWDIR)/lib/cryptolib/include

# find ./lib -iname '*.c' | sort
LIB_SRCS = \
	./arch/$(ARCH)/lib/crossystem_arch.c \
	./lib/crossystem.c \
	./lib/file_keys.c \
	./lib/fmap.c \
	./lib/host_common.c \
	./lib/host_key.c \
	./lib/host_keyblock.c \
	./lib/host_misc.c \
	./lib/host_signature.c \
	./lib/signature_digest.c

STUB_SRCS = \
	../firmware/stub/boot_device_stub.c \
	../firmware/stub/load_firmware_stub.c \
	../firmware/stub/tpm_lite_stub.c \
	../firmware/stub/utility_stub.c \
	../firmware/stub/vboot_api_stub.c

ALL_SRCS = ${LIB_SRCS} ${STUB_SRCS}

test : $(HOSTLIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_ROOT)/a.out $(TESTDIR)/main.c \
	$(HOSTLIB) -lcrypto

include ../common.mk

$(HOSTLIB) : $(ALL_OBJS) $(FWLIB)
	rm -rf $@ $(BUILD_ROOT)/.tmp
	mkdir -p $(BUILD_ROOT)/.tmp
	cd $(BUILD_ROOT)/.tmp ; ar x $(FWLIB)
	ar qc $@ $^ $(BUILD_ROOT)/.tmp/*.o
