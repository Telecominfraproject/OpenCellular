#!/bin/bash

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/../common.sh"

usage() {
  cat <<EOF
Usage: ${PROG} DIR

DIR: To generate a keypair from an RSA 3072 key (.pem file) for Hammer at DIR

EOF

  if [[ $# -ne 0 ]]; then
    die "$*"
  else
    exit 0
  fi
}

# Generate a keypair at the given directory.
generate_key() {
  local dir=$1

  # Generate RSA key.
  openssl genrsa -3 -out "${dir}/temp.pem" 3072

  # Create a keypair from an RSA .pem file generated above.
  futility create "${dir}/temp.pem" "${dir}/key_hammer"

  # Best attempt to securely delete the temp.pem file.
  shred --remove "${dir}/temp.pem"
}

main() {
  set -e

  local dir

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
  dir="$1"

  generate_key "${dir}"
}

main "$@"
