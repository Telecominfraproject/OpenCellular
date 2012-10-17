#!/bin/bash
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment kernel data key for firmware updates.
# Used when revving versions for a firmware update.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
  Usage: $0 <keyset directory>

  Increments the kernel data key in the specified keyset.
EOF
  exit 1
fi

KEY_DIR=$1

main() {
  load_current_versions "${KEY_DIR}"
  new_kernkey_ver=$(increment_version "${KEY_DIR}" "kernel_key_version")

  cd "${KEY_DIR}"
  backup_existing_kernel_data_keys ${CURR_FIRM_VER} ${CURR_KERNKEY_VER}

  cat <<EOF
Generating new kernel data version, and new kernel keyblock.

New Kernel data key version: ${new_kernkey_ver}.
EOF
  make_pair kernel_data_key ${KERNEL_DATAKEY_ALGOID} ${new_kernkey_ver}
  make_keyblock kernel ${KERNEL_KEYBLOCK_MODE} kernel_data_key kernel_subkey

  write_updated_version_file ${CURR_FIRMKEY_VER} ${CURR_FIRM_VER} \
    ${new_kernkey_ver} ${CURR_KERN_VER}
}

main "$@"
