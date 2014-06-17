#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Wrapper script for re-signing a firmware image.

# Determine script directory.
SCRIPT_DIR=$(dirname "$0")

# Load common constants and variables.
. "${SCRIPT_DIR}/common_minimal.sh"

# Abort on error.
set -e

usage() {
  cat<<EOF
Usage: $0 <input_firmware> <key_dir> <output_firmware> [firmware_version] \
[loem_output_dir]

Signs <input_firmware> with keys in <key_dir>, setting firmware version
to <firmware_version>. Outputs signed firmware to <output_firmware>.
If no firmware version is specified, it is set as 1.
EOF
  exit 1
}

gbb_update() {
  local in_firmware="$1"
  local key_dir="$2"
  local out_firmware="$3"
  local rootkey="$4"

  # Replace the root and recovery key in the Google Binary Block of the
  # firmware.  Note: This needs to happen after calling resign_firmwarefd.sh
  # since it needs to be able to verify the firmware using the root key to
  # determine the preamble flags.
  gbb_utility \
    -s \
    --recoverykey="${key_dir}/recovery_key.vbpubk" \
    --rootkey="${rootkey}" \
    "${in_firmware}" \
    "${out_firmware}"
}

# Sign a single firmware image.
# ARGS: [loem_key] [loemid]
sign_one() {
  local loem_key="$1"
  local loemid="$2"

  # Resign the firmware with new keys.
  "${SCRIPT_DIR}/resign_firmwarefd.sh" \
    "${in_firmware}" \
    "${temp_fw}" \
    "${key_dir}/firmware_data_key${loem_key}.vbprivk" \
    "${key_dir}/firmware${loem_key}.keyblock" \
    "${key_dir}/dev_firmware_data_key${loem_key}.vbprivk" \
    "${key_dir}/dev_firmware${loem_key}.keyblock" \
    "${key_dir}/kernel_subkey.vbpubk" \
    "${firmware_version}" \
    "" \
    "${loem_output_dir}" \
    "${loemid}"
}

# Process all the keysets in the loem.ini file.
sign_loems() {
  local line loem_section=false loem_index loemid
  local rootkey

  rm -f "${out_firmware}"
  while read line; do
    # Find the [loem] section.
    if ! ${loem_section}; then
      if grep -q "^ *\[loem\] *$" <<<"${line}"; then
        loem_section=true
      fi
      continue
    # Abort when we hit the next section.
    elif [[ ${line} == *"["* ]]; then
      break
    fi

    # Strip comments/whitespace.
    line=$(sed -e 's:#.*::' -e 's:^ *::' -e 's: *$::' <<<"${line}")
    loem_index=$(cut -d= -f1 <<<"${line}" | sed 's: *$::')
    loemid=$(cut -d= -f2 <<<"${line}" | sed 's:^ *::')

    echo "### Processing LOEM ${loem_index} ${loemid}"
    sign_one ".loem${loem_index}" "${loemid}"

    rootkey="${key_dir}/root_key.loem${loem_index}.vbpubk"
    cp "${rootkey}" "${loem_output_dir}/rootkey.${loemid}"

    if [[ ! -e ${out_firmware} ]]; then
      gbb_update "${temp_fw}" "${key_dir}" "${out_firmware}" "${rootkey}"
    fi
    echo
  done <"${key_dir}/loem.ini"
}

main() {
  if [[ $# -lt 3 || $# -gt 5 ]]; then
    usage
  fi

  local in_firmware=$1
  local key_dir=$2
  local out_firmware=$3
  local firmware_version=${4:-1}
  local loem_output_dir=${5:-}

  local temp_fw=$(make_temp_file)

  if [[ -e ${key_dir}/loem.ini ]]; then
    if [[ -z ${loem_output_dir} ]]; then
      err_die "need loem_output_dir w/loem keysets"
    fi
    sign_loems
  else
    sign_one
    gbb_update "${temp_fw}" "${key_dir}" "${out_firmware}" \
      "${key_dir}/root_key.vbpubk"
  fi
}
main "$@"
