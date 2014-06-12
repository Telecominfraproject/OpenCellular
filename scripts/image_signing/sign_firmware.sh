#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Wrapper script for re-signing a firmware image.

# Determine script directory.
SCRIPT_DIR=$(dirname "$0")

# Abort on error.
set -e

usage() {
  cat<<EOF
Usage: $0 <input_firmware> <key_dir> <output_firmware> [firmware_version]

Signs <input_firmware> with keys in <key_dir>, setting firmware version
to <firmware_version>. Outputs signed firmware to <output_firmware>.
If no firmware version is specified, it is set as 1.
EOF
  exit 1
}

main() {
  if [[ $# -lt 3 || $# -gt 4 ]]; then
    usage
  fi

  local in_firmware=$1
  local key_dir=$2
  local out_firmware=$3
  local firmware_version=${4:-1}

  local temp_fw=$(mktemp)
  trap "rm -f '${temp_fw}'" EXIT

  # Resign the firmware with new keys.
  "${SCRIPT_DIR}/resign_firmwarefd.sh" \
    "${in_firmware}" \
    "${temp_fw}" \
    "${key_dir}/firmware_data_key.vbprivk" \
    "${key_dir}/firmware.keyblock" \
    "${key_dir}/dev_firmware_data_key.vbprivk" \
    "${key_dir}/dev_firmware.keyblock" \
    "${key_dir}/kernel_subkey.vbpubk" \
    "${firmware_version}"

  # Replace the root and recovery key in the Google Binary Block of the
  # firmware.  Note: This needs to happen after calling resign_firmwarefd.sh
  # since it needs to be able to verify the firmware using the root key to
  # determine the preamble flags.
  gbb_utility -s \
    --rootkey="${key_dir}/root_key.vbpubk" \
    --recoverykey="${key_dir}/recovery_key.vbpubk" \
    "${temp_fw}" "${out_firmware}"
}
main "$@"
