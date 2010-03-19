#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate test keys for use by the tests.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Generate RSA test keys of various lengths. 
function generate_keys {
  for i in ${key_lengths[@]}
  do
    openssl genrsa -F4 -out ${TESTKEY_DIR}/key_rsa$i.pem $i
    # Generate self-signed certificate from key.
    openssl req -batch -new -x509 -key ${TESTKEY_DIR}/key_rsa$i.pem \
      -out ${TESTKEY_DIR}/key_rsa$i.crt
    # Generate pre-processed key for use by RSA signature verification code.
    ${UTIL_DIR}/dumpRSAPublicKey ${TESTKEY_DIR}/key_rsa$i.crt \
      > ${TESTKEY_DIR}/key_rsa$i.keyb
  done
}

mkdir -p ${TESTKEY_DIR}
generate_keys
