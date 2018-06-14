#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to preserve the on-disk file layout of the specified image and
# the latest shipping image.  This is accomplished by copying the new rootfs
# over a template rootfs (aka the latest shipping image) to preserve as much
# of the metadata from the shipping rootfs as possible. This will ensure
# minimal disk shuffling when applying the auto-update.
#
# Note: This script does not recompute the rootfs hash.

# Load common library.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

load_shflags

# Flags.
DEFINE_string image "" \
  "The image that needs to be aligned to the latest shipping image."
DEFINE_string src_image "" \
  "The image to align against."

# Copies the rootfs from |SRC_IMAGE| to the |DST_ROOT_FS| and preserves as
# much of the file system metadata in |DST_ROOT_FS| as possible.
# Args: SRC_IMAGE DST_ROOT_FS
copy_root_fs() {
  local src_image=$1
  local dst_root_fs=$2

  # Mount the src and dst rootfs.
  local src_root_fs_dir=$(mktemp -d "/tmp/align_root_fs_src_mount_dir.XXXX")
  add_cleanup_action "sudo rm -rf \"${src_root_fs_dir}\""
  mount_image_partition_ro "${src_image}" 3 "${src_root_fs_dir}"
  add_cleanup_action "sudo umount \"${src_root_fs_dir}\""

  local dst_root_fs_dir=$(mktemp -d "/tmp/align_root_fs_dst_mount_dir.XXXX")
  add_cleanup_action "sudo rm -rf \"${dst_root_fs_dir}\""
  sudo mount -o loop "${dst_root_fs}" "${dst_root_fs_dir}" -o loop
  add_cleanup_action "sudo umount \"${dst_root_fs_dir}\""

  # Temporarily make immutable files on the dst rootfs mutable.
  # We'll need to track these files in ${immutable_files} so we can make them
  # mutable again.
  local immutable_files=()
  sudo find "${dst_root_fs_dir}" -xdev -type f |
    while read -r file; do
      immutable=$(sudo lsattr "${file}" | cut -d' ' -f1 | grep -q i ; echo $?)
      if [ $immutable -eq 0 ]; then
        immutable_files=("${immutable_files[@]}" "${file}")
        sudo chattr -i "${file}"
      fi
    done

  # Copy files from the src rootfs over top of dst rootfs.
  # Use the --inplace flag to preserve as much of the file system metadata
  # as possible.
  sudo rsync -v -a -H -A -x --force --inplace --numeric-ids --delete \
      "${src_root_fs_dir}"/ "${dst_root_fs_dir}"

  # Make immutable files immutable again.
  for file in ${immutable_files[*]} ; do
    sudo chattr +i "${file}"
  done

  # Unmount the src and dst root fs so that we can replace the rootfs later.
  perform_latest_cleanup_action
  perform_latest_cleanup_action
  perform_latest_cleanup_action
  perform_latest_cleanup_action
}

# Zeroes the rootfs free space in the specified image.
# Args: IMAGE
zero_root_fs_free_space() {
  local image=$1
  local root_fs_dir=$(mktemp -d "/tmp/align_rootfs_zero_free_space_dir.XXXX")
  add_cleanup_action "sudo rm -rf \"${root_fs_dir}\""
  mount_image_partition "${image}" 3 "${root_fs_dir}"
  add_cleanup_action "sudo umount \"${root_fs_dir}\""

  info "Zeroing free space in rootfs"
  sudo dd if=/dev/zero of="${root_fs_dir}/filler" oflag=sync bs=4096 || true
  sudo rm -f "${root_fs_dir}/filler"
  sudo sync

  perform_latest_cleanup_action
  perform_latest_cleanup_action
}

main() {
  # Parse command line.
  FLAGS "$@" || exit 1
  eval set -- "${FLAGS_ARGV}"

  # Only now can we die on error.  shflags functions leak non-zero error codes,
  # so will die prematurely if 'set -e' is specified before now.
  set -e

  # Make sure we have the required parameters.
  if [ -z "${FLAGS_image}" ];  then
    die "--image is required."
  fi

  if [ ! -f "${FLAGS_image}" ]; then
    die "Cannot find the specified image."
  fi

  if [ -z "${FLAGS_src_image}" ];  then
    die "--src_image is required."
  fi

  if [ ! -f "${FLAGS_src_image}" ]; then
    die "Cannot find the specified source image."
  fi

  # Make sure the two rootfs are the same size.
  # If they are not, then there is nothing for us to do.
  # Note: Exit with a zero code so we do not break the build workflow.
  local src_root_fs_size=$(partsize "${FLAGS_src_image}" 3)
  local new_root_fs_size=$(partsize "${FLAGS_image}" 3)
  if [ ${src_root_fs_size} -ne ${new_root_fs_size} ]; then
    warn "The source rootfs and the new rootfs are not the same size."
    exit 0
  fi

  # Extract the rootfs from the src image and use this as a template
  # for the new image.
  temp_root_fs=$(mktemp "/tmp/align_rootfs_temp_rootfs.XXXX")
  add_cleanup_action "sudo rm -f \"${temp_root_fs}\""
  info "Extracting rootfs from src image"
  extract_image_partition "${FLAGS_src_image}" 3 "${temp_root_fs}"
  enable_rw_mount "${temp_root_fs}"

  # Perform actual copy of the two root file systems.
  info "Copying rootfs"
  copy_root_fs "${FLAGS_image}" "${temp_root_fs}"

  # Replace the rootfs in the new image with the aligned version.
  info "Replacing rootfs"
  replace_image_partition "${FLAGS_image}" 3 "${temp_root_fs}"

  # Zero rootfs free space.
  zero_root_fs_free_space "${FLAGS_image}"
}

main "$@"
