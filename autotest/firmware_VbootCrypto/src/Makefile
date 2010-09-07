# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

VBOOT_REFERENCE_DIR = ../../..
BUILD_TEST_DIR = vboot_reference
# Test Binaries.
TEST_BINS = \
	common.sh \
	firmware_image_tests \
	firmware_rollback_tests \
	firmware_splicing_tests \
	firmware_verify_benchmark \
	gen_test_cases.sh \
	kernel_image_tests \
	kernel_rollback_tests \
	kernel_splicing_tests \
	kernel_verify_benchmark \
	rsa_padding_test \
	rsa_verify_benchmark \
	run_image_verification_tests.sh \
	run_rsa_tests.sh \
	sha_benchmark \
	sha_tests \
	testcases \
	testkeys

# Utility Binaries.
UTIL_BINS = verify_data signature_digest_utility

all: 
        # Copy test sources into a temporary directory for building.
	rm -rf $(BUILD_TEST_DIR)
	# only copy stuff that's not us
	sh -c \
	'for dir in $(VBOOT_REFERENCE_DIR)/*; do \
		[ "$${dir}" != "$(VBOOT_REFERENCE_DIR)/autotest" ] && cp -r $${dir} .; \
	done'
        # Always build from scratch.
	$(MAKE) -C $(BUILD_TEST_DIR) clean all
        # Move test binaries for use by the tests.
	mkdir -p tests
	set -e; for i in $(TEST_BINS); do \
	  mv $(BUILD_TEST_DIR)/tests/$$i tests/ ;\
	done
        # Move utility binaries used by the tests.
	mkdir -p utility
	set -e; for i in $(UTIL_BINS); do \
	  mv $(BUILD_TEST_DIR)/utility/$$i utility/ ;\
	done
        # Delete sources.
	rm -rf $(BUILD_TEST_DIR)

clean:
	rm -rf utility/ tests/
