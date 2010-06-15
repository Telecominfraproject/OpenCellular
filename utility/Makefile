# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I$(HOSTDIR)/include \
		-I../misclibs/include \
		-I../vfirmware/include\
		-I../vboot_firmware/include\
		-I../vkernel/include
CFLAGS += $(INCLUDES)
CFLAGS += -MMD -MF $@.d
LIBS = $(BUILD)/misclibs/file_keys.o \
	$(BUILD)/misclibs/signature_digest.o \
	$(BUILD)/vfirmware/firmware_image.o \
	$(BUILD)/vkernel/kernel_image.o \
	$(HOSTLIB) \
	$(FWLIB)

BUILD_ROOT = ${BUILD}/utility

DESTDIR ?= /usr/bin

TARGET_NAMES = dumpRSAPublicKey \
		firmware_utility \
		gbb_utility \
		kernel_utility \
		load_kernel_test \
		signature_digest_utility \
		vbutil_firmware \
		vbutil_kernel \
		vbutil_key \
		vbutil_keyblock \
		verify_data

TARGET_BINS = $(addprefix ${BUILD_ROOT}/,$(TARGET_NAMES))
ALL_DEPS = $(addsuffix .d,${TARGET_BINS})

all: $(TARGET_BINS)

${BUILD_ROOT}/dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ -lcrypto

${BUILD_ROOT}/firmware_utility: firmware_utility.cc $(LIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/gbb_utility: gbb_utility.cc
	$(CXX) -DWITH_UTIL_MAIN $(CFLAGS) $< -o $@

${BUILD_ROOT}/load_kernel_test: load_kernel_test.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/kernel_utility: kernel_utility.cc $(LIBS)
	$(CXX) $(CFLAGS) $(INCLUDES) -ggdb -D__STDC_LIMIT_MACROS $< \
	-o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/signature_digest_utility: signature_digest_utility.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/vbutil_firmware: vbutil_firmware.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/vbutil_kernel: vbutil_kernel.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/vbutil_key: vbutil_key.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/vbutil_keyblock: vbutil_keyblock.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/verify_data: verify_data.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto


install: $(TARGET_BINS)
	mkdir -p $(DESTDIR)
	cp -f $(TARGET_BINS) $(DESTDIR)
	chmod a+rx $(patsubst %,$(DESTDIR)/%,$(TARGET_NAMES))

-include ALL_DEPS
