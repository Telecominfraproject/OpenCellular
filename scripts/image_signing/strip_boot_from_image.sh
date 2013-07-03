#!/bin/bash

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to remove /boot directory from an image.

# Load common constants.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

load_shflags

DEFINE_string image "chromiumos_image.bin" \
  "Input file name of Chrome OS image to strip /boot from."

# Parse command line.
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Abort on error.
set -e

if [ -z "${FLAGS_image}" ] || [ ! -s "${FLAGS_image}" ] ; then
  die "Error: need a valid file by --image"
fi

# Swiped/modifed from $SRC/src/scripts/base_library/base_image_util.sh.
zero_free_space() {
  local rootfs="$1"

  echo "Zeroing freespace in ${rootfs}"
  # dd is a silly thing and will produce a "No space left on device" message
  # that cannot be turned off and is confusing to unsuspecting victims.
  ( sudo dd if=/dev/zero of="${rootfs}/filler" bs=4096 conv=fdatasync \
      status=noxfer || true ) 2>&1 | grep -v "No space left on device"
  sudo rm "${rootfs}/filler"
}


strip_boot() {
  local image=$1

  # Mount image so we can modify it.
  local rootfs_dir=$(make_temp_dir)
  mount_image_partition ${image} 3 ${rootfs_dir}

  sudo rm -rf "${rootfs_dir}/boot" &&
    echo "/boot directory was removed."

  # To prevent the files we just removed from the FS from remaining as non-
  # zero trash blocks that bloat payload sizes, need to zero them. This was
  # done when the image was built, but needs to be repeated now that we've
  # modified it in a non-trivial way.
  zero_free_space "${rootfs_dir}"
}


IMAGE=$(readlink -f "${FLAGS_image}")
if [[ -z "${IMAGE}" || ! -f "${IMAGE}" ]]; then
  die "Missing required argument: --from (image to update)"
fi

strip_boot "${IMAGE}"
