#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Install an update payload verification public key to the image.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

main() {
  set -e

  local image="$1"
  local pub_key="$2"
  if [ $# -ne 2 ]; then
    cat <<EOF
Usage: $PROG <image.bin> <au_public_key.pem>
Installs the update verification public key <au_public_key.pem> to <image.bin>.
EOF
    exit 1
  fi
  local rootfs=$(make_temp_dir)
  local key_location="/usr/share/update_engine/"
  mount_image_partition "$image" 3 "$rootfs"
  sudo mkdir -p "$rootfs/$key_location"
  sudo cp "$pub_key" "$rootfs/$key_location/update-payload-key.pub.pem"
  sudo chown root:root "$rootfs/$key_location/update-payload-key.pub.pem"
  sudo chmod 644 "$rootfs/$key_location/update-payload-key.pub.pem"
  echo "AU verification key was installed. Do not forget to resign the image!"
}

main "$@"
