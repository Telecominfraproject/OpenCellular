#!/bin/bash

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# End-to-end test for vboot2 firmware verification

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set -e

echo 'Creating test firmware'

# Run tests in a dedicated directory for easy cleanup or debugging.
DIR="${TEST_DIR}/vb2fw_test_dir"
[ -d "$DIR" ] || mkdir -p "$DIR"
echo "Testing vb2_verify_fw in $DIR"
cd "$DIR"

# Dummy firmware body
echo 'This is a test firmware body.  This is only a test.  Lalalalala' \
    > body.test

# Pack keys using original vboot utilities
${FUTILITY} vbutil_key --pack rootkey.test \
    --key ${TESTKEY_DIR}/key_rsa8192.keyb --algorithm 11
${FUTILITY} vbutil_key --pack fwsubkey.test \
    --key ${TESTKEY_DIR}/key_rsa4096.keyb --algorithm 7
${FUTILITY} vbutil_key --pack kernkey.test \
    --key ${TESTKEY_DIR}/key_rsa2048.keyb --algorithm 4

# Create a GBB with the root key
${FUTILITY} gbb_utility -c 128,2400,0,0 gbb.test
${FUTILITY} gbb_utility gbb.test -s --hwid='Test GBB' \
  --rootkey=rootkey.test

# Keyblock with firmware subkey is signed by root key
${FUTILITY} vbutil_keyblock --pack keyblock.test \
    --datapubkey fwsubkey.test \
    --signprivate ${TESTKEY_DIR}/key_rsa8192.sha512.vbprivk

# Firmware preamble is signed with the firmware subkey
${FUTILITY} vbutil_firmware \
    --vblock vblock.test \
    --keyblock keyblock.test \
    --signprivate ${TESTKEY_DIR}/key_rsa4096.sha256.vbprivk \
    --fv body.test \
    --version 1 \
    --kernelkey kernkey.test

echo 'Verifying test firmware using vb2_verify_fw'

# Verify the firmware using vboot2 checks
${BUILD_RUN}/tests/vb20_verify_fw gbb.test vblock.test body.test

happy 'vb2_verify_fw succeeded'
