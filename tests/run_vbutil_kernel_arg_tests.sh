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
DEVKEYS="${ROOT_DIR}/tests/devkeys"
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
      "${FUTILITY}" vbutil_kernel \
        --pack "${TMPDIR}/kern_${k}_${b}.vblock" \
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
  "${FUTILITY}" vbutil_kernel --verify "$v" >/dev/null
  if [ "$?" -ne 0 ]; then
    echo -e "${COL_RED}FAILED${COL_STOP}"
    : $(( errs++ ))
  else
    echo -e "${COL_GREEN}PASSED${COL_STOP}"
  fi
  : $(( tests++ ))
  echo -n "verify $vv signed ... "
  "${FUTILITY}" vbutil_kernel --verify "$v" \
    --signpubkey "${SIGNPUBLIC}" >/dev/null
  if [ "$?" -ne 0 ]; then
    echo -e "${COL_RED}FAILED${COL_STOP}"
    : $(( errs++ ))
  else
    echo -e "${COL_GREEN}PASSED${COL_STOP}"
  fi
done



# Test repacking a USB image for the SSD, the way the installer does.

set -e
# Pack for USB
USB_KERN="${TMPDIR}/usb_kern.bin"
USB_KEYBLOCK="${DEVKEYS}/recovery_kernel.keyblock"
USB_SIGNPRIVATE="${DEVKEYS}/recovery_kernel_data_key.vbprivk"
USB_SIGNPUBKEY="${DEVKEYS}/recovery_key.vbpubk"
echo -n "pack USB kernel ... "
: $(( tests++ ))
"${FUTILITY}" vbutil_kernel \
  --pack "${USB_KERN}" \
  --keyblock "${USB_KEYBLOCK}" \
  --signprivate "${USB_SIGNPRIVATE}" \
  --version 1 \
  --config "${CONFIG}" \
  --bootloader "${BIG}" \
  --vmlinuz "${BIG}" \
  --arch arm
if [ "$?" -ne 0 ]; then
  echo -e "${COL_RED}FAILED${COL_STOP}"
  : $(( errs++ ))
else
  echo -e "${COL_GREEN}PASSED${COL_STOP}"
fi

# And verify it.
echo -n "verify USB kernel ... "
: $(( tests++ ))
"${FUTILITY}" vbutil_kernel \
  --verify "${USB_KERN}" \
  --signpubkey "${USB_SIGNPUBKEY}" >/dev/null
if [ "$?" -ne 0 ]; then
  echo -e "${COL_RED}FAILED${COL_STOP}"
  : $(( errs++ ))
else
  echo -e "${COL_GREEN}PASSED${COL_STOP}"
fi

# Now we re-sign the same image using the normal keys. This is the kernel
# image that is put on the hard disk by the installer. Note: To save space on
# the USB image, we're only emitting the new verfication block, and the
# installer just replaces that part of the hard disk's kernel partition.
SSD_KERN="${TMPDIR}/ssd_kern.bin"
SSD_KEYBLOCK="${DEVKEYS}/kernel.keyblock"
SSD_SIGNPRIVATE="${DEVKEYS}/kernel_data_key.vbprivk"
SSD_SIGNPUBKEY="${DEVKEYS}/kernel_subkey.vbpubk"
echo -n "repack to SSD kernel ... "
: $(( tests++ ))
"${FUTILITY}" vbutil_kernel \
  --repack "${SSD_KERN}" \
  --vblockonly \
  --keyblock "${SSD_KEYBLOCK}" \
  --signprivate "${SSD_SIGNPRIVATE}" \
  --oldblob "${TMPDIR}/usb_kern.bin" >/dev/null
if [ "$?" -ne 0 ]; then
  echo -e "${COL_RED}FAILED${COL_STOP}"
  : $(( errs++ ))
else
  echo -e "${COL_GREEN}PASSED${COL_STOP}"
fi

# To verify it, we have to replace the vblock from the original image.
tempfile="${TMPDIR}/foo.bin"
cat "${SSD_KERN}" > "$tempfile"
dd if="${USB_KERN}" bs=65536 skip=1 >> $tempfile 2>/dev/null

echo -n "verify SSD kernel ... "
: $(( tests++ ))
"${FUTILITY}" vbutil_kernel \
  --verify "$tempfile" \
  --signpubkey "${SSD_SIGNPUBKEY}" >/dev/null
if [ "$?" -ne 0 ]; then
  echo -e "${COL_RED}FAILED${COL_STOP}"
  : $(( errs++ ))
else
  echo -e "${COL_GREEN}PASSED${COL_STOP}"
fi

# Finally make sure that the kernel command line stays good.
orig=$(cat "${CONFIG}" | tr '\012' ' ')
packed=$("${FUTILITY}" dump_kernel_config "${USB_KERN}")
echo -n "check USB kernel config ..."
: $(( tests++ ))
if [ "$orig" != "$packed" ]; then
  echo -e "${COL_RED}FAILED${COL_STOP}"
  : $(( errs++ ))
else
  echo -e "${COL_GREEN}PASSED${COL_STOP}"
fi

repacked=$("${FUTILITY}" dump_kernel_config "${tempfile}")
echo -n "check SSD kernel config ..."
: $(( tests++ ))
if [ "$orig" != "$packed" ]; then
  echo -e "${COL_RED}FAILED${COL_STOP}"
  : $(( errs++ ))
else
  echo -e "${COL_GREEN}PASSED${COL_STOP}"
fi

# Summary
ME=$(basename "$0")
if [ "$errs" -ne 0 ]; then
  echo -e "${COL_RED}${ME}: ${errs}/${tests} tests failed${COL_STOP}"
  exit 1
fi
happy "${ME}: All ${tests} tests passed"
exit 0
