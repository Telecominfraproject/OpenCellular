#!/bin/bash
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/uefi_common.sh"

usage() {
  cat <<EOF
Usage: ${PROG} [options]

Generate key pairs for UEFI secure boot.

Options:
  --output <dir>  Where to write the keys (default is cwd).
                  The base name must be '.../uefi'.
  --no-pk         Do not generate PK.
EOF

  if [[ $# -ne 0 ]]; then
    die "unknown option $*"
  else
    exit 0
  fi
}

main() {
  set -e

  local generate_pk="true"
  local output_dir="${PWD}"

  while [[ $# -gt 0 ]]; do
    case $1 in
    --output)
      output_dir="$2"
      shift
      ;;
    --no-pk)
      info "Will not generate PK."
      generate_pk="false"
      ;;
    -h|--help)
      usage
      ;;
    *)
      usage "Unknown option: $1"
      ;;
    esac
    shift
  done

  check_uefi_key_dir_name "${output_dir}"
  pushd "${output_dir}" >/dev/null || die "Wrong output directory name"

  if [[ ! -e "${UEFI_VERSION_FILE}" ]]; then
    echo "No version file found. Creating default ${UEFI_VERSION_FILE}."
    printf '%s_key_version=1\n' {pk,kek,db,db_child} > "${UEFI_VERSION_FILE}"
  fi

  local pk_key_version kek_key_version db_key_version db_child_key_version

  # Get the key versions for normal keypairs
  pk_key_version=$(get_uefi_version "pk_key_version")
  kek_key_version=$(get_uefi_version "kek_key_version")
  db_key_version=$(get_uefi_version "db_key_version")
  db_child_key_version=$(get_uefi_version "db_child_key_version")

  if [[ "${generate_pk}" == "true" ]]; then
    make_pk_keypair "${pk_key_version}"
  fi
  make_kek_keypair "${kek_key_version}"
  make_db_keypair "${db_key_version}"
  make_db_child_keypair "${db_key_version}" "${db_child_key_version}"

  popd >/dev/null
}

main "$@"
