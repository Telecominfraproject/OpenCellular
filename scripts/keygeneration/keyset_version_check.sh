#!/bin/bash
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script that sanity checks a keyset to ensure actual key versions
# match those set in key.versions.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
Usage: $0 <keyset directory>

Sanity check a keyset directory for key versions.
EOF
  exit 1
fi

KEY_DIR="$1"
VERSION_FILE="${KEY_DIR}/key.versions"

keyblock_version() {
  local keyblock="$1"
  echo "$(vbutil_keyblock --unpack "${keyblock}" | grep 'Data key version' |
    cut -f 2 -d : | tr -d ' ')"
}

key_version() {
  local key="$1"
  echo "$(vbutil_key --unpack "${key}" | grep 'Key Version' | cut -f 2 -d : |
    tr -d ' ')"
}

# Compare versions and print out error if there is a mismatch.
check_versions() {
  local expected="$1"
  local got="$2"
  local expected_label="$3"
  local got_label="$4"
  if [[ ${expected} != ${got} ]]; then
    echo "ERROR: ${expected_label} version does not match ${got_label} version"
    echo "EXPECTED (${expected_label} version): ${expected}"
    echo "GOT (${got_label} version): ${got}"
    return 1
  fi
  return 0
}

main() {
 local testfail=0

 local expected_kkey="$(get_version kernel_key_version)"
 local expected_fkey="$(get_version firmware_key_version)"
 local expected_firmware="$(get_version firmware_version)"
 local expected_kernel="$(get_version kernel_version)"

 check_versions "${expected_firmware}" "${expected_kkey}" \
   "firmware" "kernel key" || testfail=1

 local got_fkey_keyblock="$(keyblock_version ${KEY_DIR}/firmware.keyblock)"
 local got_fkey="$(key_version ${KEY_DIR}/firmware_data_key.vbpubk)"

 local got_kkey_keyblock="$(keyblock_version ${KEY_DIR}/kernel.keyblock)"
 local got_ksubkey="$(key_version ${KEY_DIR}/kernel_subkey.vbpubk)"
 local got_kdatakey="$(key_version ${KEY_DIR}/kernel_data_key.vbpubk)"

 check_versions "${got_fkey_keyblock}" "${got_fkey}" "firmware keyblock key" \
   "firmware key" || testfail=1
 check_versions "${got_kkey_keyblock}" "${got_ksubkey}" "kernel keyblock key" \
   "kernel subkey" || testfail=1
 check_versions "${got_kdatakey}" "${got_ksubkey}" "kernel data key" \
   "kernel subkey" || testfail=1
 check_versions "${expected_fkey}" "${got_fkey}" "key.versions firmware key" \
   "firmware key" || testfail=1
 check_versions "${expected_kkey}" "${got_kdatakey}" "key.versions kernel key" \
   "kernel datakey" || testfail=1
 check_versions "${expected_kkey}" "${got_ksubkey}" "key.versions kernel key" \
   "kernel subkey" || testfail=1
 exit ${testfail}
}

main "$@"
