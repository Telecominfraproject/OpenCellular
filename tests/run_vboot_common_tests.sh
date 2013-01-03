#!/bin/bash

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run verified boot firmware and kernel verification tests.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

return_code=0

function test_vboot_common {
  ${TEST_DIR}/vboot_common_tests
  if [ $? -ne 0 ]
  then
    return_code=255
  fi
}

# Test a single key+hash algorithm
function test_vboot_common2_single {
    local algonum=$1
    local keylen=$2
    local hashalgo=$3
    echo -e "For signing key ${COL_YELLOW}RSA-$keylen/$hashalgo${COL_STOP}:"
    echo ${TEST_DIR}/vboot_common2_tests $algonum \
        ${TESTKEY_DIR}/key_rsa${keylen}.pem \
        ${TESTKEY_DIR}/key_rsa${keylen}.keyb
    ${TEST_DIR}/vboot_common2_tests $algonum \
        ${TESTKEY_DIR}/key_rsa${keylen}.pem \
        ${TESTKEY_DIR}/key_rsa${keylen}.keyb
    if [ $? -ne 0 ]
    then
        return_code=255
    fi
}

# Test all key+hash algorithms
function test_vboot_common2_all {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      test_vboot_common2_single $algorithmcounter $keylen $hashalgo
      let algorithmcounter=algorithmcounter+1
    done
  done
}

# Test only the algorithms we actually use
function test_vboot_common2 {
    test_vboot_common2_single 4 2048 sha256
    test_vboot_common2_single 7 4096 sha256
    test_vboot_common2_single 11 8192 sha512
}

# Test a single block algorithm + data algorithm
function test_vboot_common3_single {
    local signing_algonum=$1
    local signing_keylen=$2
    local signing_hashalgo=$3
    local data_algonum=$4
    local data_keylen=$5
    local data_hashalgo=$6

    echo -e "For ${COL_YELLOW}signing algorithm \
RSA-${signing_keylen}/${signing_hashalgo}${COL_STOP} \
and ${COL_YELLOW}data signing algorithm RSA-${data_keylen}/\
${data_hashalgo}${COL_STOP}"
    ${TEST_DIR}/vboot_common3_tests \
        $signing_algonum $data_algonum \
        ${TESTKEY_DIR}/key_rsa${signing_keylen}.pem \
        ${TESTKEY_DIR}/key_rsa${signing_keylen}.keyb \
        ${TESTKEY_DIR}/key_rsa${data_keylen}.pem \
        ${TESTKEY_DIR}/key_rsa${data_keylen}.keyb
    if [ $? -ne 0 ]
    then
        return_code=255
    fi
}

# Test all combinations of key block signing algorithm and data signing
# algorithm
function test_vboot_common3_all {
  signing_algorithmcounter=0
  data_algorithmcounter=0
  for signing_keylen in ${key_lengths[@]}
  do
    for signing_hashalgo in ${hash_algos[@]}
    do
      let data_algorithmcounter=0
      for data_keylen in ${key_lengths[@]}
      do
        for data_hashalgo in ${hash_algos[@]}
        do
            test_vboot_common3_single \
                $signing_algorithmcounter $signing_keylen $signing_hashalgo \
                $data_algorithmcounter $data_keylen $data_hashalgo
            let data_algorithmcounter=data_algorithmcounter+1
        done
      done
      let signing_algorithmcounter=signing_algorithmcounter+1
    done
  done
}

# Test only the combinations of key block signing algorithm and data signing
# algorithm that we actually use
function test_vboot_common3 {
    test_vboot_common3_single 7 4096 sha256 4 2048 sha256
    test_vboot_common3_single 11 8192 sha512 4 2048 sha256
    test_vboot_common3_single 11 8192 sha512 7 4096 sha256
}

check_test_keys
echo
echo "Testing vboot_common tests which don't depend on keys..."
test_vboot_common

echo
echo "Testing vboot_common tests which depend on one key..."
if [ "$1" == "--all" ] ; then
    test_vboot_common2_all
else
    test_vboot_common2
fi

echo
echo "Testing vboot_common tests which depend on two keys..."
if [ "$1" == "--all" ] ; then
    test_vboot_common3_all
else
    test_vboot_common3
fi

exit $return_code
