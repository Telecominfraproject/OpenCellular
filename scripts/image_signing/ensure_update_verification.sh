#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Verify that update payload verification is enabled.

# Abort on error.
set -e

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

usage() {
  echo "Usage: $PROG image"
}

main() {
  if [ $# -ne 1 ]; then
    usage
    exit 1
  fi

  local image=$1
  local rootfs=$(make_temp_dir)
  local key_location="/usr/share/update_engine/update-payload-key.pub.pem"
  mount_image_partition_ro "$image" 3 "$rootfs"
  if [ ! -e "$rootfs/$key_location" ]; then
    die "Update payload verification key not found at $key_location"
  fi
}

main "$@"
