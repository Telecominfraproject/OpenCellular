#!/bin/bash
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/common.sh"

usage() {
  cat <<EOF
Usage: ${0##*/} <number of loem keys to add>

If the existing keyset is not set up for loem usage, it will be converted.

Note: Use 0 if you want to just convert an existing keyset.
EOF
  exit ${1:-0}
}

convert_keyset_to_loem() {
  local f

  printf "Converting to loem keyset; continue? (y/N) "
  read f
  [[ ${f} == [yY] ]]

  for f in {firmware_data,root}_key.vb{pub,priv}k firmware.keyblock; do
    if [[ ${f} == "root_key.vbprivk" && ! -e ${f} ]]; then
      # For official keys, we won't have the private half of the root key.
      echo "Skipping ${f} for official keys"
      continue
    fi
    if [[ ${f} == *.vbprivk && ! -e ${f} ]]; then
      # For official keys, will be gpg wrapped.
      f+=".gpg"
    fi
    mv -i "${f}" "${f/./.loem1.}"
  done

  echo "[loem]" > loem.ini
}

main() {
  set -e -u

  if [[ $# -ne 1 || $1 == -* ]]; then
    usage
  fi

  # Keep `local` and assignment split so return values are checked.
  local firmware_key_version
  local num_keys highest_key k

  if [[ ! -e ${VERSION_FILE} ]]; then
    echo "missing ${VERSION_FILE} in ${PWD}; please create one" >&2
    exit 1
  fi

  firmware_key_version=$(get_version "firmware_key_version")

  # See if we need to convert the keyset first.
  if [[ -e root_key.vbpubk ]]; then
    convert_keyset_to_loem
  fi

  num_keys=$1
  highest_key=$(printf '%s\n' firmware.loem*.keyblock |
                sed -r 's:firmware.loem(.*).keyblock:\1:' |
                sort -n | tail -1)
  echo "There are ${highest_key} loem keys; ading ${num_keys} more"

  for ((k = highest_key + 1; k < highest_key + 1 + num_keys; ++k)); do
    echo "Generating LOEM ${k}"
    make_pair root_key.loem${k} ${ROOT_KEY_ALGOID}
    make_pair firmware_data_key.loem${k} ${FIRMWARE_DATAKEY_ALGOID} \
      ${firmware_key_version}
    make_keyblock firmware.loem${k} ${FIRMWARE_KEYBLOCK_MODE} \
      firmware_data_key.loem${k} root_key.loem${k}
  done

  echo
  echo "Don't forget to update loem.ini to allocate the keys!"
}
main "$@"
