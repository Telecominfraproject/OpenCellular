#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate test cases for use for the RSA verify benchmark.

KEY_DIR=testkeys
TESTCASE_DIR=testcases
UTIL_DIR=../utils/
TEST_FILE=test_file 
TEST_FILE_SIZE=1000000

hash_algos=( sha1 sha256 sha512 )
key_lengths=( 1024 2048 4096 8192 ) 

# Generate public key signatures and digest on an input file for 
# various combinations of message digest algorithms and RSA key sizes.
function generate_test_signatures {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      openssl dgst -${hashalgo} -binary -out $1.${hashalgo}.digest $1
      ${UTIL_DIR}/signature_digest $algorithmcounter $1 | openssl rsautl -sign \
        -pkcs -inkey ${KEY_DIR}/key_rsa${keylen}.pem \
        > $1.rsa${keylen}_${hashalgo}.sig
      let algorithmcounter=algorithmcounter+1
    done
  done
}

function pre_work {
  # Generate a file with random bytes for signature tests.
  echo "Generating test file..."
  dd if=/dev/urandom of=${TESTCASE_DIR}/${TEST_FILE} bs=${TEST_FILE_SIZE} count=1
}

if [ ! -d "$KEY_DIR" ]
then
  echo "You must run gen_test_cases.sh to generate test keys first."
  exit 1
fi

if [ ! -d "$TESTCASE_DIR" ]
then
  mkdir  "$TESTCASE_DIR"
fi

pre_work
echo "Generating test signatures..."
generate_test_signatures ${TESTCASE_DIR}/$TEST_FILE
