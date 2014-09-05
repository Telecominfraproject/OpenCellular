#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate test keys for use by the tests.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set -e

sha_types=( 1 256 512 )

# Generate RSA test keys of various lengths.
function generate_keys {
  key_index=0
  key_name_base="${TESTKEY_DIR}/key_rsa"
  for i in ${key_lengths[@]}
  do
    key_base="${key_name_base}${i}"
    if [ -f "${key_base}.keyb" ]; then
      continue
    fi

    openssl genrsa -F4 -out ${key_base}.pem $i
    # Generate self-signed certificate from key.
    openssl req -batch -new -x509 -key ${key_base}.pem \
      -out ${key_base}.crt

    # Generate pre-processed key for use by RSA signature verification code.
    ${BIN_DIR}/dumpRSAPublicKey -cert ${key_base}.crt \
      > ${key_base}.keyb

    alg_index=0
    for sha_type in ${sha_types[@]}
    do
      alg=$((${key_index} * 3 + ${alg_index}))
  # wrap the public key
      ${FUTILITY} vbutil_key \
        --pack "${key_base}.sha${sha_type}.vbpubk" \
        --key "${key_base}.keyb" \
        --version 1 \
        --algorithm ${alg}

  # wrap the private key
      ${FUTILITY} vbutil_key \
        --pack "${key_base}.sha${sha_type}.vbprivk" \
        --key "${key_base}.pem" \
        --algorithm ${alg}
      alg_index=$((${alg_index} + 1))
    done
    key_index=$((${key_index} + 1))
  done
}

mkdir -p ${TESTKEY_DIR}
generate_keys
