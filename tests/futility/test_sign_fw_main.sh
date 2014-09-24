#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

KEYDIR=${SRCDIR}/tests/devkeys

# create a firmware blob
dd bs=1024 count=16 if=/dev/urandom of=${TMP}.fw_main

# try the old way
${FUTILITY} vbutil_firmware --vblock ${TMP}.vblock.old \
  --keyblock ${KEYDIR}/firmware.keyblock \
  --signprivate ${KEYDIR}/firmware_data_key.vbprivk \
  --version 12 \
  --fv ${TMP}.fw_main \
  --kernelkey ${KEYDIR}/kernel_subkey.vbpubk \
  --flags 42

# verify
${FUTILITY} vbutil_firmware --verify ${TMP}.vblock.old \
  --signpubkey ${KEYDIR}/root_key.vbpubk \
  --fv ${TMP}.fw_main

# and the new way
${FUTILITY} sign --debug \
  --signprivate ${KEYDIR}/firmware_data_key.vbprivk \
  --keyblock ${KEYDIR}/firmware.keyblock \
  --kernelkey ${KEYDIR}/kernel_subkey.vbpubk \
  --version 12 \
  --fv ${TMP}.fw_main \
  --flags 42 \
  ${TMP}.vblock.new

# They should match
cmp ${TMP}.vblock.old ${TMP}.vblock.new

# cleanup
rm -rf ${TMP}*
exit 0
