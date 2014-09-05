#!/bin/bash -u
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This tests that vblocks using pre-3.0 versions of VbFirmwarePreambleHeader
# and VbKernelPreambleHeader will still verify (or not) correctly. We need to
# keep the old versions around to make sure that we can still sign images in
# the ways that existing devices can validate.

# Load common constants and variables for tests.
. "$(dirname "$0")/common.sh"

if [ "${1:---some}" == "--all" ] ; then
    # all algs
    algs="0 1 2 3 4 5 6 7 8 9 10 11"
else
    # just the algs we use
    algs="4 7 11"
fi

# output directories
PREAMBLE_DIR="${SCRIPT_DIR}/preamble_tests"
DATADIR="${PREAMBLE_DIR}/data"
V2DIR="${PREAMBLE_DIR}/preamble_v2x"

tests=0
errs=0

# Check the firmware results
for d in $algs; do
  for r in $algs; do
    for rr in $algs; do
      if [ "$r" = "$rr" ]; then
        what="verify"
        cmp="-ne"
      else
        what="reject"
        cmp="-eq"
      fi
      : $(( tests++ ))
      echo -n "${what} fw_${d}_${r}.vblock with root_${rr}.vbpubk ... "
      "${FUTILITY}" vbutil_firmware \
        --verify "${V2DIR}/fw_${d}_${r}.vblock" \
        --signpubkey "${DATADIR}/root_${rr}.vbpubk" \
        --fv "${DATADIR}/FWDATA" >/dev/null 2>&1
      if [ "$?" "$cmp" 0 ]; then
        echo -e "${COL_RED}FAILED${COL_STOP}"
        : $(( errs++ ))
      else
        echo -e "${COL_GREEN}PASSED${COL_STOP}"
      fi
    done
  done
done


# Check the kernel results
for d in $algs; do
  for r in $algs; do
    for rr in $algs; do
      if [ "$r" = "$rr" ]; then
        what="verify"
        cmp="-ne"
      else
        what="reject"
        cmp="-eq"
      fi
      : $(( tests++ ))
      echo -n "${what} kern_${d}_${r}.vblock with root_${rr}.vbpubk ... "
      "${FUTILITY}" vbutil_kernel \
        --verify "${V2DIR}/kern_${d}_${r}.vblock" \
        --signpubkey "${DATADIR}/root_${rr}.vbpubk" >/dev/null 2>&1
      if [ "$?" "$cmp" 0 ]; then
        echo -e "${COL_RED}FAILED${COL_STOP}"
        : $(( errs++ ))
      else
        echo -e "${COL_GREEN}PASSED${COL_STOP}"
      fi
    done
  done
done


# Check the kernel results
for d in $algs; do
  for r in $algs; do
      : $(( tests++ ))
      echo -n "verify kern_${d}_${r}.vblock with hash only ... "
      "${FUTILITY}" vbutil_kernel \
          --verify "${V2DIR}/kern_${d}_${r}.vblock" >/dev/null 2>&1
      if [ "$?" -ne 0 ]; then
        echo -e "${COL_RED}FAILED${COL_STOP}"
        : $(( errs++ ))
      else
        echo -e "${COL_GREEN}PASSED${COL_STOP}"
      fi
  done
done


# Summary
ME=$(basename "$0")
if [ "$errs" -ne 0 ]; then
  echo -e "${COL_RED}${ME}: ${errs}/${tests} tests failed${COL_STOP}"
  exit 1
fi
happy "${ME}: All ${tests} tests passed"
exit 0
