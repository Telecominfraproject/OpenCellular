#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script that unpacks a firmware image (in .fd format) into its component
# pieces. Only outputs firmware A and B data, vblocks and the GBB.

# The mosys tool must be in the system path.

# Abort on error
set -e

# Check arguments
if [ $# -ne 1 ] ; then
  echo "Usage: $0 src_fd"
  echo "Outputs firmware.gbb, firmware[A|B].[data|vblock]"
  exit 1
fi

# Make sure the tools we need are available.
type -P mosys &>/dev/null || \
  { echo "mosys tool not found."; exit 1; }

src_fd=$1

# Grab GBB Area offset and size
match_str="GBB Area"
line=$(mosys -f -k eeprom map $1 | grep "$match_str")
offset="$(echo $line | sed -e 's/.*area_offset=\"\([a-f0-9x]*\)\".*/\1/')"
let gbb_offset="$offset"
size="$(echo $line | sed -e 's/.*area_size=\"\([a-f0-9x]*\)\".*/\1/')"
let gbb_size="$size"

# Grab Firmware A and B offset and size
for i in "A" "B"
do
  match_str="$i Key"
  line=$(mosys -f -k eeprom map $1 | grep "$match_str")
  offset="$(echo $line | sed -e 's/.*area_offset=\"\([a-f0-9x]*\)\".*/\1/')"
  eval let \
    fw${i}_vblock_offset="$offset"
  size="$(echo $line | sed -e 's/.*area_size=\"\([a-f0-9x]*\)\".*/\1/')"
  eval let \
    fw${i}_vblock_size="$size"

  match_str="$i Data"
  line=$(mosys -f -k eeprom map $1 | grep "$match_str")
  offset="$(echo $line | sed -e 's/.*area_offset=\"\([a-f0-9x]*\)\".*/\1/')"
  eval let \
    fw${i}_offset="$offset"
  size="$(echo $line | sed -e 's/.*area_size=\"\([a-f0-9x]*\)\".*/\1/')"
  eval let \
    fw${i}_size="$size"
done

echo "Extracting GBB"
dd if="${src_fd}" of="firmware.gbb" skip="${gbb_offset}" bs=1 \
  count="${gbb_size}"
echo "Extracting Firmware data and vblock(s)"
dd if="${src_fd}" of="firmwareA.data" skip="${fwA_offset}" bs=1 \
  count="${fwA_size}"
dd if="${src_fd}" of="firmwareA.vblock" skip="${fwA_vblock_offset}" bs=1 \
  count="${fwA_vblock_size}"
dd if="${src_fd}" of="firmwareB.data" skip="${fwB_offset}" bs=1 \
  count="${fwB_size}"
dd if="${src_fd}" of="firmwareB.vblock" skip="${fwB_vblock_offset}" bs=1 \
  count="${fwB_vblock_size}"
