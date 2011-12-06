#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Wrapper script for re-signing a firmware image.

# Determine script directory
SCRIPT_DIR=$(dirname $0)

# Abort on error.
set -e

if [ $# -lt 3 ] || [ $# -gt 4 ]; then
  cat<<EOF
Usage: $0 <input_firmware> <key_dir> <output_firmware> [firmware_version]

Signs <input_firmware> with keys in <key_dir>, setting firmware version
to <firmware_version>. Outputs signed firmware to <output_firmware>.
If no firmware version is specified, it is set as 1.
EOF
  exit 1
fi

IN_FIRMWARE=$1
KEY_DIR=$2
OUT_FIRMWARE=$3
FIRMWARE_VERSION=${4:-1}

temp_fw=$(mktemp)
trap "rm ${temp_fw}" EXIT

# Resign the firmware with new keys
${SCRIPT_DIR}/resign_firmwarefd.sh ${IN_FIRMWARE} ${temp_fw} \
  ${KEY_DIR}/firmware_data_key.vbprivk \
  ${KEY_DIR}/firmware.keyblock \
  ${KEY_DIR}/dev_firmware_data_key.vbprivk \
  ${KEY_DIR}/dev_firmware.keyblock \
  ${KEY_DIR}/kernel_subkey.vbpubk \
  ${FIRMWARE_VERSION}

# Replace the root and recovery key in the Google Binary Block of the firmware.
# Note: This needs to happen after calling resign_firmwarefd.sh since it needs
# to be able to verify the firmware using the root key to determine the preamble
# flags.
gbb_utility -s \
  --rootkey=${KEY_DIR}/root_key.vbpubk \
  --recoverykey=${KEY_DIR}/recovery_key.vbpubk \
  ${temp_fw} ${OUT_FIRMWARE}

