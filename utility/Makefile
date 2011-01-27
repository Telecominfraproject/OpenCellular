# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I$(FWDIR)/lib/tpm_lite/include \
		-I$(HOSTDIR)/include
CFLAGS += $(INCLUDES)
CFLAGS += -MMD -MF $@.d
LIBS = $(HOSTLIB)
HOSTCC = cc

BUILD_ROOT = ${BUILD}/utility

DESTDIR ?= /usr/bin

TARGET_NAMES = dumpRSAPublicKey \
		dump_kernel_config \
		gbb_utility \
		bmpblk_utility \
		load_kernel_test \
		signature_digest_utility \
		tlcl_generator \
		tpm_init_temp_fix \
		tpmc \
		vbutil_firmware \
		vbutil_kernel \
		vbutil_key \
		vbutil_keyblock \
		verify_data \
		dev_make_keypair \
		dev_sign_file \
		dump_fmap \
		dev_debug_vboot \
		pack_firmware_image

TARGET_BINS = $(addprefix ${BUILD_ROOT}/,$(TARGET_NAMES))
ALL_DEPS = $(addsuffix .d,${TARGET_BINS})

all: $(TARGET_BINS)

${BUILD_ROOT}/dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ -lcrypto

${BUILD_ROOT}/dump_kernel_config: dump_kernel_config.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/gbb_utility: gbb_utility.cc
	$(CXX) -DWITH_UTIL_MAIN $(CFLAGS) $< -o $@

${BUILD_ROOT}/bmpblk_utility: bmpblk_utility.cc
	$(CXX) -DWITH_UTIL_MAIN -lyaml $(CFLAGS) $< -o $@

${BUILD_ROOT}/load_kernel_test: load_kernel_test.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/signature_digest_utility: signature_digest_utility.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/tlcl_generator: tlcl_generator.c
	$(HOSTCC) $(CFLAGS) $(INCLUDES) -fpack-struct $< -o $@

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

${BUILD_ROOT}/sign_image: sign_image.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/tpm_init_temp_fix: tpm_init_temp_fix.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

${BUILD_ROOT}/tpm_set_readsrkpub: tpm_set_readsrkpub.c
	$(CC) $(CFLAGS) $< -o $@ -ltspi

${BUILD_ROOT}/dev_make_keypair: dev_make_keypair
	cp -f $< $@
	chmod +x $@

${BUILD_ROOT}/dev_debug_vboot: dev_debug_vboot
	cp -f $< $@
	chmod +x $@

${BUILD_ROOT}/tpmc: tpmc.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

${BUILD_ROOT}/dev_sign_file: dev_sign_file.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) -lcrypto

${BUILD_ROOT}/dump_fmap: dump_fmap.c
	$(CC) $(CFLAGS) $< -o $@

${BUILD_ROOT}/pack_firmware_image: pack_firmware_image
	cp -f $< $@
	chmod +x $@

install: $(TARGET_BINS)
	mkdir -p $(DESTDIR)
	cp -f $(TARGET_BINS) $(DESTDIR)
	chmod a+rx $(patsubst %,$(DESTDIR)/%,$(TARGET_NAMES))

STRUCTURES_TMP=${BUILD}/tlcl_structures.tmp
STRUCTURES_SRC=${FWDIR}/lib/tpm_lite/include/tlcl_structures.h

update_tlcl_structures: ${BUILD_ROOT}/tlcl_generator
	${BUILD_ROOT}/tlcl_generator > $(STRUCTURES_TMP)
	cmp -s $(STRUCTURES_TMP) $(STRUCTURES_SRC) || \
		( echo "%% Updating structures.h %%" && \
		  cp $(STRUCTURES_TMP) $(STRUCTURES_SRC) )

-include ${ALL_DEPS}
