#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script that just takes in a kernel partition and outputs a new vblock
# signed with the specific keys. For use on signing servers.

# vbutil_kernel must be in the system path.

SCRIPT_DIR=$(dirname $0)

# Abort on error
set -e

# Check arguments
if [ $# -lt 4 ] || [ $# -gt 5 ]; then
  echo "usage: $0 src_kpart dst_vblock kernel_datakey kernel_keyblock [version]"
  exit 1
fi

# Make sure the tools we need are available.
type -P vbutil_kernel &>/dev/null || \
  ( echo "vbutil_kernel tool not found."; exit 1; )

SRC_KPART=$1
DST_VBLOCK=$2
KERNEL_DATAKEY=$3
KERNEL_KEYBLOCK=$4
VERSION=$5

if [ -z $VERSION ]; then
  VERSION=1
fi
echo "Using kernel version: $VERSION"

vbutil_kernel --repack "${DST_VBLOCK}" \
  --vblockonly \
  --keyblock "${KERNEL_KEYBLOCK}" \
  --signprivate "${KERNEL_DATAKEY}" \
  --version "${VERSION}" \
  --oldblob "${SRC_KPART}"

echo "New kernel vblock was output to ${DST_VBLOCK}"

