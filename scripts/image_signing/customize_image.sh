#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Customize a Chrome OS release image. The cgpt utility must be on the
# sudo path.
#
# The following changes are applied:
# - Set the root password.

# Usage: ./customize_image.sh <image.bin> <root_password>

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

readonly ROOTFS_DIR=$(mktemp -d)
readonly GPT=cgpt

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

change_root_password() {
  local password=$1
  echo "Changing root password to '$password'..."
  local crypted_password="$(echo $password | openssl passwd -1 -stdin)"
  local temp_shadow="$ROOTFS_DIR/etc/tempshadow"
  echo "root:$crypted_password:14500:0:::::" \
    | sudo tee "$temp_shadow" > /dev/null
  grep -Ev ^root: "$ROOTFS_DIR/etc/shadow" \
    | sudo tee -a "$temp_shadow" > /dev/null
  sudo mv -f "$temp_shadow" "$ROOTFS_DIR/etc/shadow"
}

main() {
  local image=$1
  local root_password=$2
  if [ $# -ne 2 ]; then
    echo "Usage: $0 <image.bin> <root_password>"
    exit 1
  fi

  set -e
  trap failure EXIT
  mount_image_partition "$image" 3 $ROOTFS_DIR
  change_root_password "$root_password"
  cleanup
  echo "Done."
  trap - EXIT
}

main $@
