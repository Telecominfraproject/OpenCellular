#!/bin/bash

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/common.sh"

usage() {
  cat <<EOF
Usage: ${PROG} DIR

Generate Android's 4 framework key pairs at DIR.  For detail, please refer to
"Certificates and private keys" and "Manually generating keys" in
https://source.android.com/devices/tech/ota/sign_builds.html.

EOF

  if [[ $# -ne 0 ]]; then
    die "$*"
  else
    exit 0
  fi
}

# Use the same SUBJECT used in Nexus.
SUBJECT='/C=US/ST=California/L=Mountain View/O=Google Inc./OU=Android/CN=Android'

# Generate .pk8 and .x509.pem at the given directory.
make_pair() {
  local dir=$1
  local name=$2

  # Generate RSA key.
  openssl genrsa -3 -out "${dir}/temp.pem" 2048

  # Create a certificate with the public part of the key.
  openssl req -new -x509 -key "${dir}/temp.pem" -out "${dir}/${name}.x509.pem" \
    -days 10000 -subj "${SUBJECT}"

  # Create a PKCS#8-formatted version of the private key.
  openssl pkcs8 -in "${dir}/temp.pem" -topk8 -outform DER \
    -out "${dir}/${name}.pk8" -nocrypt

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
  dir=$1

  make_pair "${dir}" platform
  make_pair "${dir}" shared
  make_pair "${dir}" media
  make_pair "${dir}" releasekey
}

main "$@"
