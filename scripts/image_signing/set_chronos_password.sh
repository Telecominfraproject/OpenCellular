#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Customizes a Chrome OS release image by setting the chronos user password.

# Usage: ./set_chronos_password.sh <image.bin> <chronos_password>

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

change_chronos_password() {
  local rootfs=$1
  local password=$2
  echo "Changing chronos password to '$password'..."
  local crypted_password="$(echo $password | openssl passwd -1 -stdin)"
  local temp_shadow="$rootfs/etc/tempshadow"
  echo "chronos:$crypted_password:14500:0:99999::::" \
    | sudo tee "$temp_shadow" > /dev/null
  grep -Ev ^chronos: "$rootfs/etc/shadow" \
    | sudo tee -a "$temp_shadow" > /dev/null
  sudo mv -f "$temp_shadow" "$rootfs/etc/shadow"
}

main() {
  set -e

  local image=$1
  local chronos_password=$2
  if [ $# -ne 2 ]; then
    echo "Usage: $PROG <image.bin> <chronos_password>"
    exit 1
  fi

  local rootfs=$(mktemp -d)
  mount_image_partition "$image" 3 "$rootfs"
  trap "sudo umount -d $rootfs; rm -rf $rootfs" EXIT
  change_chronos_password "$rootfs" "$chronos_password"
  touch "$image"  # Updates the image modification time.
  echo "Done."
}

main $@
