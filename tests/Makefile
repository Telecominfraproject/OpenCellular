# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDES += -I./include \
		-I$(FWDIR)/lib/include \
		-I$(FWDIR)/lib/cgptlib/include \
		-I$(FWDIR)/lib/cryptolib/include \
		-I$(FWDIR)/lib/tpm_lite/include \
		-I$(HOSTDIR)/include
BUILD_ROOT = ${BUILD}/tests

TEST_NAMES = cgptlib_test \
	     rollback_index2_tests \
	     rsa_padding_test \
	     rsa_utility_tests \
	     rsa_verify_benchmark \
	     sha_benchmark \
	     sha_tests \
	     stateful_util_tests \
	     utility_string_tests \
	     utility_tests \
	     tpm_bootmode_tests \
	     vboot_common_tests \
	     vboot_common2_tests \
	     vboot_common3_tests \
	     vboot_nvstorage_test

TEST_BINS = $(addprefix ${BUILD_ROOT}/,$(TEST_NAMES))

TEST_LIB = ${BUILD_ROOT}/test.a
TEST_LIB_SRCS = test_common.c timer_utils.c crc32_test.c
TEST_LIB_OBJS = $(TEST_LIB_SRCS:%.c=${BUILD_ROOT}/%.o)
ALL_DEPS = $(addsuffix .d,${TEST_BINS} ${TEST_LIB_OBJS})
# Allow multiple definitions, so tests can mock functions from other libraries
CFLAGS +=  -MMD -MF $@.d -Xlinker --allow-multiple-definition

LIBS := ${TEST_LIB} $(HOSTLIB)

ifneq (${RUNTESTS},)
EXTRA_TARGET = runtests
endif

all: $(TEST_BINS) ${EXTRA_TARGET}

rbtest: $(BUILD_ROOT)/rollback_index_test

${TEST_LIB}: ${TEST_LIB_OBJS}
	rm -f $@
	ar qc $@ $^

${BUILD_ROOT}/rollback_index_test.o : rollback_index_test.c
	$(CC) $(CFLAGS) -I/usr/include $(INCLUDES) -MMD -MF $@.d -c -o $@ $<

${BUILD_ROOT}/rollback_index_test: rollback_index_test.c ${HOSTLIB}
	$(CC) $(CFLAGS) -I/usr/include $(INCLUDES) $< -o $@ \
        -ltlcl ${HOSTLIB} -lcrypto -lrt

# Compile rollback_index.c for unit test, so it uses the same implementation
# as it does in the firmware.
${BUILD_ROOT}/rollback_index_for_test.o : $(FWDIR)/lib/rollback_index.c
	$(CC) $(CFLAGS) -DROLLBACK_UNITTEST $(INCLUDES) \
	-MMD -MF $@.d -c -o $@ $<

${BUILD_ROOT}/rollback_index2_tests: rollback_index2_tests.c \
		${BUILD_ROOT}/rollback_index_for_test.o ${LIBS}
	$(CC) $(CFLAGS) $(INCLUDES) $< ${BUILD_ROOT}/rollback_index_for_test.o \
	${LIBS} -o $@ -lcrypto -lrt

${BUILD_ROOT}/%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MF $@.d -c -o $@ $<

${BUILD_ROOT}/%: %.c ${LIBS}
	$(CC) $(CFLAGS) $(INCLUDES) $< ${LIBS} -o $@ -lcrypto -lrt

# TODO: port these tests to new API, if not already eqivalent
# functionality in other tests
#
#		big_firmware_tests
#		firmware_image_tests
#		firmware_rollback_tests
#		firmware_splicing_tests
#		firmware_verify_benchmark
#		verify_firmware_fuzz_driver
#
#		big_kernel_tests
#		kernel_image_tests
#		kernel_rollback_tests
#		kernel_splicing_tests
#		kernel_verify_benchmark
#		verify_kernel_fuzz_driver

# Generate test keys
genkeys:
	./gen_test_keys.sh

# Run cgpt tests
runcgpttests:
	${BUILD_ROOT}/cgptlib_test
	./run_cgpt_tests.sh  ${BUILD}/cgpt/cgpt

# Run crypto tests
runcryptotests:
	./run_rsa_tests.sh
	${BUILD_ROOT}/rsa_utility_tests
	${BUILD_ROOT}/sha_tests
	./run_vboot_common_tests.sh

# Run other misc tests
runmisctests:
	./run_vbutil_tests.sh
	${BUILD_ROOT}/rollback_index2_tests
	${BUILD_ROOT}/stateful_util_tests
	${BUILD_ROOT}/tpm_bootmode_tests
	${BUILD_ROOT}/utility_string_tests
	${BUILD_ROOT}/utility_tests

#This will exercise vbutil_kernel and vbutil_firmware
runfuzztests:
	./gen_fuzz_test_cases.sh

# Run bmpblk_utility tests
runbmptests:
	$(MAKE) BUILD=$(shell readlink -f ${BUILD}) -C bitmaps runtests

ALLTESTS=runcgpttests runcryptotests runmisctests runfuzztests runbmptests

runtests: genkeys ${ALLTESTS}

# TODO: tests to run when ported to new API
#	./run_image_verification_tests.sh
#	# Splicing tests
#	${BUILD_ROOT}/firmware_splicing_tests
#	${BUILD_ROOT}/kernel_splicing_tests
#	# Rollback Tests
#	${BUILD_ROOT}/firmware_rollback_tests
#	${BUILD_ROOT}/kernel_rollback_tests

install-rbtest: $(BUILD_ROOT)/rollback_index_test
	mkdir -p $(DESTDIR)
	cp -f $(BUILD_ROOT)/rollback_index_test $(DESTDIR)

-include ${ALL_DEPS}
