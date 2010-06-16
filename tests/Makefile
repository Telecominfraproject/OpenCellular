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

TEST_LIB = ${BUILD_ROOT}/test.a
TEST_LIB_SRCS = rollback_index_mock.c test_common.c timer_utils.c crc32_test.c
TEST_LIB_OBJS = $(TEST_LIB_SRCS:%.c=${BUILD_ROOT}/%.o)
ALL_DEPS = $(addsuffix .d,${TEST_BINS} ${TEST_LIB_OBJS})
CFLAGS +=  -MMD -MF $@.d

LIBS := ${TEST_LIB} $(IMAGE_LIBS) $(UTIL_LIBS) $(HOSTLIB) $(FWLIB)

all: $(TEST_BINS) ${EXTRA_TARGET}

${TEST_LIB}: ${TEST_LIB_OBJS}
	rm -f $@
	ar qc $@ $^

${BUILD_ROOT}/%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MF $@.d -c -o $@ $<

${BUILD_ROOT}/%: %.c ${LIBS}
	$(CC) $(CFLAGS) $(INCLUDES) $< ${LIBS} -o $@ -lcrypto -lrt

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

-include ${ALL_DEPS}