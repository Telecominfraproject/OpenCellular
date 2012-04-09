#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
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
DEFINE_string app_id "{87efface-864d-49a5-9bb3-4b050a7c227a}" \
  "App ID to use when pinging the Omaha server for the latest version."
DEFINE_string channel "dev-channel" \
  "Release channel of the build."
DEFINE_string hardware_id "IEC MARIO PONY 6101" \
  "The Hardware ID of the latest shipping image to use as a template."
DEFINE_string image "" \
  "The image that needs to be aligned to the latest shipping image."

IMAGE_SEARCH_STRING=
RELEASE_URL=
RELEASE_ALT_URL=

# Sets up environment variables specific to the board.
initialize() {
  if [ "${FLAGS_hardware_id}" = "IEC MARIO PONY 6101" ]; then
    IMAGE_SEARCH_STRING="*SSD_MP_SIGNED.bin"
    RELEASE_URL="http://chromeos-images/chromeos-official/${FLAGS_channel}/x86-mario"
    RELEASE_ALT_URL="${RELEASE_URL}-rc"
  else
    die "Hardware ID \"${FLAGS_hardware_id}\" not supported"
  fi
}

# Gets the latest shipping version for the specified Hardware ID by pinging
# the Omaha server.  The latest shipping version is printed to stdout.
# The caller must use process substition when invoking this function so we
# use a simple trap for clean up instead of calling add_cleanup_action.
get_latest_shipping_version() {
  info "Pinging Omaha for the latest shipping version."
  local auserver_url="https://tools.google.com/service/update2"
  local au_request_file=$(mktemp "/tmp/align_rootfs_au_request.XXXX")
  trap "rm -f \"${au_request_file}\"" INT TERM EXIT
  cat > "${au_request_file}" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<o:gupdate xmlns:o="http://www.google.com/update2/request"
   version="ChromeOSUpdateEngine-0.1.0.0"
   updaterversion="ChromeOSUpdateEngine-0.1.0.0" protocol="2.0" ismachine="1">
    <o:os version="Indy" platform="Chrome OS" sp="ForcedUpdate_i686"></o:os>
    <o:app appid="${FLAGS_app_id}"
       version="0.0.0.0" lang="en-US" track="${FLAGS_channel}"
       hardware_class="${FLAGS_hardware_id}" delta_okay="true">
        <o:updatecheck></o:updatecheck>
    </o:app>
</o:gupdate>
EOF

  wget -q --header="Content-Type: text/xml" \
    --post-file="${au_request_file}" -O - ${auserver_url} |
    sed 's/.*\(ChromeOSVersion="\)\([0-9\.]*\)"\(.*\)/\2/'
}

# Downloads the image from the specified |URL| using ${IMAGE_SEARCH_STRING} as
# a wildcard match and write the image to |OUTPUT_FILE|.
# Args: URL OUTPUT_FILE
download_image() {
  # Since we don't know the exact name, we'll just download recursively based
  # on a wildcard. Then we'll rename that download file to the desired output
  # file. It's important that the IMAGE_SEARCH_STRING matches only one file.
  local url=$1
  local output_file=$2
  local retcode=0

  local download_dir=$(mktemp -d "/tmp/align_rootfs_download_dir.XXXX")
  add_cleanup_action "sudo rm -rf \"${download_dir}\""

  wget -A "${IMAGE_SEARCH_STRING}" --progress=bar -r -l1 -nd \
    -P "${download_dir}" ${url} || retcode=$?
  if [ ${retcode} -eq 0 ]; then
    mv -f "${download_dir}"/* "${output_file}"
  else
    return ${retcode}
  fi
}

# Retrieves the latest shipping image and saves it to |IMAGE|.
# Args: VERSION IMAGE
get_shipping_image() {
  # The image may reside in one of two URLs. We don't know exactly which one
  # so we'll have to try both.
  local version=$1
  local image=$2
  local url="${RELEASE_URL}/${version}"
  download_image "${url}" "${image}" || retcode=$?
  if [ ${retcode} -gt 0 ]; then
    url="${RELEASE_ALT_URL}/${version}"
    download_image "${url}" "${image}"
  fi
}

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

  # Initialize some variables specific to the board.
  initialize

  # Download the latest shipping image.
  local latest_shipping_version=$(get_latest_shipping_version)
  local latest_shipping_image=$(mktemp "/tmp/align_rootfs_shipping_image.XXXX")
  add_cleanup_action "sudo rm -f \"${latest_shipping_image}\""
  info "Downloading image (${latest_shipping_version})"
  get_shipping_image "${latest_shipping_version}" "${latest_shipping_image}"

  # Make sure the two rootfs are the same size.
  # If they are not, then there is nothing for us to do.
  # Note: Exit with a zero code so we do not break the build workflow.
  local shipping_root_fs_size=$(partsize "${latest_shipping_image}" 3)
  local new_root_fs_size=$(partsize "${FLAGS_image}" 3)
  if [ ${shipping_root_fs_size} -ne ${new_root_fs_size} ]; then
    warn "The latest shipping rootfs and the new rootfs are not the same size."
    exit 0
  fi

  # Extract the rootfs from the shipping image and use this as a template
  # for the new image.
  temp_root_fs=$(mktemp "/tmp/align_rootfs_temp_rootfs.XXXX")
  add_cleanup_action "sudo rm -f \"${temp_root_fs}\""
  info "Extracting rootfs from shipping image"
  extract_image_partition "${latest_shipping_image}" 3 "${temp_root_fs}"
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
