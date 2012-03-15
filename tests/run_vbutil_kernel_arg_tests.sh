#!/bin/bash -u
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Quick test of vbutil_kernel args, to make sure we can pack and unpack
# less-than-full-sized components.
#

# Load common constants and variables for tests.
. "$(dirname "$0")/common.sh"

# directories
DATA_DIR="${SCRIPT_DIR}/preamble_tests/data"
TMPDIR="${TEST_DIR}/vbutil_kernel_arg_tests_dir"
[ -d "${TMPDIR}" ] || mkdir -p "${TMPDIR}"

# Arbitrarily chosen keys and config file.
KEYBLOCK="${DATA_DIR}/kb_0_0.keyblock"
SIGNPRIVATE="${DATA_DIR}/data_0.vbprivk"
SIGNPUBLIC="${DATA_DIR}/root_0.vbpubk"
CONFIG="${DATA_DIR}/dummy_config.txt"

# Create some big and little files for the kernel and bootloader
BIG="${TMPDIR}/big.bin"
dd if=/dev/urandom bs=32768 count=1 of="${BIG}" 2>/dev/null
SMALL="${TMPDIR}/small.bin"
dd if=/dev/urandom bs=16 count=1 of="${SMALL}" 2>/dev/null

declare -a KERN_VALS
declare -a BOOT_VALS
KERN_VALS=("--vmlinuz=${BIG}" "--vmlinuz=${SMALL}")
BOOT_VALS=("--bootloader=${BIG}" "--bootloader=${SMALL}")

tests=0
errs=0

# Pack a bunch of stuff
k=0
while [ "$k" -lt "${#KERN_VALS[*]}" ]; do
  b=0
  while [ "$b" -lt "${#BOOT_VALS[*]}" ]; do
    echo -n "pack kern_${k}_${b}.vblock ... "
    : $(( tests++ ))
      ${UTIL_DIR}/vbutil_kernel --pack "${TMPDIR}/kern_${k}_${b}.vblock" \
        --keyblock "${KEYBLOCK}" \
        --signprivate "${SIGNPRIVATE}" \
        --version 1 \
        --arch arm \
        --config "${CONFIG}" \
        "${KERN_VALS[$k]}" \
        "${BOOT_VALS[$k]}" >/dev/null
      if [ "$?" -ne 0 ]; then
        echo -e "${COL_RED}FAILED${COL_STOP}"
        : $(( errs++ ))
      else
        echo -e "${COL_GREEN}PASSED${COL_STOP}"
      fi
    : $(( b++ ))
  done
  : $(( k++ ))
done

# Now unpack it
for v in ${TMPDIR}/kern_*.vblock; do
  : $(( tests++ ))
  vv=$(basename "$v")
  echo -n "verify $vv ... "
  "${UTIL_DIR}/vbutil_kernel" --verify "$v" >/dev/null
  if [ "$?" -ne 0 ]; then
    echo -e "${COL_RED}FAILED${COL_STOP}"
    : $(( errs++ ))
  else
    echo -e "${COL_GREEN}PASSED${COL_STOP}"
  fi
  : $(( tests++ ))
  echo -n "verify $vv signed ... "
  "${UTIL_DIR}/vbutil_kernel" --verify "$v" \
    --signpubkey "${SIGNPUBLIC}" >/dev/null
  if [ "$?" -ne 0 ]; then
    echo -e "${COL_RED}FAILED${COL_STOP}"
    : $(( errs++ ))
  else
    echo -e "${COL_GREEN}PASSED${COL_STOP}"
  fi
done

# Summary
ME=$(basename "$0")
if [ "$errs" -ne 0 ]; then
  echo -e "${COL_RED}${ME}: ${errs}/${tests} tests failed${COL_STOP}"
  exit 1
fi
happy "${ME}: All ${tests} tests passed"
exit 0
