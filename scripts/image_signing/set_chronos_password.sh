#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Customizes a Chrome OS release image by setting the chronos user password.

# Usage: ./set_chronos_password.sh <image.bin> <chronos_password>

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

readonly ROOTFS_DIR=$(mktemp -d)

cleanup() {
  set +e
  echo Cleaning up...
  sudo umount -d "$ROOTFS_DIR"
  rm -rf "$ROOTFS_DIR"
}

failure() {
  cleanup
  exit 1
}

change_chronos_password() {
  local password=$1
  echo "Changing chronos password to '$password'..."
  local crypted_password="$(echo $password | openssl passwd -1 -stdin)"
  local temp_shadow="$ROOTFS_DIR/etc/tempshadow"
  echo "chronos:$crypted_password:14500:0:99999::::" \
    | sudo tee "$temp_shadow" > /dev/null
  grep -Ev ^chronos: "$ROOTFS_DIR/etc/shadow" \
    | sudo tee -a "$temp_shadow" > /dev/null
  sudo mv -f "$temp_shadow" "$ROOTFS_DIR/etc/shadow"
}

main() {
  local image=$1
  local chronos_password=$2
  if [ $# -ne 2 ]; then
    echo "Usage: $0 <image.bin> <chronos_password>"
    exit 1
  fi

  set -e
  trap failure EXIT
  mount_image_partition "$image" 3 $ROOTFS_DIR
  change_chronos_password "$chronos_password"
  cleanup
  touch "$image"  # Updates the image modification time.
  echo "Done."
  trap - EXIT
}

main $@
