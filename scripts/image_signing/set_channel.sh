#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Changes the channel on a Chrome OS image.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set -e

if [ $# -ne 2 ]; then
  cat <<EOF
Usage: $PROG <image.bin> <channel>

<image.bin>: Path to image.
<channel>: The new channel of the image.
EOF
  exit 1
fi

main() {
  local image=$1
  local to=$2
  
  rootfs=$(make_temp_dir)
  mount_image_partition "${image}" 3 "${rootfs}"
  # Get the current channel on the image.
  local from=$(grep '^CHROMEOS_RELEASE_TRACK=' \
    "${rootfs}/etc/lsb-release" | cut -d '=' -f 2)
  from=${from%"-channel"}
  echo "Current channel is '${from}'. Changing to '${to}'."
  sed -i "s/\b${from}\b/${to}/" "${rootfs}/etc/lsb-release" &&
    echo "Channel change successful."
  cat "${rootfs}/etc/lsb-release"
}

main "$@"
