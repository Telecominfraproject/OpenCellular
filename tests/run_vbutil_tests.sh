#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run verified boot firmware and kernel verification tests.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

return_code=0

function test_vbutil_key {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      echo -e "For signing key ${COL_YELLOW}RSA-$keylen/$hashalgo${COL_STOP}:"
      # Pack the key
      ${UTIL_DIR}/vbutil_key --pack \
        --in ${TESTKEY_DIR}/key_rsa${keylen}.keyb \
        --out ${TESTKEY_SCRATCH_DIR}/key_alg${algorithmcounter}.vbpubk \
        --version 1 \
        --algorithm $algorithmcounter
      if [ $? -ne 0 ]
      then
        return_code=255
      fi

      # Unpack the key
      # TODO: should verify we get the same key back out?
      ${UTIL_DIR}/vbutil_key --unpack \
        --in ${TESTKEY_SCRATCH_DIR}/key_alg${algorithmcounter}.vbpubk
      if [ $? -ne 0 ]
      then
        return_code=255
      fi

      let algorithmcounter=algorithmcounter+1
    done
  done
}


function test_vbutil_keyblock {
# Test for various combinations of firmware signing algorithm and
# kernel signing algorithm
  signing_algorithmcounter=0
  data_algorithmcounter=0
  for signing_keylen in ${key_lengths[@]}
  do
    for signing_hashalgo in ${hash_algos[@]}
    do
      let data_algorithmcounter=0
      for datakeylen in ${key_lengths[@]}
      do
        for datahashalgo in ${hash_algos[@]}
        do
          echo -e "For ${COL_YELLOW}signing algorithm \
RSA-${signing_keylen}/${signing_hashalgo}${COL_STOP} \
and ${COL_YELLOW}data key algorithm RSA-${datakeylen}/\
${datahashalgo}${COL_STOP}"
          # Remove old file
          keyblockfile="${TESTKEY_SCRATCH_DIR}/"
          keyblockfile+="sign${signing_algorithmcounter}_data"
          keyblockfile+="${data_algorithmcounter}.keyblock"
          rm -f ${keyblockfile}

          # Pack
          ${UTIL_DIR}/vbutil_keyblock --pack ${keyblockfile} \
            --datapubkey \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algorithmcounter}.vbpubk \
            --signprivate ${TESTKEY_DIR}/key_rsa${signing_keylen}.pem \
            --algorithm $signing_algorithmcounter
          if [ $? -ne 0 ]
          then
            return_code=255
          fi

          # Unpack
          ${UTIL_DIR}/vbutil_keyblock --unpack ${keyblockfile} \
            --signpubkey \
            ${TESTKEY_SCRATCH_DIR}/key_alg${signing_algorithmcounter}.vbpubk
          # TODO: check data key against the packed one?
          if [ $? -ne 0 ]
          then
            return_code=255
          fi

          let data_algorithmcounter=data_algorithmcounter+1
        done
      done
      let signing_algorithmcounter=signing_algorithmcounter+1
    done
  done
}


check_test_keys

echo
echo "Testing vbutil_key..."
test_vbutil_key

echo
echo "Testing vbutil_keyblock..."
test_vbutil_keyblock


exit $return_code

