#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Customizes a Chrome OS release image by setting /etc/lsb-release values.

# Usage: ./set_lsb_release.sh <image.bin> [<key> <value>]
#
# If executed with key/value parameters, sets <key>=<value> in
# /etc/lsb-release and then dumps /etc/lsb-release to stdout. If
# executed with no key/value, dumps /etc/lsb-release to stdout. Note
# that the order of the keyvals in /etc/lsb-release may not be
# preserved.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set_lsb_release_keyval() {
  local rootfs=$1
  local key=$2
  local value=$3
  echo "Setting '$key' to '$value'..."
  local temp_lsb_release="$rootfs/etc/temp-lsb-release"
  echo "$key=$value" | sudo tee "$temp_lsb_release" > /dev/null
  grep -Ev "^$key=" "$rootfs/etc/lsb-release" \
    | sudo tee -a "$temp_lsb_release" > /dev/null
  sudo mv -f "$temp_lsb_release" "$rootfs/etc/lsb-release"
}

main() {
  set -e

  local image=$1
  local key=$2
  local value=$3
  if [ $# -ne 1 ] && [ $# -ne 3 ]; then
    echo "Usage: $PROG <image.bin> [<key> <value>]"
    exit 1
  fi

  local rootfs=$(mktemp -d)
  mount_image_partition "$image" 3 "$rootfs"
  trap "sudo umount -d $rootfs; rm -rf $rootfs" EXIT
  if [ -n "$key" ]; then
    set_lsb_release_keyval "$rootfs" "$key" "$value"
    touch "$image"  # Updates the image modification time.
  fi
  echo ===
  cat "$rootfs/etc/lsb-release"
  echo ===
  echo Done.
}

main $@
