#!/bin/bash -eux
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# Test 'futility show' against expected output
SHOW_FILES="
  tests/devkeys/root_key.vbpubk
  tests/devkeys/root_key.vbprivk
  tests/devkeys/kernel.keyblock
  tests/futility/data/fw_vblock.bin
  tests/futility/data/fw_gbb.bin
  tests/futility/data/bios_zgb_mp.bin
  tests/futility/data/bios_mario_mp.bin
  tests/futility/data/kern_preamble.bin
  tests/futility/data/sample.vbpubk2
  tests/futility/data/sample.vbprik2
  tests/testkeys/key_rsa2048.pem
  tests/testkeys/key_rsa8192.pub.pem
"

for file in $SHOW_FILES; do
    outfile="show.${file//\//_}"
    gotfile="${OUTDIR}/${outfile}"
    wantfile="${SRCDIR}/tests/futility/expect_output/${outfile}"
    ${FUTILITY} show "${SRCDIR}/${file}" | tee "${gotfile}"

    # Uncomment this to update the expected output
    #cp ${gotfile} ${wantfile}

    diff ${wantfile} ${gotfile}
done


# Test 'futility vbutil_key' against expected output
VBUTIL_KEY_FILES="
  tests/devkeys/root_key.vbpubk
  tests/devkeys/root_key.vbprivk
"

for file in $VBUTIL_KEY_FILES; do
    outfile="vbutil_key.${file//\//_}"
    gotfile="${OUTDIR}/${outfile}"
    wantfile="${SRCDIR}/tests/futility/expect_output/${outfile}"
    ${FUTILITY} vbutil_key --unpack "${SRCDIR}/${file}" | tee "${gotfile}"

    # Uncomment this to update the expected output
    #cp ${gotfile} ${wantfile}

    diff ${wantfile} ${gotfile}
done


# Test 'futility vbutil_keyblock' against expected output
file="tests/devkeys/kernel.keyblock"
outfile="vbutil_keyblock.${file//\//_}"
gotfile="${OUTDIR}/${outfile}"
wantfile="${SRCDIR}/tests/futility/expect_output/${outfile}"
${FUTILITY} vbutil_keyblock --unpack "${SRCDIR}/${file}" \
    --signpubkey "${SRCDIR}/tests/devkeys/kernel_subkey.vbpubk" \
    | tee "${gotfile}"

# Uncomment this to update the expected output
#cp ${gotfile} ${wantfile}

diff ${wantfile} ${gotfile}


# Test 'futility vbutil_firmware' against expected output
KEYDIR=${SRCDIR}/tests/devkeys
outfile="vbutil_firmware.verify"
gotfile="${OUTDIR}/${outfile}"
wantfile="${SRCDIR}/tests/futility/expect_output/${outfile}"

# Create a firmware blob and vblock.  Version and flags are just
# arbitrary non-zero numbers so we can verify they're printed
# properly.
dd bs=1024 count=16 if=/dev/urandom of=${TMP}.fw_main
${FUTILITY} vbutil_firmware --vblock ${TMP}.vblock.old \
  --keyblock ${KEYDIR}/firmware.keyblock \
  --signprivate ${KEYDIR}/firmware_data_key.vbprivk \
  --version 12 \
  --fv ${TMP}.fw_main \
  --kernelkey ${KEYDIR}/kernel_subkey.vbpubk \
  --flags 42

# Verify
${FUTILITY} vbutil_firmware --verify ${TMP}.vblock.old \
  --signpubkey ${KEYDIR}/root_key.vbpubk \
  --fv ${TMP}.fw_main | tee "${gotfile}"

# Uncomment this to update the expected output
#cp ${gotfile} ${wantfile}

diff ${wantfile} ${gotfile}


# cleanup
rm -rf ${TMP}*
exit 0
