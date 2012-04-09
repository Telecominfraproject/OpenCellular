#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to convert a recovery image into an SSD image usable by factory.

# TODO(gauravsh): crosbug.com/14790 (Merge this with
#                 convert_recovery_to_ssd.sh)

# Load common constants and variables.
. "$(dirname "$0")/common_minimal.sh"

usage() {
  cat <<EOF
Usage: $PROG <signed_recovery_image> <original_image_zip> <output_ssd_image>

Converts <signed_recovery_image> into a full SSD image usable by factory. Uses
stateful partition from SSD image <original_image_zip>.
EOF
}

if [ $# -ne 3 ]; then
  usage
  exit 1
fi

type -P cgpt &>/dev/null ||
  { echo "cgpt tool must be in the path"; exit 1; }

# Abort on errors.
set -e

RECOVERY_IMAGE=$1
IMAGE_ZIP=$2
SSD_IMAGE=$3

work_dir=$(make_temp_dir)

echo "Extracting original SSD image."
unzip -o $IMAGE_ZIP chromiumos_base_image.bin -d ${work_dir}

mv ${work_dir}/chromiumos_base_image.bin ${SSD_IMAGE}

kerna_offset=$(partoffset ${RECOVERY_IMAGE} 2)
kernb_offset=$(partoffset ${RECOVERY_IMAGE} 4)
# Kernel partition sizes should be the same.
kern_size=$(partsize ${RECOVERY_IMAGE} 2)

rootfs=$(make_temp_file)
echo "Replacing RootFS on the SSD with that of the RECOVERY image"
extract_image_partition ${RECOVERY_IMAGE} 3 ${rootfs}
replace_image_partition ${SSD_IMAGE} 3 ${rootfs}

kerna=$(make_temp_file)
echo "Replacing KernelA on the SSD with that of the RECOVERY image"
extract_image_partition ${RECOVERY_IMAGE} 4 ${kerna}
replace_image_partition ${SSD_IMAGE} 2 ${kerna}

# Overwrite the kernel vblock on the created SSD image.
stateful_dir=$(make_temp_dir)
tmp_vblock=$(make_temp_file)
mount_image_partition_ro ${RECOVERY_IMAGE} 1 ${stateful_dir}
sudo cp ${stateful_dir}/vmlinuz_hd.vblock ${tmp_vblock}
echo "Overwriting kernel vblock with SSD kernel vblock"
sudo dd if=${tmp_vblock} of=${SSD_IMAGE} seek=${kerna_offset} bs=512 \
  conv=notrunc
sudo umount ${stateful_dir}

# Zero out Kernel B partition.
echo "Zeroing out Kernel partition B"
sudo dd if=/dev/zero of=${SSD_IMAGE} seek=${kernb_offset} bs=512 \
  count=${kern_size} conv=notrunc
echo "${RECOVERY_IMAGE} was converted to a factory SSD image: ${SSD_IMAGE}"
