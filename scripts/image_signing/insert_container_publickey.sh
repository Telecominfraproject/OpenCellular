#!/bin/bash
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

load_shflags || exit 1

FLAGS_HELP="Usage: ${PROG} <image.bin|rootfs_dir> <public_key.pem>

Installs the container verification public key <public_key.pem> to
<image.bin|rootfs_dir>.
"

# Parse command line.
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Abort on error.
set -e

main() {
  if [[ $# -ne 2 ]]; then
    flags_help
    exit 1
  fi

  local image="$1"
  local pub_key="$2"
  local rootfs
  local key_location="/usr/share/misc/"

  if [[ -d "${image}" ]]; then
    rootfs="${image}"
  else
    rootfs=$(make_temp_dir)
    mount_image_partition "${image}" 3 "${rootfs}"
  fi

  sudo install \
    -D -o root -g root -m 644 \
    "${pub_key}" "${rootfs}/${key_location}/oci-container-key-pub.pem"
  info "Container verification key was installed." \
       "Do not forget to resign the image!"
}

main "$@"
