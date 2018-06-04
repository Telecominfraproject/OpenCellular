#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"


IN=${SCRIPTDIR}/data/bios_link_mp.bin
BIOS=${TMP}.bios.bin

cp ${IN} ${BIOS}

AREAS="RW_SECTION_A VBLOCK_B BOOT_STUB"

# Extract good blobs first
${FUTILITY} dump_fmap -x ${BIOS} ${AREAS}

# Save the good blobs, make same-size random blobs, create command
CMDS=""
for a in ${AREAS}; do
  size=$(stat -c '%s' $a)
  mv $a $a.good
  dd if=/dev/urandom of=$a.rand bs=$size count=1
  CMDS="$CMDS $a:$a.rand"
done

# Poke the new blobs in
${FUTILITY} load_fmap ${BIOS} ${CMDS}

# Pull them back out and see if they match
${FUTILITY} dump_fmap -x ${BIOS} ${AREAS}
for a in ${AREAS}; do
  cmp $a $a.rand
done

# cleanup
rm -f ${TMP}* ${AREAS} *.rand *.good
exit 0
