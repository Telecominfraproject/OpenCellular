# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
INCLUDES ?= -I../include/
TOP ?= ../

FIRMWARE_LIBS = $(TOP)/crypto/libcrypto.a $(TOP)/common/libcommon.a
LIBS = $(TOP)/utils/kernel_image.o $(TOP)/utils/firmware_image.o \
	$(TOP)/utils/file_keys.o $(TOP)/utils/signature_digest.o -lcrypto

tests: firmware_image_tests verify_firmware_fuzz_driver \
	kernel_image_tests verify_kernel_fuzz_driver \
	sha_tests sha_benchmark rsa_verify_benchmark rsa_padding_test

sha_tests: sha_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(FIRMWARE_LIBS)

firmware_image_tests: firmware_image_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARE_LIBS)

verify_firmware_fuzz_driver: verify_firmware_fuzz_driver.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARE_LIBS)

kernel_image_tests: kernel_image_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARE_LIBS)

verify_kernel_fuzz_driver: verify_kernel_fuzz_driver.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS) $(FIRMWARE_LIBS)

sha_benchmark:	sha_benchmark.c timer_utils.c $(FIRMWARE_LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt

rsa_padding_test: rsa_padding_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS) $(FIRMWARE_LIBS)

rsa_verify_benchmark: rsa_verify_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(LIBS) $(FIRMWARE_LIBS)

clean:
	rm -f sha_tests sha_benchmark rsa_verify_benchmark \
	firmware_image_tests kernel_image_tests rsa_padding_test \
	verify_firmware_fuzz_driver verify_kernel_fuzz_driver
