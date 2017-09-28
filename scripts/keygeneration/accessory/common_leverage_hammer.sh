#!/bin/bash

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/../common.sh"

usage() {
  cat <<EOF
Usage: ${PROG} [options]

Options:
  -o, --output_dir <dir>:    Where to write the keys (default is cwd)
EOF

  if [[ $# -ne 0 ]]; then
    die "$*"
  else
    exit 0
  fi
}

generate_rsa3072_exp3_key() {
  local output_dir="$1"
  local key_name="$2"

  # Generate RSA key.
  openssl genrsa -3 -out "${output_dir}/temp.pem" 3072

  # Create a keypair from an RSA .pem file generated above.
  futility create "${output_dir}/temp.pem" "${output_dir}/key_${key_name}"

  # Best attempt to securely delete the temp.pem file.
  shred --remove "${output_dir}/temp.pem"
}

# To generate a keypair with the same algorithm of Hammer and rename the kepair
# to specific accessory's name.
leverage_hammer_to_create_key() {
  local output_dir="${PWD}"
  local key_name="$1"
  shift

  while [[ $# -gt 0 ]]; do
    case "$1" in
    -h|--help)
      usage
      ;;
    -o|--output_dir)
      output_dir="$2"
      if [[ ! -d "${output_dir}" ]]; then
        die "output dir ("${output_dir}") doesn't exist."
      fi
      shift
      ;;
    -*)
      usage "Unknown option: "$1""
      ;;
    *)
      usage "Unknown argument "$1""
      ;;
    esac
    shift
  done

  generate_rsa3072_exp3_key "${output_dir}" "${key_name}"
}
