#!/bin/bash
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment UEFI Key Exchange Key (KEK).

# Load common constants and variables.
. "$(dirname "$0")/uefi_common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
  Usage: $0 <keyset directory>

  Increments the UEFI Key Exchange Key (KEK) in the specified keyset.
EOF
  exit 1
fi

KEY_DIR="$1"

main() {
  check_uefi_key_dir_name "${KEY_DIR}"

  load_current_uefi_key_versions "${KEY_DIR}"
  new_kek_key_ver=$(increment_uefi_version "${KEY_DIR}" "kek_key_version")

  cd "${KEY_DIR}"
  backup_kek_keypair "${CURR_KEK_KEY_VER}"

  cat <<EOF
Generating new UEFI Key Exchange Key (KEK) version.

New Key Exchange Key version: ${new_kek_key_ver}.
EOF
  make_kek_keypair "${new_kek_key_ver}"
  write_updated_uefi_version_file "${CURR_PK_KEY_VER}" "${new_kek_key_ver}" \
      "${CURR_DB_KEY_VER}" "${CURR_DB_CHILD_KEY_VER}"
}

main "$@"
