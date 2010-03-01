#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run verified boot firmware and kernel verification tests. 

return_code=0
hash_algos=( sha1 sha256 sha512 )
key_lengths=( 1024 2048 4096 8192 ) 
TEST_FILE=test_file 
TEST_FILE_SIZE=1000000

COL_RED='\E[31;1m'
COL_GREEN='\E[32;1m'
COL_YELLOW='\E[33;1m'
COL_BLUE='\E[34;1m'
COL_STOP='\E[0;m'

function test_firmware_verification {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      echo -e "For Root key ${COL_YELLOW}RSA-$keylen/$hashalgo${COL_STOP}:"
      cd ${UTIL_DIR} && ${TEST_DIR}/firmware_image_tests $algorithmcounter \
        ${TEST_DIR}/testkeys/key_rsa8192.pem \
        ${TEST_DIR}/testkeys/key_rsa8192.keyb \
        ${TEST_DIR}/testkeys/key_rsa${keylen}.pem \
        ${TEST_DIR}/testkeys/key_rsa${keylen}.keyb
      if [ $? -ne 0 ]
      then
        return_code=255
      fi
      let algorithmcounter=algorithmcounter+1
    done
  done
}

function test_kernel_verification {
# Test for various combinations of firmware signing algorithm and
# kernel signing algorithm
  firmware_algorithmcounter=0
  kernel_algorithmcounter=0
  for firmware_keylen in ${key_lengths[@]}
  do
    for firmware_hashalgo in ${hash_algos[@]}
    do
      let kernel_algorithmcounter=0
      for kernel_keylen in ${key_lengths[@]}
      do
        for kernel_hashalgo in ${hash_algos[@]}
        do
          echo -e "For ${COL_YELLOW}Firmware signing algorithm \
RSA-${firmware_keylen}/${firmware_hashalgo}${COL_STOP} \
and ${COL_YELLOW}Kernel signing algorithm RSA-${kernel_keylen}/\
${kernel_hashalgo}${COL_STOP}"
          cd ${UTIL_DIR} && ${TEST_DIR}/kernel_image_tests \
            $firmware_algorithmcounter $kernel_algorithmcounter \
            ${TEST_DIR}/testkeys/key_rsa${firmware_keylen}.pem \
            ${TEST_DIR}/testkeys/key_rsa${firmware_keylen}.keyb \
            ${TEST_DIR}/testkeys/key_rsa${kernel_keylen}.pem \
            ${TEST_DIR}/testkeys/key_rsa${kernel_keylen}.keyb
          if [ $? -ne 0 ]
          then
            return_code=255
          fi
          let kernel_algorithmcounter=kernel_algorithmcounter+1
        done
      done
      let firmware_algorithmcounter=firmware_algorithmcounter+1
    done
  done
}

# Determine script directory.
if [[ $0 == '/'* ]]; 
then
  SCRIPT_DIR="`dirname $0`"
elif [[ $0 == './'* ]];
then
  SCRIPT_DIR="`pwd`"
else
  SCRIPT_DIR="`pwd`"/"`dirname $0`"
fi
UTIL_DIR=`dirname ${SCRIPT_DIR}`/utils
KEY_DIR=${SCRIPT_DIR}/testkeys
TEST_DIR=${SCRIPT_DIR}/

echo
echo "Testing high-level firmware image verification..."
test_firmware_verification

echo
echo "Testing high-level kernel image verification..."
test_kernel_verification

exit $return_code
