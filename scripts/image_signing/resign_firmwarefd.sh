#!/bin/sh

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to resign a firmware image using a different set of keys
# for use on signing servers.
#
# arguments: src_fd, dst_fd, firmware_datakey, and firmware_keyblock
#
# src_fd: Input firmware image (in .fd format)
# dst_fd: output firmware image name
# firmware_datakey: Key used to sign firmware data (in .vbprivk format)
# firmware_keyblock: Key block for firmware data key (in .keyblock format)
#
# Both the mosys tool and vbutil_firmware should be in the system path.
#
# This script parses the output of mosys tool from
# http://code.google.com/p/mosys
#
# to determine the regions in the image containing "Firmware [A|B] Data" and
# "Firmware [A|B] Key", which contain firmware data and firmware vblocks
# respectively. It will then generate new vblocks using the set of keys
# passed as arguments and output a new firmware image, with this new firmware
# vblocks the old ones.
#
# Here is an example output of mosys:
#
# area_offset="0x001c0000" area_size="0x00040000" area_name="Boot Stub" \
#   area_flags_raw="0x01" area_flags="static"
# area_offset="0x001a0000" area_size="0x00020000" area_name="GBB Area" \
#   area_flags_raw="0x01" area_flags="static"
# area_offset="0x00008000" area_size="0x00002000" area_name="Firmware A Key" \
#   area_flags_raw="0x01" area_flags="static"
# area_offset="0x0000a000" area_size="0x0009e000" area_name="Firmware A Data" \
#  area_flags_raw="0x03" area_flags="static,compressed"
# area_offset="0x000a8000" area_size="0x00002000" area_name="Firmware B Key" \
#  area_flags_raw="0x01" area_flags="static"
# area_offset="0x000aa000" area_size="0x0002e000" area_name="Firmware B Data" \
#  area_flags_raw="0x03" area_flags="static,compressed"
# area_offset="0x00005200" area_size="0x00001000" area_name="RW VPD" \
#  area_flags_raw="0x00" area_flags=""
#
# This shows that Firmware A Data is at offset 0x0000a0000 in the .fd image
# and is of size 0x0009e000 bytes. This can be extracted to generate new vblocks
# which can then replace old vblock for Firmware A ("Firmware A Key" region at
# offset 0x00008000 and size 0x00002000).

# Load common constants and variables.
. "$(dirname "$0")/common_minimal.sh"

# Abort on error
set -e

# Check arguments
if [ $# -lt 7 ] || [ $# -gt 8 ]; then
  echo "Usage: $PROG src_fd dst_fd firmware_datakey firmware_keyblock"\
   "dev_firmware_datakey dev_firmware_keyblock kernel_subkey [version]"
  exit 1
fi

# Make sure the tools we need are available.
for prog in mosys vbutil_firmware; do
  type "${prog}" &>/dev/null || \
    { echo "${prog} tool not found."; exit 1; }
done

SRC_FD=$1
DST_FD=$2
FIRMWARE_DATAKEY=$3
FIRMWARE_KEYBLOCK=$4
DEV_FIRMWARE_DATAKEY=$5
DEV_FIRMWARE_KEYBLOCK=$6
KERNEL_SUBKEY=$7
VERSION=$8

if [ -z $VERSION ]; then
  VERSION=1
fi
echo "Using firmware version: $VERSION"

# Parse offsets and size of firmware data and vblocks
for i in "A" "B"
do
  line=$(mosys -f -k eeprom map $1 | grep "$i Key") ||
  line=$(mosys -f -k eeprom map $1 | grep "VBLOCK_$i") ||
   { echo "Couldn't parse vblock section $i from mosys output";
     exit 1; }

  offset="$(echo $line | sed -e 's/.*area_offset=\"\([a-f0-9x]*\)\".*/\1/')"
  eval fw${i}_vblock_offset=$((offset))
  size="$(echo $line | sed -e 's/.*area_size=\"\([a-f0-9x]*\)\".*/\1/')"
  eval fw${i}_vblock_size=$((size))

  line=$(mosys -f -k eeprom map $1 | grep "$i Data") ||
  line=$(mosys -f -k eeprom map $1 | grep "FW_MAIN_$i") ||
  { echo "Couldn't parse Firmware $i section from mosys output";
    exit 1; }

  offset="$(echo $line | sed -e 's/.*area_offset=\"\([a-f0-9x]*\)\".*/\1/')"
  eval fw${i}_offset=$((offset))
  size="$(echo $line | sed -e 's/.*area_size=\"\([a-f0-9x]*\)\".*/\1/')"
  eval fw${i}_size=$((size))
done

temp_fwimage=$(make_temp_file)
temp_out_vb=$(make_temp_file)

# Extract out Firmware A data and generate signature using the right keys.
# Firmware A is the dev firmware.
dd if="${SRC_FD}" of="${temp_fwimage}" skip="${fwA_offset}" bs=1 \
  count="${fwA_size}"

echo "Re-calculating Firmware A vblock"
vbutil_firmware \
  --vblock "${temp_out_vb}" \
  --keyblock "${DEV_FIRMWARE_KEYBLOCK}" \
  --signprivate "${DEV_FIRMWARE_DATAKEY}" \
  --version "${VERSION}" \
  --fv "${temp_fwimage}" \
  --kernelkey "${KERNEL_SUBKEY}"

# Create a copy of the input image and put in the new vblock for firmware A
cp "${SRC_FD}" "${DST_FD}"
dd if="${temp_out_vb}" of="${DST_FD}" seek="${fwA_vblock_offset}" bs=1 \
  count="${fwA_vblock_size}" conv=notrunc

# Firmware B is the normal firmware.
dd if="${SRC_FD}" of="${temp_fwimage}" skip="${fwB_offset}" bs=1 \
  count="${fwB_size}"
echo "Re-calculating Firmware B vblock"
vbutil_firmware \
  --vblock "${temp_out_vb}" \
  --keyblock "${FIRMWARE_KEYBLOCK}" \
  --signprivate "${FIRMWARE_DATAKEY}" \
  --version "${VERSION}" \
  --fv "${temp_fwimage}" \
  --kernelkey "${KERNEL_SUBKEY}"

# Destination image has already been created.
dd if="${temp_out_vb}" of="${DST_FD}" seek="${fwB_vblock_offset}" bs=1 \
  count="${fwB_vblock_size}" conv=notrunc

echo "New signed image was output to ${DST_FD}"
