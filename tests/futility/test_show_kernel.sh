#!/bin/bash -eux
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

DEVKEYS=${SRCDIR}/tests/devkeys
TESTKEYS=${SRCDIR}/tests/testkeys

echo 'Creating test kernel'

# Dummy kernel data
echo "hi there" > ${TMP}.config.txt
dd if=/dev/urandom bs=16384 count=1 of=${TMP}.bootloader.bin
dd if=/dev/urandom bs=32768 count=1 of=${TMP}.kernel.bin

# Pack kernel data key using original vboot utilities.
${FUTILITY} vbutil_key --pack ${TMP}.datakey.test \
    --key ${TESTKEYS}/key_rsa2048.keyb --algorithm 4

# Keyblock with kernel data key is signed by kernel subkey
# Flags=5 means dev=0 rec=0
${FUTILITY} vbutil_keyblock --pack ${TMP}.keyblock.test \
    --datapubkey ${TMP}.datakey.test \
    --flags 5 \
    --signprivate ${DEVKEYS}/kernel_subkey.vbprivk

# Kernel preamble is signed with the kernel data key
${FUTILITY} vbutil_kernel \
    --pack ${TMP}.kernel.test \
    --keyblock ${TMP}.keyblock.test \
    --signprivate ${TESTKEYS}/key_rsa2048.sha256.vbprivk \
    --version 1 \
    --arch arm \
    --vmlinuz ${TMP}.kernel.bin \
    --bootloader ${TMP}.bootloader.bin \
    --config ${TMP}.config.txt

echo 'Verifying test kernel'

# Verify the kernel
${FUTILITY} show ${TMP}.kernel.test \
    --publickey ${DEVKEYS}/kernel_subkey.vbpubk \
  | egrep 'Signature.*valid'

echo 'Test kernel blob looks good'

# cleanup
rm -rf ${TMP}*
exit 0
