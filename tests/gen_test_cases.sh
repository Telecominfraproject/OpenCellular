#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate test cases for use for the RSA verify benchmark.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

TEST_FILE=${TESTCASE_DIR}/test_file
TEST_FILE_SIZE=1000000

# Generate public key signatures on an input file for various combinations
# of message digest algorithms and RSA key sizes.
function generate_test_signatures {
  echo "Generating test signatures..."
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      openssl dgst -${hashalgo} -binary ${TEST_FILE} > \
        ${TEST_FILE}.${hashalgo}.digest
      ${BIN_DIR}/signature_digest_utility $algorithmcounter  \
        ${TEST_FILE} | openssl rsautl \
        -sign -pkcs -inkey ${TESTKEY_DIR}/key_rsa${keylen}.pem \
        > ${TEST_FILE}.rsa${keylen}_${hashalgo}.sig
      let algorithmcounter=algorithmcounter+1
    done
  done
}

# Generate a file with random bytes for signature tests.
function generate_test_file {
  echo "Generating test file..."
  dd if=/dev/urandom of=${TEST_FILE} bs=${TEST_FILE_SIZE} count=1
}

mkdir -p ${TESTCASE_DIR}
check_test_keys
generate_test_file
generate_test_signatures
