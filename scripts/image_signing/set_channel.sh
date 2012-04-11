#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
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
  local rootfs lsb

  rootfs=$(make_temp_dir)
  lsb="${rootfs}/etc/lsb-release"
  mount_image_partition "${image}" 3 "${rootfs}"
  # Get the current channel on the image.
  local from=$(grep '^CHROMEOS_RELEASE_TRACK=' "${lsb}" | cut -d '=' -f 2)
  from=${from%"-channel"}
  echo "Current channel is '${from}'. Changing to '${to}'."

  local sudo
  if [[ ! -w ${lsb} ]] ; then
    sudo="sudo"
  fi
  ${sudo} sed -i "s/\b${from}\b/${to}/" "${lsb}" &&
    echo "Channel change successful."
  cat "${lsb}"
}

main "$@"
