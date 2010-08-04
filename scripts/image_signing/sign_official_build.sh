#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Sign the final build image using the "official" keys.

# Usage: sign_for_ssd.sh <type> input_image /path/to/keys/dir output_image
# 
# where <type> is one of:
#               ssd  (sign an SSD image)
#               recovery (sign a USB recovery image)               
#               install (sign a factory install image) 

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

if [ $# -ne 4 ]; then
  cat <<EOF
Usage: $0 <type> input_image /path/to/keys/dir output_image"
where <type> is one of:
             ssd  (sign an SSD image)
             recovery (sign a USB recovery image)               
             install (sign a factory install image) 
EOF
  exit 1
fi

# Abort on errors.
set -e

TYPE=$1
INPUT_IMAGE=$2
KEY_DIR=$3
OUTPUT_IMAGE=$4


# Generate the SSD image
sign_for_ssd() {
  ${SCRIPT_DIR}/resign_image.sh ${INPUT_IMAGE} ${OUTPUT_IMAGE} \
    ${KEY_DIR}/kernel_data_key.vbprivk \
    ${KEY_DIR}/kernel.keyblock
  echo "Output signed SSD image to ${OUTPUT_IMAGE}"
}

# Generate the USB (recovery + install) image
sign_for_recovery() {
  ${SCRIPT_DIR}/resign_image.sh ${INPUT_IMAGE} ${OUTPUT_IMAGE} \
    ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    ${KEY_DIR}/recovery_kernel.keyblock 

  # Now generate the installer vblock with the SSD keys.
  temp_kimage=$(mktemp)
  trap "rm -f ${temp_kimage}" EXIT
  temp_out_vb=$(mktemp)
  trap "rm -f ${temp_out_vb}" EXIT
  extract_image_partition ${OUTPUT_IMAGE} 2 ${temp_kimage}
  ${SCRIPT_DIR}/resign_kernel_partition.sh ${temp_kimage} ${temp_out_vb} \
    ${KEY_DIR}/kernel_data_key.vbprivk \
    ${KEY_DIR}/kernel.keyblock

  # Copy the installer vblock to the stateful partition.
  local stateful_dir=$(mktemp -d)
  trap "sudo umount -d $stateful_dir; rm -rf $stateful_dir" EXIT
  mount_image_partition ${OUTPUT_IMAGE} 1 ${stateful_dir}
  sudo cp ${temp_out_vb} ${stateful_dir}/vmlinuz_hd.vblock

  echo "Output signed recovery image to ${OUTPUT_IMAGE}"
}

# Generate the factory install image.
sign_for_factory_install() {
  ${SCRIPT_DIR}/resign_image.sh ${INPUT_IMAGE} ${OUTPUT_IMAGE} \
    ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    ${KEY_DIR}/installer_kernel.keyblock
  echo "Output signed factory install image to ${OUTPUT_IMAGE}"
}

if [ "${TYPE}" == "ssd" ]; then
  sign_for_ssd
elif [ "${TYPE}" == "recovery" ]; then
  sign_for_recovery
elif [ "${TYPE}" == "install" ]; then
  sign_for_factory_install
else
  echo "Invalid type ${TYPE}"
  exit 1
fi
  
  
