#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

KEYDIR=${SRCDIR}/tests/devkeys

# The input BIOS images are all signed with MP keys. We resign them with dev
# keys, which means we can precalculate the expected results. Note that the
# script does not change the root or recovery keys in the GBB.
INFILES="
${SCRIPTDIR}/data/bios_link_mp.bin
${SCRIPTDIR}/data/bios_mario_mp.bin
${SCRIPTDIR}/data/bios_peppy_mp.bin
${SCRIPTDIR}/data/bios_zgb_mp.bin
"

count=0
for infile in $INFILES; do

  base=${infile##*/}

  : $(( count++ ))
  echo -n "$count " 1>&3

  outfile=${TMP}.${base}.new
  loemid="loem"
  loemdir=${TMP}.${base}_dir

  mkdir -p ${loemdir}

  # grep for existing sha1sums (skipping root & recovery keys)
  ${FUTILITY} show ${infile} | grep sha1sum \
    | sed -e 's/.*: \+//' | tail -n 4 > ${TMP}.${base}.sha.orig

  # resign_firmwarefd.sh works on BIOS image files. The args are:
  #
  #   infile
  #   outfile
  #   firmware_datakey
  #   firmware_keyblock
  #   dev_firmware_datakey   (these are only used if RW A & RW B differ)
  #   dev_firmware_keyblock
  #   kernel_subkey
  #   firmware_version
  #   preamble_flag
  #   loem_output_dir        (optional: dir for copy of new vblocks)
  #   loemid                 (optional: copy new vblocks using this name)
  #
  ${BINDIR}/resign_firmwarefd.sh \
    ${infile} \
    ${outfile} \
    ${KEYDIR}/firmware_data_key.vbprivk \
    ${KEYDIR}/firmware.keyblock \
    ${KEYDIR}/dev_firmware_data_key.vbprivk \
    ${KEYDIR}/dev_firmware.keyblock \
    ${KEYDIR}/kernel_subkey.vbpubk \
    14 \
    9 \
    ${loemdir} \
    ${loemid}

  # check the firmware version and preamble flags
  m=$(${FUTILITY} show ${outfile} | \
    egrep 'Firmware version: +14$|Preamble flags: +9$' | wc -l)
  [ "$m" = "4" ]

  # check the sha1sums
  ${FUTILITY} show ${outfile} | grep sha1sum \
    | sed -e 's/.*: \+//' > ${TMP}.${base}.sha.new
  cmp ${SCRIPTDIR}/data_${base}_expect.txt ${TMP}.${base}.sha.new

  # and the LOEM stuff
  ${FUTILITY} show ${loemdir}/*.${loemid} | grep sha1sum \
    | sed -e 's/.*: \+//' > ${loemdir}/loem.sha.new
  # the vblocks don't have root or recovery keys
  tail -4 ${SCRIPTDIR}/data_${base}_expect.txt > ${loemdir}/sha.expect
  cmp ${loemdir}/sha.expect ${loemdir}/loem.sha.new

done

# cleanup
rm -rf ${TMP}*
exit 0
