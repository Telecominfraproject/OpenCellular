#!/bin/bash
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment UEFI Platform Key (PK).

# Load common constants and variables.
. "$(dirname "$0")/uefi_common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
  Usage: $0 <keyset directory>

  Increments the UEFI Platform Key (PK) in the specified keyset.
EOF
  exit 1
fi

KEY_DIR="$1"

main() {
  check_uefi_key_dir_name "${KEY_DIR}"

  load_current_uefi_key_versions "${KEY_DIR}"
  new_pk_key_ver=$(increment_uefi_version "${KEY_DIR}" "pk_key_version")

  cd "${KEY_DIR}"
  backup_existing_pk_keypair "${CURR_PK_KEY_VER}"

  cat <<EOF
Generating new UEFI Platform Key (PK) version.

New Platform Key version: ${new_pk_key_ver}.
EOF
  make_pk_keypair "${new_pk_key_ver}"
  write_updated_uefi_version_file "${new_pk_key_ver}" "${CURR_KEK_KEY_VER}" \
      "${CURR_DB_KEY_VER}" "${CURR_DB_CHILD_KEY_VER}"
}

main "$@"
