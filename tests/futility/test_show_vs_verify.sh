#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# some stuff we'll need
DEVKEYS=${SRCDIR}/tests/devkeys

# The show command exits with 0 if the data is consistent.
# The verify command exits with 0 only if all the data is verified.

####  keyblock

${FUTILITY} show ${DEVKEYS}/firmware.keyblock

if ${FUTILITY} verify ${DEVKEYS}/firmware.keyblock ; then false; fi

${FUTILITY} verify ${DEVKEYS}/firmware.keyblock \
  --publickey ${DEVKEYS}/root_key.vbpubk


#### firmware vblock

# Get some bits to look at
${FUTILITY} dump_fmap -x ${SCRIPTDIR}/data/bios_peppy_mp.bin \
  GBB:${TMP}.gbb VBLOCK_A:${TMP}.vblock_a FW_MAIN_A:${TMP}.fw_main_a
${FUTILITY} gbb_utility -g -k ${TMP}.rootkey ${TMP}.gbb


${FUTILITY} show ${TMP}.vblock_a

${FUTILITY} show ${TMP}.vblock_a --publickey ${TMP}.rootkey

${FUTILITY} show ${TMP}.vblock_a \
  --publickey ${TMP}.rootkey \
  --fv ${TMP}.fw_main_a

if ${FUTILITY} verify ${TMP}.vblock_a ; then false ; fi

if ${FUTILITY} verify ${TMP}.vblock_a \
  --publickey ${TMP}.rootkey ; then false ; fi

${FUTILITY} verify ${TMP}.vblock_a \
  --publickey ${TMP}.rootkey \
  --fv ${TMP}.fw_main_a


#### kernel partition

${FUTILITY} show ${SCRIPTDIR}/data/rec_kernel_part.bin

${FUTILITY} show ${SCRIPTDIR}/data/rec_kernel_part.bin \
  --publickey ${DEVKEYS}/kernel_subkey.vbpubk

${FUTILITY} show ${SCRIPTDIR}/data/rec_kernel_part.bin \
  --publickey ${DEVKEYS}/recovery_key.vbpubk

if ${FUTILITY} verify ${SCRIPTDIR}/data/rec_kernel_part.bin ; then false ; fi

if ${FUTILITY} verify ${SCRIPTDIR}/data/rec_kernel_part.bin \
  --publickey ${DEVKEYS}/kernel_subkey.vbpubk ; then false ; fi

${FUTILITY} verify ${SCRIPTDIR}/data/rec_kernel_part.bin \
  --publickey ${DEVKEYS}/recovery_key.vbpubk


# cleanup
rm -rf ${TMP}*
exit 0
