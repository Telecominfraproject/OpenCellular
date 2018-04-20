#!/bin/bash
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment UEFI DB key.

# Load common constants and variables.
. "$(dirname "$0")/uefi_common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
  Usage: $0 <keyset directory>

  Increments the UEFI DB key in the specified keyset.
EOF
  exit 1
fi

KEY_DIR="$1"

main() {
  check_uefi_key_dir_name "${KEY_DIR}"

  load_current_uefi_key_versions "${KEY_DIR}"
  new_db_key_ver=$(increment_uefi_version "${KEY_DIR}" "db_key_version")
  new_db_child_key_ver=1

  cd "${KEY_DIR}"
  backup_existing_db_keypair_and_children "${CURR_DB_KEY_VER}"

  cat <<EOF
Generating new UEFI DB key version.

New DB key version: ${new_db_key_ver}.
EOF
  make_db_keypair "${new_db_key_ver}"
  make_db_child_keypair "${new_db_key_ver}" "${new_db_child_key_ver}"
  write_updated_uefi_version_file "${CURR_PK_KEY_VER}" "${CURR_KEK_KEY_VER}" \
      "${new_db_key_ver}" "${new_db_child_key_ver}"
}

main "$@"
