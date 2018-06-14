#!/bin/bash

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run verified boot firmware and kernel verification tests.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

return_code=0

function test_vbutil_key_single {
    local algonum=$1
    local keylen=$2
    local hashalgo=$3

    echo -e "For signing key ${COL_YELLOW}RSA-$keylen/$hashalgo${COL_STOP}:"
    # Pack the key
    ${FUTILITY} vbutil_key \
        --pack ${TESTKEY_SCRATCH_DIR}/key_alg${algonum}.vbpubk \
        --key ${TESTKEY_DIR}/key_rsa${keylen}.keyb \
        --version 1 \
        --algorithm $algonum
    if [ $? -ne 0 ]
    then
        return_code=255
    fi

    # Unpack the key
    # TODO: should verify we get the same key back out?
    ${FUTILITY} vbutil_key \
        --unpack ${TESTKEY_SCRATCH_DIR}/key_alg${algonum}.vbpubk
    if [ $? -ne 0 ]
    then
        return_code=255
    fi
}

function test_vbutil_key_all {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
      for hashalgo in ${hash_algos[@]}
      do
          test_vbutil_key_single $algorithmcounter $keylen $hashalgo
          let algorithmcounter=algorithmcounter+1
      done
  done
}

function test_vbutil_key {
    test_vbutil_key_single 4 2048 sha256
    test_vbutil_key_single 7 4096 sha256
    test_vbutil_key_single 11 8192 sha512
}

function test_vbutil_keyblock_single {
    local signing_algonum=$1
    local signing_keylen=$2
    local signing_hashalgo=$3
    local data_algonum=$4
    local data_keylen=$5
    local data_hashalgo=$6

          echo -e "For ${COL_YELLOW}signing algorithm \
RSA-${signing_keylen}/${signing_hashalgo}${COL_STOP} \
and ${COL_YELLOW}data key algorithm RSA-${datakeylen}/\
${datahashalgo}${COL_STOP}"
          # Remove old file
          keyblockfile="${TESTKEY_SCRATCH_DIR}/"
          keyblockfile+="sign${signing_algonum}_data"
          keyblockfile+="${data_algonum}.keyblock"
          rm -f ${keyblockfile}

          # Wrap private key
          ${FUTILITY} vbutil_key \
            --pack ${TESTKEY_SCRATCH_DIR}/key_alg${algonum}.vbprivk \
            --key ${TESTKEY_DIR}/key_rsa${signing_keylen}.pem \
            --algorithm $signing_algonum
          if [ $? -ne 0 ]
          then
            echo -e "${COL_RED}Wrap vbprivk${COL_STOP}"
            return_code=255
          fi

          # Wrap public key
          ${FUTILITY} vbutil_key \
            --pack ${TESTKEY_SCRATCH_DIR}/key_alg${algonum}.vbpubk \
            --key ${TESTKEY_DIR}/key_rsa${signing_keylen}.keyb \
            --algorithm $signing_algonum
          if [ $? -ne 0 ]
          then
            echo -e "${COL_RED}Wrap vbpubk${COL_STOP}"
            return_code=255
          fi

          # Pack
          ${FUTILITY} vbutil_keyblock --pack ${keyblockfile} \
            --datapubkey \
              ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk \
            --signprivate \
              ${TESTKEY_SCRATCH_DIR}/key_alg${algonum}.vbprivk
          if [ $? -ne 0 ]
          then
            echo -e "${COL_RED}Pack${COL_STOP}"
            return_code=255
          fi

          # Unpack
          ${FUTILITY} vbutil_keyblock --unpack ${keyblockfile} \
            --datapubkey \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk2 \
            --signpubkey \
            ${TESTKEY_SCRATCH_DIR}/key_alg${algonum}.vbpubk
          if [ $? -ne 0 ]
          then
            echo -e "${COL_RED}Unpack${COL_STOP}"
            return_code=255
          fi

          # Check
          if ! cmp -s \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk2
          then
            echo -e "${COL_RED}Check${COL_STOP}"
            return_code=255
            exit 1
          fi

          echo -e "${COL_YELLOW}Testing keyblock creation using \
external signer.${COL_STOP}"
          # Pack using external signer
          # Pack
          ${FUTILITY} vbutil_keyblock --pack ${keyblockfile} \
            --datapubkey \
              ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk \
            --signprivate_pem \
              ${TESTKEY_DIR}/key_rsa${signing_keylen}.pem \
            --pem_algorithm "${signing_algonum}" \
            --externalsigner "${SCRIPT_DIR}/external_rsa_signer.sh"

          if [ $? -ne 0 ]
          then
            echo -e "${COL_RED}Pack${COL_STOP}"
            return_code=255
          fi

          # Unpack
          ${FUTILITY} vbutil_keyblock --unpack ${keyblockfile} \
            --datapubkey \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk2 \
            --signpubkey \
            ${TESTKEY_SCRATCH_DIR}/key_alg${signing_algonum}.vbpubk
          if [ $? -ne 0 ]
          then
            echo -e "${COL_RED}Unpack${COL_STOP}"
            return_code=255
          fi

          # Check
          if ! cmp -s \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk \
            ${TESTKEY_SCRATCH_DIR}/key_alg${data_algonum}.vbpubk2
          then
            echo -e "${COL_RED}Check${COL_STOP}"
            return_code=255
            exit 1
          fi
}


function test_vbutil_keyblock_all {
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
          test_vbutil_keyblock_single \
                $signing_algorithmcounter $signing_keylen $signing_hashalgo \
                $data_algorithmcounter $data_keylen $data_hashalgo
          let data_algorithmcounter=data_algorithmcounter+1
        done
      done
      let signing_algorithmcounter=signing_algorithmcounter+1
    done
  done
}

function test_vbutil_keyblock {
    test_vbutil_keyblock_single 7 4096 sha256 4 2048 sha256
    test_vbutil_keyblock_single 11 8192 sha512 4 2048 sha256
    test_vbutil_keyblock_single 11 8192 sha512 7 4096 sha256
}


check_test_keys

echo
echo "Testing vbutil_key..."
if [ "$1" == "--all" ] ; then
    test_vbutil_key_all
else
    test_vbutil_key
fi

echo
echo "Testing vbutil_keyblock..."
if [ "$1" == "--all" ] ; then
    test_vbutil_keyblock_all
else
    test_vbutil_keyblock
fi

exit $return_code

