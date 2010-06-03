# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../
CC ?= gcc
CFLAGS ?= -Wall -DNDEBUG -O3 -Werror
INCLUDES += -I./include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I../misclibs/include \
		-I../vfirmware/include\
		-I../vkernel/include
IMAGE_LIBS = $(TOP)/vfirmware/firmware_image.o \
		$(TOP)/vkernel/kernel_image.o
UTIL_LIBS = $(TOP)/misclibs/file_keys.o $(TOP)/misclibs/signature_digest.o
LIBS = $(IMAGE_LIBS) $(UTIL_LIBS) $(FWLIB) -lcrypto

TEST_BINS = big_firmware_tests \
		big_kernel_tests \
		cgptlib_test \
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

cgptlib_test: cgptlib_test.c crc32_test.c
	$(CC) $(CFLAGS) -ansi $(INCLUDES) $^ -o $@ $(LIBS)

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
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(UTIL_LIBS) $(FWLIB) \
	-lcrypto

rsa_verify_benchmark: rsa_verify_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(UTIL_LIBS) $(FWLIB) \
	-lcrypto

sha_benchmark:	sha_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(FWLIB)

sha_tests: sha_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(FWLIB)

verify_firmware_fuzz_driver: verify_firmware_fuzz_driver.c \
	rollback_index_mock.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

verify_kernel_fuzz_driver: verify_kernel_fuzz_driver.c rollback_index_mock.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

runtests:
	# Crypto tests
	./run_rsa_tests.sh
	./sha_tests
	./run_image_verification_tests.sh
	# Splicing tests
	./firmware_splicing_tests
	./kernel_splicing_tests	
	# Rollback Tests
	./firmware_rollback_tests
	./kernel_rollback_tests
	# Helper Library Tests
	./cgptlib_test

clean:
	rm -f $(TEST_BINS)
