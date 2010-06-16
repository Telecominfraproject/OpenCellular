# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I../host/include \
		-I../misclibs/include \
		-I../vboot_firmware/lib/include\
		-I../vkernel/include
IMAGE_LIBS = $(BUILD)/vkernel/kernel_image.o \
		$(BUILD)/vkernel/kernel_image_fw.o
UTIL_LIBS = $(BUILD)/misclibs/file_keys.o $(BUILD)/misclibs/signature_digest.o
LIBS = $(IMAGE_LIBS) $(UTIL_LIBS) $(HOSTLIB) $(FWLIB) -lcrypto
BUILD_ROOT = ${BUILD}/tests

TEST_NAMES = big_kernel_tests \
		cgptlib_test \
		kernel_image_tests \
		kernel_rollback_tests \
		kernel_splicing_tests \
		kernel_verify_benchmark \
		rsa_padding_test \
		rsa_verify_benchmark \
		sha_benchmark \
		sha_tests \
		vboot_common_tests \
		vboot_common2_tests \
		vboot_common3_tests \
		verify_kernel_fuzz_driver
TEST_BINS = $(addprefix ${BUILD_ROOT}/,$(TEST_NAMES))
ALL_DEPS = $(addsuffix .d,${TEST_BINS})

# TODO: port these tests to vboot_firmware, if not already eqivalent
# functionality
#		big_firmware_tests
#		firmware_image_tests
#		firmware_rollback_tests
#		firmware_splicing_tests
#		firmware_verify_benchmark
#		verify_firmware_fuzz_driver


ifneq (${RUNTESTS},)
EXTRA_TARGET = runtests
endif

all: $(TEST_BINS) ${EXTRA_TARGET}

${BUILD_ROOT}/big_firmware_tests: big_firmware_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/big_kernel_tests: big_kernel_tests.c rollback_index_mock.c \
	test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/cgptlib_test: cgptlib_test.c crc32_test.c
	$(CC) $(CFLAGS) -ansi $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/firmware_image_tests: firmware_image_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/firmware_rollback_tests: firmware_rollback_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/firmware_splicing_tests: firmware_splicing_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/firmware_verify_benchmark: firmware_verify_benchmark.c \
	timer_utils.c rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(LIBS)

${BUILD_ROOT}/kernel_image_tests: kernel_image_tests.c rollback_index_mock.c \
	test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/kernel_rollback_tests: kernel_rollback_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/kernel_splicing_tests: kernel_splicing_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/kernel_verify_benchmark: kernel_verify_benchmark.c timer_utils.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(LIBS)

${BUILD_ROOT}/rsa_padding_test: rsa_padding_test.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(UTIL_LIBS) $(FWLIB) \
	-lcrypto

${BUILD_ROOT}/rsa_verify_benchmark: rsa_verify_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(UTIL_LIBS) $(FWLIB) \
	-lcrypto

${BUILD_ROOT}/sha_benchmark:	sha_benchmark.c timer_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lrt $(FWLIB)

${BUILD_ROOT}/sha_tests: sha_tests.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(FWLIB)

${BUILD_ROOT}/vboot_common_tests: vboot_common_tests.c \
	rollback_index_mock.c test_common.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/vboot_common2_tests: vboot_common2_tests.c \
	test_common.c $(HOSTLIB) $(FWLIB)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/vboot_common3_tests: vboot_common3_tests.c \
	test_common.c $(HOSTLIB) $(FWLIB)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/verify_firmware_fuzz_driver: verify_firmware_fuzz_driver.c \
	rollback_index_mock.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

${BUILD_ROOT}/verify_kernel_fuzz_driver: verify_kernel_fuzz_driver.c \
	rollback_index_mock.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

runtests:
	./gen_test_keys.sh
	# Crypto tests
	./run_rsa_tests.sh
	${BUILD_ROOT}/sha_tests
	./run_vbutil_tests.sh
	./run_vboot_common_tests.sh
	./run_image_verification_tests.sh
	# Splicing tests
	#${BUILD_ROOT}/firmware_splicing_tests
	${BUILD_ROOT}/kernel_splicing_tests
	# Rollback Tests
	#${BUILD_ROOT}/firmware_rollback_tests
	${BUILD_ROOT}/kernel_rollback_tests
	# Helper Library Tests
	${BUILD_ROOT}/cgptlib_test
	# Tool tests
	./run_cgpt_tests.sh  ${BUILD}/cgpt/cgpt
