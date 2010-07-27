#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script that just takes in a kernel partition and outputs a new vblock
# signed with the specific keys. For use on signing servers.

# vbutil_kernel must be in the system path.

# Abort on error
set -e

# Check arguments
if [ $# -ne 4 ] ; then
  echo "usage: $0 src_kpart dst_vblock kernel_datakey kernel_keyblock"
  exit 1
fi

# Make sure the tools we need are available.
type -P vbutil_kernel &>/dev/null || \
  ( echo "vbutil_kernel tool not found."; exit 1; )

src_kpart=$1
dst_vblock=$2
kernel_datakey=$3
kernel_keyblock=$4

vbutil_kernel \
  --repack "${dst_vblock}" \
  --vblockonly \
  --keyblock "${kernel_keyblock}" \
  --signprivate "${kernel_datakey}" \
  --oldblob "${src_kpart}"

echo "New kernel vblock was output to ${dst_vblock}"

