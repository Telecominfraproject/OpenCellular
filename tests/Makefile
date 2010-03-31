# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CC ?= gcc
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
INCLUDES += -I./include \
		-I../cryptolib/include \
		-I../common/include \
		-I../misclibs/include \
		-I../vfirmware/include\
		-I../vkernel/include
BASE_LIBS = $(TOP)/cryptolib/libcrypto.a $(TOP)/common/libcommon.a
IMAGE_LIBS = $(TOP)/vfirmware/firmware_image.o \
		$(TOP)/vfirmware/firmware_image_fw.o \
		$(TOP)/vkernel/kernel_image.o \
		$(TOP)/vkernel/kernel_image_fw.o
UTIL_LIBS = $(TOP)/misclibs/file_keys.o $(TOP)/misclibs/signature_digest.o
LIBS = $(IMAGE_LIBS) $(UTIL_LIBS) -lcrypto $(BASE_LIBS)

TEST_BINS = big_firmware_tests \
		big_kernel_tests \
		firmware_image_tests \
		firmware_rollback_tests \
		firmware_splicing_tests \
		firmware_verify_benchmark \
		kernel_image_tests \
		kernel_rollback_tests \
		kernel_splicing_tests \
		kernel_verify_benchmark \
		rsa_padding_test \
		rsa_verify_benchmark \
		sha_benchmark \
		sha_tests \
		verify_firmware_fuzz_driver \
		verify_kernel_fuzz_driver

all: $(TEST_BINS)

big_firmware_tests: big_firmware_tests.c rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

big_kernel_tests: big_kernel_tests.c rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

firmware_image_tests: firmware_image_tests.c rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

firmware_rollback_tests: firmware_rollback_tests.c rollback_index_mock.c \
	test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

firmware_splicing_tests: firmware_splicing_tests.c rollback_index_mock.c \
	test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

firmware_verify_benchmark: firmware_verify_benchmark.c timer_utils.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(LIBS)

kernel_image_tests: kernel_image_tests.c rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

kernel_rollback_tests: kernel_rollback_tests.c rollback_index_mock.c \
	test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

kernel_splicing_tests: kernel_splicing_tests.c rollback_index_mock.c \
	test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

kernel_verify_benchmark: kernel_verify_benchmark.c timer_utils.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(LIBS)

rsa_padding_test: rsa_padding_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(UTIL_LIBS) $(BASE_LIBS) \
	-lcrypto

rsa_verify_benchmark: rsa_verify_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(UTIL_LIBS) $(BASE_LIBS) \
	-lcrypto

sha_benchmark:	sha_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(BASE_LIBS)

sha_tests: sha_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(BASE_LIBS)

verify_firmware_fuzz_driver: verify_firmware_fuzz_driver.c \
	rollback_index_mock.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

verify_kernel_fuzz_driver: verify_kernel_fuzz_driver.c rollback_index_mock.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

clean:
	rm -f $(TEST_BINS)
