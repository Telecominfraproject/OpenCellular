#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate test cases for use for the RSA verify benchmark.

set -e

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Use a different directory for fuzzing test cases.
TESTKEY_DIR=${TESTKEY_DIR:-$(realpath  ${SCRIPT_DIR}/../tests/testkeys)}
TESTCASE_DIR=${BUILD_DIR}/fuzz_testcases
TEST_IMAGE_FILE=${TESTCASE_DIR}/testimage
TEST_IMAGE_SIZE=500000
TEST_BOOTLOADER_FILE=${TESTCASE_DIR}/testbootloader
TEST_BOOTLOADER_SIZE=50000
TEST_CONFIG_FILE=${TESTCASE_DIR}/testconfig
# Config size must < 4096
TEST_CONFIG_SIZE=3000

function generate_fuzzing_images {
  echo "Generating key blocks..."
  # Firmware key block - RSA8192/SHA512 root key, RSA4096/SHA512 firmware
  # signing key.
  ${FUTILITY} vbutil_keyblock \
    --pack ${TESTCASE_DIR}/firmware.keyblock \
    --datapubkey ${TESTKEY_DIR}/key_rsa4096.sha512.vbpubk \
    --signprivate ${TESTKEY_DIR}/key_rsa8192.sha1.vbprivk

  # Kernel key block - RSA4096/SHA512 kernel signing subkey, RSA4096/SHA512
  # kernel signing key.
  ${FUTILITY} vbutil_keyblock \
    --pack ${TESTCASE_DIR}/kernel.keyblock \
    --datapubkey ${TESTKEY_DIR}/key_rsa4096.sha512.vbpubk \
    --signprivate ${TESTKEY_DIR}/key_rsa4096.sha1.vbprivk \
    --flags 15

  echo "Generating signed firmware test image..."
  ${FUTILITY} vbutil_firmware \
    --vblock ${TESTCASE_DIR}/firmware.vblock \
    --keyblock ${TESTCASE_DIR}/firmware.keyblock\
    --signprivate ${TESTKEY_DIR}/key_rsa4096.sha256.vbprivk \
    --version 1 \
    --fv  $1 \
    --kernelkey ${TESTKEY_DIR}/key_rsa4096.sha512.vbpubk
  # TODO(gauravsh): ALso test with (optional) flags.
  cp ${TESTKEY_DIR}/key_rsa8192.sha512.vbpubk ${TESTCASE_DIR}/root_key.vbpubk

  echo "Generating signed kernel test image..."
  ${FUTILITY} vbutil_kernel \
    --pack ${TESTCASE_DIR}/kernel.vblock.image \
    --keyblock ${TESTCASE_DIR}/kernel.keyblock \
    --signprivate ${TESTKEY_DIR}/key_rsa4096.sha256.vbprivk \
    --version 1 \
    --vmlinuz ${TEST_IMAGE_FILE} \
    --bootloader ${TEST_BOOTLOADER_FILE} \
    --config ${TEST_CONFIG_FILE}
  # TODO(gauravsh): Also test with (optional) padding.
  cp ${TESTKEY_DIR}/key_rsa4096.sha512.vbpubk \
    ${TESTCASE_DIR}/firmware_key.vbpubk
}

function pre_work {
  # Generate a file to serve as random bytes for firmware/kernel contents.
  # NOTE: The kernel and config file can't really be random, but the bootloader
  # can. That's probably close enough.
  echo "Generating test image file..."
  dd if=/dev/urandom of=${TEST_IMAGE_FILE} bs=${TEST_IMAGE_SIZE} count=1
  echo "Generating test bootloader file..."
  # TODO(gauravsh): Use a valid bootloader here?
  dd if=/dev/urandom of=${TEST_BOOTLOADER_FILE} bs=${TEST_BOOTLOADER_SIZE} \
    count=1
  echo "Generating test config file..."
  # TODO(gauravsh): Use a valid config file here?
  dd if=/dev/urandom of=${TEST_CONFIG_FILE} bs=${TEST_CONFIG_SIZE} count=1
}

mkdir -p ${TESTCASE_DIR}
pre_work
check_test_keys
generate_fuzzing_images ${TEST_IMAGE_FILE}

