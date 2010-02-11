#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate test keys for use by the tests.

KEY_DIR=testkeys
key_lengths=( 1024 2048 4096 8192 ) 
UTIL_DIR=../utils/

# Generate RSA test keys of various lengths. 
function generate_keys {
  for i in ${key_lengths[@]}
  do
    openssl genrsa -F4 -out ${KEY_DIR}/key_rsa$i.pem $i
    # Generate self-signed certificate from key.
    openssl req -batch -new -x509 -key ${KEY_DIR}/key_rsa$i.pem \
      -out ${KEY_DIR}/key_rsa$i.crt
    # Generate pre-processed key for use by RSA signature verification code.
    ${UTIL_DIR}/dumpRSAPublicKey ${KEY_DIR}/key_rsa$i.crt \
      > ${KEY_DIR}/key_rsa$i.keyb
  done
}

if [ ! -d "$KEY_DIR" ]
then
  mkdir  "$KEY_DIR"
fi

generate_keys
