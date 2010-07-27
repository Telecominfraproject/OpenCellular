#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script that unpacks a firmware image (in .fd format) into its component
# pieces. Only outputs firmware A and B data, vblocks and the GBB.

# The fmap_decode tool must be in the system path.

# Abort on error
set -e

# Check arguments
if [ $# -ne 1 ] ; then
  echo "Usage: $0 src_fd"
  echo "Outputs firmware.gbb, firmware[A|B].[data|vblock]"
  exit 1
fi

# Make sure the tools we need are available.
type -P fmap_decode &>/dev/null || \
  { echo "fmap_decode tool not found."; exit 1; }

src_fd=$1

# Parse offsets and size of firmware data and vblocks
let gbb_offset="$(fmap_decode $1 | grep GBB | cut -b 14-23)"
let gbb_size="$(fmap_decode $1 | grep GBB | cut -b 37-46)"
set -x
for i in "A" "B"
do
  match_str="$i Key"
  eval let \
    fw${i}_vblock_offset="$(fmap_decode $1 | grep "$match_str" | cut -b 14-23)"
  eval let \
    fw${i}_vblock_size="$(fmap_decode $1 | grep "$match_str" | cut -b 37-46)"

  match_str="$i Data"
  eval let fw${i}_offset="$(fmap_decode $1 | grep "$match_str" | cut -b 14-23)"
  eval let fw${i}_size="$(fmap_decode $1 | grep "$match_str" | cut -b 37-46)"
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
