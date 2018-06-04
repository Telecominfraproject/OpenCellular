#!/bin/bash
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment firmware version key for firmware updates.
# Used when revving versions for a firmware update.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
  Usage: $0 <keyset directory>

  Increments the firmware version in the specified keyset.
EOF
  exit 1
fi

KEY_DIR=$1

main() {
  load_current_versions "${KEY_DIR}"
  new_firmkey_ver=$(increment_version "${KEY_DIR}" "firmware_key_version")

  cd "${KEY_DIR}"
  backup_existing_firmware_keys ${CURR_FIRM_VER} ${CURR_FIRMKEY_VER}

  cat <<EOF
Generating new firmware version key.

New Firmware key version (due to firmware key change): ${new_firmkey_ver}.
EOF
  make_pair firmware_data_key ${FIRMWARE_DATAKEY_ALGOID} ${new_firmkey_ver}
  make_keyblock firmware ${FIRMWARE_KEYBLOCK_MODE} firmware_data_key root_key

  write_updated_version_file ${new_firmkey_ver} ${CURR_FIRM_VER} \
    ${CURR_KERNKEY_VER} ${CURR_KERN_VER}
}

main "$@"
