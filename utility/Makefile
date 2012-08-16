# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
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
CRYPTO_LIBS := $(shell $(PKG_CONFIG) --libs libcrypto)

BUILD_ROOT = ${BUILD}/utility

DESTDIR ?= /usr/bin

# Special flags for auto-update toolkits (must be statically linked).
AU_LDFLAGS = $(LDFLAGS) -static
AU_TARGETS = crossystem \
		dump_fmap \
		gbb_utility

TARGET_NAMES = $(AU_TARGETS) \
		dumpRSAPublicKey \
		dump_kernel_config \
		load_kernel_test \
		pad_digest_utility \
		signature_digest_utility \
		tlcl_generator \
		tpm_init_temp_fix \
		tpmc \
		vbutil_ec \
		vbutil_firmware \
		vbutil_kernel \
		vbutil_key \
		vbutil_keyblock \
		verify_data \
		dev_make_keypair \
		dev_sign_file \
		dev_debug_vboot \
		vbutil_what_keys

ifeq ($(MINIMAL),)
TARGET_NAMES += bmpblk_font bmpblk_utility eficompress efidecompress
else
TARGET_NAMES += mount-encrypted
endif

TARGET_BINS = $(addprefix ${BUILD_ROOT}/,$(TARGET_NAMES))
ALL_DEPS = $(addsuffix .d,${TARGET_BINS})

all: $(TARGET_BINS) $(DUMPKERNELCONFIGLIB)

${BUILD_ROOT}/crossystem: crossystem_main.c $(LIBS)
	$(CC) $(CFLAGS) $(AU_LDFLAGS) $< -o $@ $(LIBS)

${BUILD_ROOT}/gbb_utility: gbb_utility.cc
	$(CXX) -DWITH_UTIL_MAIN $(CFLAGS) $(AU_LDFLAGS) $< -o $@

${BUILD_ROOT}/dump_fmap: dump_fmap.c $(LIBS)
	$(CC) $(CFLAGS) $(AU_LDFLAGS) $< -o $@ $(LIBS)

${BUILD_ROOT}/dumpRSAPublicKey: dumpRSAPublicKey.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(CRYPTO_LIBS)

${BUILD_ROOT}/dump_kernel_config: dump_kernel_config_main.c \
                                  $(DUMPKERNELCONFIGLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ \
		$(LIBS) $(DUMPKERNELCONFIGLIB) $(CRYPTO_LIBS)

${BUILD_ROOT}/dump_kernel_config.o: dump_kernel_config.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DUMPKERNELCONFIGLIB): ${BUILD_ROOT}/dump_kernel_config.o
	rm -f $@
	ar qc $@ $^

${BUILD_ROOT}/bmpblk_utility.o: bmpblk_utility.cc
	$(CXX) -DWITH_UTIL_MAIN $(CFLAGS) -c $< -o $@

${BUILD_ROOT}/bmpblk_util.o: bmpblk_util.c
	$(CC) $(CFLAGS) -c $< -o $@

${BUILD_ROOT}/bmpblk_font.o: bmpblk_font.c
	$(CC) $(CFLAGS) -c $< -o $@

${BUILD_ROOT}/image_types.o: image_types.c
	$(CC) $(CFLAGS) -c $< -o $@

${BUILD_ROOT}/eficompress.o: eficompress.c
	$(CC) $(CFLAGS) -c $< -o $@

${BUILD_ROOT}/eficompress: eficompress.c
	$(CC) $(CFLAGS) -DSTANDALONE $(LDFLAGS) $< -o $@

${BUILD_ROOT}/efidecompress.o: efidecompress.c
	$(CC) $(CFLAGS) -c $< -o $@

${BUILD_ROOT}/efidecompress: efidecompress.c
	$(CC) $(CFLAGS) -DSTANDALONE $(LDFLAGS) $< -o $@

${BUILD_ROOT}/bmpblk_utility: ${BUILD_ROOT}/bmpblk_utility.o \
				${BUILD_ROOT}/bmpblk_util.o \
				${BUILD_ROOT}/image_types.o \
				${BUILD_ROOT}/eficompress.o \
				${BUILD_ROOT}/efidecompress.o
	$(CXX) $(CFLAGS) $^ -o $@ -llzma -lyaml

${BUILD_ROOT}/bmpblk_font: ${BUILD_ROOT}/bmpblk_font.o \
				${BUILD_ROOT}/image_types.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# TODO: rewrite load_firmware_test to support new wrapper API
#${BUILD_ROOT}/load_firmware_test: load_firmware_test.c $(LIBS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/load_kernel_test: load_kernel_test.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/pad_digest_utility: pad_digest_utility.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/signature_digest_utility: signature_digest_utility.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/tlcl_generator: tlcl_generator.c
	$(HOSTCC) $(CFLAGS) -fpack-struct $(LDFLAGS) $< -o $@

${BUILD_ROOT}/vbutil_ec: vbutil_ec.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/vbutil_firmware: vbutil_firmware.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/vbutil_kernel: vbutil_kernel.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/vbutil_key: vbutil_key.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/vbutil_keyblock: vbutil_keyblock.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/verify_data: verify_data.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/sign_image: sign_image.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

${BUILD_ROOT}/tpm_init_temp_fix: tpm_init_temp_fix.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS)

${BUILD_ROOT}/tpm_set_readsrkpub: tpm_set_readsrkpub.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ -ltspi

${BUILD_ROOT}/dev_make_keypair: dev_make_keypair
	cp -f $< $@
	chmod +x $@

${BUILD_ROOT}/dev_debug_vboot: dev_debug_vboot
	cp -f $< $@
	chmod +x $@

${BUILD_ROOT}/vbutil_what_keys: vbutil_what_keys
	cp -f $< $@
	chmod +x $@

${BUILD_ROOT}/tpmc: tpmc.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS)

${BUILD_ROOT}/mount-helpers.o: mount-helpers.c mount-helpers.h mount-encrypted.h
	$(CC) -Wall -Werror -O2 -D_FORTIFY_SOURCE=2 -fstack-protector \
		$(shell $(PKG_CONFIG) --cflags glib-2.0 openssl) \
		-c $< -o $@

# The embedded libcrypto includes conflict with the shipped openssl,
# so this builds without the common CFLAGS (and those includes).
${BUILD_ROOT}/mount-encrypted: mount-encrypted.c mount-encrypted.h \
				${BUILD_ROOT}/mount-helpers.o $(LIBS)
	$(CC) -Wall -Werror -O2 -D_FORTIFY_SOURCE=2 -fstack-protector \
		$(shell $(PKG_CONFIG) --cflags glib-2.0 openssl) \
		-I$(FWDIR)/include \
		-I$(HOSTDIR)/include \
		$(LDFLAGS) \
		$< -o $@ $(shell $(PKG_CONFIG) --libs glib-2.0 openssl) \
		-lm \
		${BUILD_ROOT}/mount-helpers.o $(LIBS)

${BUILD_ROOT}/dev_sign_file: dev_sign_file.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LIBS) $(CRYPTO_LIBS)

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
