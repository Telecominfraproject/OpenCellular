#!/bin/bash

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/../common.sh"

usage() {
  cat <<EOF
Usage: ${PROG} DIR

DIR: To generate a keypair from an RSA 3072 key (.pem file) for Rose at DIR

EOF

  if [[ $# -ne 0 ]]; then
    die "$*"
  else
    exit 0
  fi
}

# Generate a keypair from hammer's script at the given directory.
generate_key() {
  local dir=$1
  TMP=$(mktemp -d --suffix=.create_rose_keys)

  ./create_new_hammer_keys.sh "${TMP}"
  if [[ $? -ne 0 ]]; then
    die "Failed to call create_new_hammer_keys.sh."
  fi

  mv "${TMP}/key_hammer.vbprik2" "${dir}/key_rose.vbprik2"
  mv "${TMP}/key_hammer.vbpubk2" "${dir}/key_rose.vbpubk2"
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
