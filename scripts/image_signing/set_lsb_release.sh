#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Customizes a Chrome OS release image by setting /etc/lsb-release values.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set_lsb_release_keyval() {
  local rootfs=$1
  local key=$2
  local value=$3
  local temp_lsb_release="$rootfs/etc/temp-lsb-release"
  echo "$key=$value" | sudo tee "$temp_lsb_release" > /dev/null
  grep -Ev "^$key=" "$rootfs/etc/lsb-release" \
    | sudo tee -a "$temp_lsb_release" > /dev/null
  sudo sort -o "$rootfs/etc/lsb-release" "$temp_lsb_release"
  sudo rm -f "$temp_lsb_release"
}

main() {
  set -e

  local image=$1
  local key=$2
  local value=$3
  if [ $# -ne 1 ] && [ $# -ne 3 ]; then
    cat <<EOF
Usage: $PROG <image.bin> [<key> <value>]

Examples:

$ $PROG chromiumos_image.bin

Dumps /etc/lsb-release from chromiumos_image.bin to stdout.

$ $PROG chromiumos_image.bin CHROMEOS_RELEASE_DESCRIPTION "New description"

Sets the CHROMEOS_RELEASE_DESCRIPTION key's value to "New description"
in /etc/lsb-release in chromiumos_image.bin, sorts the keys and dumps
the updated file to stdout.

EOF
    exit 1
  fi

  local rootfs=$(make_temp_dir)
  mount_image_partition_ro "$image" 3 "$rootfs"
  if [ -n "$key" ]; then
    sudo umount "$rootfs"
    mount_image_partition "$image" 3 "$rootfs"
    set_lsb_release_keyval "$rootfs" "$key" "$value"
    touch "$image"  # Updates the image modification time.
  fi
  cat "$rootfs/etc/lsb-release"
}

main "$@"
