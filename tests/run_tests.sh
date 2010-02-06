#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run tests for cryptographic routine implementations - Message digests 
# and RSA Signature verification.

hash_algos=( sha1 sha256 sha512 )
key_lengths=( 1024 2048 4096 8192 ) 
TEST_FILE=test_file 
TEST_FILE_SIZE=1000000
UTIL_DIR=../utils/

# Generate RSA test keys of various lengths. 
function generate_keys {
  for i in ${key_lengths[@]}
  do
    openssl genrsa -F4 -out key_rsa$i.pem $i
    # Generate self-signed certificate from key.
    openssl req -batch -new -x509 -key key_rsa$i.pem -out key_rsa$i.crt
    # Generate pre-processed key for use by RSA signature verification code.
    ${UTIL_DIR}/dumpRSAPublicKey key_rsa$i.crt > key_rsa$i.keyb
  done
}

# Generate public key signatures on an input file for various combinations
# of message digest algorithms and RSA key sizes.
function generate_signatures {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      ${UTIL_DIR}/signature_digest $algorithmcounter $1 | openssl rsautl -sign \
      -pkcs -inkey key_rsa${keylen}.pem > $1.rsa${keylen}\_${hashalgo}.sig
      let algorithmcounter=algorithmcounter+1
    done
  done
}

function test_signatures {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      echo "For RSA-$keylen and $hashalgo:"
      ${UTIL_DIR}/verify_data $algorithmcounter key_rsa${keylen}.keyb \
        ${TEST_FILE}.rsa${keylen}\_${hashalgo}.sig ${TEST_FILE}
      let algorithmcounter=algorithmcounter+1
    done
  done
}

function pre_work {
  # Generate a file with random bytes for signature tests.
  echo "Generating test file..."
  dd if=/dev/urandom of=${TEST_FILE} bs=${TEST_FILE_SIZE} count=1
  echo "Generating test keys..."
  generate_keys
  echo "Generating signatures..."
  generate_signatures $TEST_FILE
}

function cleanup {
  rm ${TEST_FILE} ${TEST_FILE}.*.sig key_rsa*.*
}

echo "Testing message digests..."
./sha_tests

echo
echo "Testing signature verification..."
pre_work
test_signatures

echo
echo "Cleaning up..."
cleanup


