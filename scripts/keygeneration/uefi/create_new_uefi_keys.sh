#!/bin/bash

# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/uefi_common.sh"

usage() {
  cat <<EOF
Usage: ${PROG} <OUTPUT_DIR>

Generate key pairs for UEFI secure boot.
EOF

  if [[ $# -ne 0 ]]; then
    die "$*"
  else
    exit 0
  fi
}

main() {
  set -e

  while [[ $# -gt 0 ]]; do
    case $1 in
    -h|--help)
      usage
      ;;
    -*)
      usage "Unknown option: $1"
      ;;
    *)
      break
      ;;
    esac
  done

  if [[ $# -ne 1 ]]; then
    usage "Missing output directory"
  fi

  local dir="$1"

  check_uefi_key_dir_name "${dir}"
  pushd "${dir}" > /dev/null

  if [[ ! -e "${UEFI_VERSION_FILE}" ]]; then
    echo "No version file found. Creating default ${UEFI_VERSION_FILE}."
    (
      printf '%s_key_version=1\n' {pk,kek,db,db_child}
    ) > "${UEFI_VERSION_FILE}"
  fi

  local pk_key_version kek_key_version db_key_version db_child_key_version

  # Get the key versions for normal keypairs
  pk_key_version=$(get_uefi_version "pk_key_version")
  kek_key_version=$(get_uefi_version "kek_key_version")
  db_key_version=$(get_uefi_version "db_key_version")
  db_child_key_version=$(get_uefi_version "db_child_key_version")

  make_pk_keypair "${pk_key_version}"
  make_kek_keypair "${kek_key_version}"
  make_db_keypair "${db_key_version}"
  make_db_child_keypair "${db_key_version}" "${db_child_key_version}"

  popd > /dev/null
}

main "$@"
