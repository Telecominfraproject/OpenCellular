#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate test vboot public keys and keyblocks for testing.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

function generate_vpubks {
  algorithmcounter=0
  for keylen in ${key_lengths[@]}
  do
    for hashalgo in ${hash_algos[@]}
    do
      ${FUTILITY} vbutil_key --pack \
        --in ${TESTKEY_DIR}/key_rsa${keylen}.keyb \
        --out ${TESTKEY_DIR}/key_rsa${keylen}.${hashalgo}.vbpubk \
        --version 1 \
        --algorithm ${algorithmcounter}
      let algorithmcounter=algorithmcounter+1
    done
  done
}

check_test_keys
generate_vpubks
