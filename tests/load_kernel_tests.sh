#!/bin/bash

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# End-to-end test for vboot2 kernel verification

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set -e

CGPT=${BIN_DIR}/cgpt

echo 'Creating test kernel'

# Run tests in a dedicated directory for easy cleanup or debugging.
DIR="${TEST_DIR}/load_kernel_test_dir"
[ -d "$DIR" ] || mkdir -p "$DIR"
echo "Testing kernel verification in $DIR"
cd "$DIR"

# Dummy kernel data
echo "hi there" > "dummy_config.txt"
dd if=/dev/urandom bs=16384 count=1 of="dummy_bootloader.bin"
dd if=/dev/urandom bs=32768 count=1 of="dummy_kernel.bin"

# Pack kernel data key using original vboot utilities.
${FUTILITY} vbutil_key --pack datakey.test \
    --key ${TESTKEY_DIR}/key_rsa2048.keyb --algorithm 4

# Keyblock with kernel data key is signed by kernel subkey
# Flags=5 means dev=0 rec=0
${FUTILITY} vbutil_keyblock --pack keyblock.test \
    --datapubkey datakey.test \
    --flags 5 \
    --signprivate ${SCRIPT_DIR}/devkeys/kernel_subkey.vbprivk

# Kernel preamble is signed with the kernel data key
${FUTILITY} vbutil_kernel \
    --pack "kernel.test" \
    --keyblock "keyblock.test" \
    --signprivate ${TESTKEY_DIR}/key_rsa2048.sha256.vbprivk \
    --version 1 \
    --arch arm \
    --vmlinuz "dummy_kernel.bin" \
    --bootloader "dummy_bootloader.bin" \
    --config "dummy_config.txt"

echo 'Verifying test kernel'

# Verify the kernel
${FUTILITY} vbutil_kernel \
    --verify "kernel.test" \
    --signpubkey ${SCRIPT_DIR}/devkeys/kernel_subkey.vbpubk

happy 'Kernel verification succeeded'

# Now create a dummy disk image
echo 'Creating test disk image'
dd if=/dev/zero of=disk.test bs=1024 count=1024
${CGPT} create disk.test
${CGPT} add -i 1 -S 1 -P 1 -b 64 -s 960 -t kernel -l kernelA disk.test
${CGPT} show disk.test

# And insert the kernel into it
dd if=kernel.test of=disk.test bs=512 seek=64 conv=notrunc

# And verify it using futility
echo 'Verifying test disk image'
${BUILD_RUN}/tests/verify_kernel disk.test \
    ${SCRIPT_DIR}/devkeys/kernel_subkey.vbpubk

happy 'Image verification succeeded'
