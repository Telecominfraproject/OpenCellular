# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

VBOOT_SRC_DIR ?= ../../../..
BUILD_TEST_DIR = vboot_tests
# Test Binaries.
TEST_BINS = \
	cgptlib_test \
	rsa_padding_test \
	rsa_verify_benchmark \
	sha_benchmark \
	sha_tests \
	vboot_common_tests \
	vboot_common2_tests \
	vboot_common3_tests
# Test Runner Scripts.
TEST_SCRIPTS = \
	common.sh \
	gen_test_cases.sh \
	gen_test_keys.sh \
	gen_test_vbpubks.sh \
	run_cgpt_tests.sh \
	run_rsa_tests.sh \
	run_vboot_common_tests.sh \
	run_vbutil_tests.sh \
	testcases \
	testkeys

# Utility Binaries.
UTIL_BINS = \
	load_kernel_test \
	signature_digest_utility \
	vbutil_firmware \
	vbutil_kernel \
	vbutil_key \
	vbutil_keyblock \
	verify_data

all: 
        # Copy test sources into a temporary directory for building.
	rm -rf $(BUILD_TEST_DIR)
	mkdir -p $(BUILD_TEST_DIR)
        # Ignore autotest and build directory.
	sh -c \
	'for dir in $(VBOOT_SRC_DIR)/*; do \
		[ "$${dir}" != "$(VBOOT_SRC_DIR)/build" ] && \
		[ "$${dir}" != "$(VBOOT_SRC_DIR)/autotest" ] && \
    		        cp -r "$${dir}" "$(BUILD_TEST_DIR)"; \
	done'
        # Always build from scratch.
	$(MAKE) -C $(BUILD_TEST_DIR) clean all
        # Move test binaries for use by the tests.
	mkdir -p build/tests
	set -e; for i in $(TEST_BINS); do \
	  mv $(BUILD_TEST_DIR)/build/tests/$$i build/tests/ ;\
	done
        # Move test scripts for use by the tests.
	mkdir -p tests
	set -e; for i in $(TEST_SCRIPTS); do \
	  mv $(BUILD_TEST_DIR)/tests/$$i tests/ ;\
	done
        # Move utility binaries used by the tests.
	mkdir -p build/utility
	set -e; for i in $(UTIL_BINS); do \
	  mv $(BUILD_TEST_DIR)/build/utility/$$i build/utility/ ;\
	done
        # Delete sources.
	rm -rf $(BUILD_TEST_DIR)

clean:
	rm -rf utility/ tests/
