#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Wrapper script for re-signing a firmware image.

# Determine script directory
SCRIPT_DIR=$(dirname $0)

# Abort on error.
set -e

FIRMWARE_VERSION=1

if [ $# -ne 3 ]; then
  cat<<EOF
Usage: $0 <input_firmware> <key_dir> <output_firmware>

Signs <input_firmware> with keys in <key_dir> and outputs signed firmware to 
<output_firmware>.
EOF
  exit 1
fi

IN_FIRMWARE=$1
KEY_DIR=$2
OUT_FIRMWARE=$3

temp_fw=$(mktemp)
trap "rm ${temp_fw}" EXIT

# Replace the root and recovery key in the Google Binary Block of the firmware.
gbb_utility -s \
  --rootkey=${KEY_DIR}/root_key.vbpubk \
  --recoverykey=${KEY_DIR}/recovery_key.vbpubk \
  ${IN_FIRMWARE} ${temp_fw}

# Resign the firmware with new keys
${SCRIPT_DIR}/resign_firmwarefd.sh ${temp_fw} ${OUT_FIRMWARE} \
  ${KEY_DIR}/firmware_data_key.vbprivk \
  ${KEY_DIR}/firmware.keyblock \
  ${KEY_DIR}/dev_firmware_data_key.vbprivk \
  ${KEY_DIR}/dev_firmware.keyblock \
  ${KEY_DIR}/kernel_subkey.vbpubk \
  ${FIRMWARE_VERSION}
