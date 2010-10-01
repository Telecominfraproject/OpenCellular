#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Print usage string
usage() {
    cat <<EOF
Usage: $PROG dst_image src_image
This will put the root file system from src_image into dst_image.
EOF
}

if [ $# -ne 2 ]; then
  usage
  exit 1
fi

DST_IMAGE=$1
SRC_IMAGE=$2

temp_rootfs=$(make_temp_file)
extract_image_partition ${SRC_IMAGE} 3 ${temp_rootfs}
replace_image_partition ${DST_IMAGE} 3 ${temp_rootfs}
echo "RootFS from ${SRC_IMAGE} was copied into ${DST_IMAGE}"
