#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Standalone version of cros_resign_image.sh script from
# from chromeos/src/scripts/bin/ for use on signing servers.

# Both the cgpt tool and vbutil_kernel should be in the system path.

# Abort on error
set -e

# Check arguments
if [ $# -ne 4 ] ; then
  echo "usage: $0 src_bin dst_bin kernel_datakey kernel_keyblock"
  exit 1
fi

# Make sure the tools we need are available.
type -P cgpt &>/dev/null || \
  { echo "cgpt tool not found."; exit 1; }
type -P vbutil_kernel &>/dev/null || \
  { echo "vbutil_kernel tool not found."; exit 1; }

sector_size=512  # sector size in bytes
num_sectors_vb=128  # number of sectors in kernel verification blob
src_bin=$1
dst_bin=$2
datakey=$3
keyblock=$4

koffset="$(cgpt show -b -i 2 $1)"
ksize="$(cgpt show -s -i 2 $1)"

echo "Re-signing image ${src_bin} and outputting ${dst_bin}"
temp_kimage=$(mktemp)
trap "rm -f ${temp_kimage}" EXIT
temp_out_vb=$(mktemp)
trap "rm -f ${temp_out_vb}" EXIT

# Grab the kernel image in preparation for resigning
dd if="${src_bin}" of="${temp_kimage}" skip=$koffset bs=$sector_size \
  count=$ksize
vbutil_kernel \
  --repack "${temp_out_vb}" \
  --vblockonly \
  --keyblock "${kernel_keyblock}" \
  --signprivate "${kernel_datakey}" \
  --oldblob "${temp_kimage}"

# Create a copy of the input image and put in the new vblock
cp "${src_bin}" "${dst_bin}"
dd if="${temp_out_vb}" of="${dst_bin}" seek=$koffset bs=$sector_size \
  count=$num_sectors_vb conv=notrunc

echo "New signed image was output to ${dst_bin}"

