#!/bin/bash
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment kernel subkey for firmware updates.
# Used when revving versions for a firmware update.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
  Usage: $0 <keyset directory>

  Increments the kernel subkey in the specified keyset.
EOF
  exit 1
fi

KEY_DIR=$1

main() {
  load_current_versions "${KEY_DIR}"
  new_firm_ver=$(increment_version "${KEY_DIR}" "firmware_version")

  cd "${KEY_DIR}"
  backup_existing_kernel_subkeys ${CURR_FIRM_VER} ${CURR_KERNKEY_VER}

  cat <<EOF
Generating new kernel subkey and new kernel keyblock.

New Firmware version (due to kernel subkey change): ${new_firm_ver}.
EOF
  make_pair kernel_subkey ${KERNEL_SUBKEY_ALGOID} ${new_firm_ver}
  make_keyblock kernel $KERNEL_KEYBLOCK_MODE kernel_data_key kernel_subkey

  write_updated_version_file ${CURR_FIRMKEY_VER} ${new_firm_ver} \
    ${CURR_KERNKEY_VER} ${CURR_KERN_VER}
}

main "$@"
