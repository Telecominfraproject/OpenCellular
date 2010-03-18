#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate test cases for use for the RSA verify benchmark.

TESTCASE_DIR=fuzz_testcases
TESTKEY_DIR=testkeys
UTIL_DIR=../utils/
TEST_FILE=test_file 
TEST_FILE_SIZE=1000000

hash_algos=( sha1 sha256 sha512 )
key_lengths=( 1024 2048 4096 8192 ) 

# Generate public key signatures and digest on an input file for 
# various combinations of message digest algorithms and RSA key sizes.
function generate_fuzzing_images {
  echo "Generating signed firmware test image..."
  # Generate a test verified boot firmware image and copy root public key.
  ${UTIL_DIR}/firmware_utility --generate \
    --in  $1 \
    --root_key ${TESTKEY_DIR}/key_rsa8192.pem \
    --firmware_sign_key ${TESTKEY_DIR}/key_rsa4096.pem \
    --firmware_sign_key_pub ${TESTKEY_DIR}/key_rsa4096.keyb \
    --firmware_sign_algorithm 8 \
    --firmware_key_version 1 \
    --firmware_version 1 \
    --out ${TESTCASE_DIR}/firmware.signed
  cp ${TESTKEY_DIR}/key_rsa8192.keyb ${TESTCASE_DIR}/root_key.keyb

  echo "Generating signed kernel test image..."
  # Generate a test verified boot kernel image and copy firmware public key.
  ${UTIL_DIR}/kernel_utility --generate \
    --in $1 \
    --firmware_key ${TESTKEY_DIR}/key_rsa4096.pem \
    --kernel_key ${TESTKEY_DIR}/key_rsa1024.pem \
    --kernel_key_pub ${TESTKEY_DIR}/key_rsa1024.keyb \
    --firmware_sign_algorithm 8 \
    --kernel_sign_algorithm 2 \
    --kernel_key_version 1 \
    --kernel_version 1 \
    --out ${TESTCASE_DIR}/kernel.signed 
  cp ${TESTKEY_DIR}/key_rsa4096.keyb ${TESTCASE_DIR}/firmware_key.keyb
}

function pre_work {
  # Generate a file with random bytes for signature tests.
  echo "Generating test file..."
  dd if=/dev/urandom of=${TESTCASE_DIR}/${TEST_FILE} bs=${TEST_FILE_SIZE} \
    count=1
}

if [ ! -d ${TESTKEY_DIR} ]
then
  echo "You must run gen_test_keys.sh to generate test keys first."
  exit 1
fi

if [ ! -d ${TESTCASE_DIR} ]
then
  mkdir ${TESTCASE_DIR}
fi

pre_work
generate_fuzzing_images ${TESTCASE_DIR}/$TEST_FILE
