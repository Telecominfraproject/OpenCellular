#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Standalone version of cros_resign_image.sh script from
# chromeos/src/scripts/bin/ for use on signing servers.

# Both the cgpt tool and vbutil_kernel should be in the system path.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Abort on error
set -e

# Check arguments
if [ $# -lt 4 ] || [ $# -gt 5 ] ; then
  echo "usage: $PROG src_bin dst_bin kernel_datakey kernel_keyblock [version]"
  exit 1
fi

# Make sure the tools we need are available.
for prereqs in vbutil_kernel cgpt;
do
  type -P "${prereqs}" &>/dev/null || \
    { echo "${prereqs} tool not found."; exit 1; }
done

SRC_BIN=$1
DST_BIN=$2
KERNEL_DATAKEY=$3
KERNEL_KEYBLOCK=$4
VERSION=$5

if [ -z $VERSION ]; then
  VERSION=1
fi
echo "Using kernel version: $VERSION"

temp_kimage=$(make_temp_file)
extract_image_partition ${SRC_BIN} 2 ${temp_kimage}
updated_kimage=$(make_temp_file)

vbutil_kernel --repack "${updated_kimage}" \
  --keyblock "${KERNEL_KEYBLOCK}" \
  --signprivate "${KERNEL_DATAKEY}" \
  --version "${VERSION}" \
  --oldblob "${temp_kimage}"

# Create a copy of the input image and put in the new vblock
cp "${SRC_BIN}" "${DST_BIN}"
replace_image_partition ${DST_BIN} 2 ${updated_kimage}
echo "New signed image was output to ${DST_BIN}"

