#!/bin/bash
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/common.sh"

load_shflags || exit 1

DEFINE_string output "" \
  "Where to write signed output to (default: sign in-place)"

FLAGS_HELP="Usage: ${PROG} [options] <input_image> <key_dir>

Signs <input_image> with keys in <key_dir>. Should have an imageloader.json
file which imageloader can understand and will use to mount the squashfs
image that provides the container's rootfs and OCI configuration.

Input can be an unpacked imageloader image, or a CRX/ZIP file.
"

# Parse command line.
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Abort on error.
set -e

# Sign the directory holding OCI container(s).  We look for an imageloader.json
# file.
sign_oci_container() {
  [[ $# -eq 3 ]] || die "Usage: sign_oci_container <input> <key> <output>"
  local input="${1%/}"
  local key_file="$2"
  local output="$3"

  if [[ "${input}" != "${output}" ]]; then
    rsync -a "${input}/" "${output}/"
  fi

  local manifest out_manifest
  while read -d $'\0' -r manifest; do
    out_manifest="${output}/${manifest%.json}.sig.2"
    manifest="${input}/${manifest}"
    info "Signing: ${manifest}"
    if ! openssl dgst -sha256 -sign "${key_file}" \
                      -out "${out_manifest}" "${manifest}"; then
      die "Failed to sign"
    fi
  done < <(find "${input}/" -name imageloader.json -printf '%P\0')
}

# Sign the crx/zip holding OCI container(s).  We look for an imageloader.json
# file.
sign_oci_container_zip() {
  [[ $# -eq 3 ]] || die "Usage: sign_oci_container_zip <input> <key> <output>"
  local input="$1"
  local key_file="$2"
  local output="$3"
  local tempdir=$(make_temp_dir)

  info "Unpacking archive: ${input}"
  unzip -q "${input}" -d "${tempdir}"

  sign_oci_container "${tempdir}" "${key_file}" "${tempdir}"

  rm -f "${output}"
  info "Packing archive: ${output}"
  (
    cd "${tempdir}"
    zip -q -r - ./
  ) >"${output}"
}

main() {
  if [[ $# -ne 2 ]]; then
    flags_help
    exit 1
  fi

  local input="${1%/}"
  local key_dir="$2"

  local key_file="${key_dir}/cros-oci-container.pem"
  if [[ ! -e "${key_file}" ]]; then
    die "Missing key file: ${key_file}"
  fi

  : "${FLAGS_output:=${input}}"

  if [[ -f "${input}" ]]; then
    sign_oci_container_zip "${input}" "${key_file}" "${FLAGS_output}"
  else
    sign_oci_container "${input}" "${key_file}" "${FLAGS_output}"
  fi
}
main "$@"
