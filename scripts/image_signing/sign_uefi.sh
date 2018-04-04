#!/bin/bash

# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/common.sh"

set -e

usage() {
  cat <<EOF
Usage: $PROG /path/to/target/dir /path/to/keys/dir

Sign UEFI binaries in the target directory.
EOF
  if [[ $# -gt 0 ]]; then
    error "$*"
    exit 1
  fi
  exit 0
}

sign_efi_file() {
  local target="$1"
  local temp_dir="$2"
  local priv_key="$3"
  local sign_cert="$4"
  local verify_cert="$5"
  if [[ -z "${verify_cert}" ]]; then
    verify_cert="${sign_cert}"
  fi

  info "Signing efi file ${target}"
  sudo sbattach --remove "${target}" || true
  local signed_file="${temp_dir}/$(basename "${target}")"
  sbsign --key="${priv_key}" --cert="${sign_cert}" \
      --output="${signed_file}" "${target}" || warn "Cannot sign ${target}"
  if [[ -f "${signed_file}" ]]; then
    sudo cp -f "${signed_file}" "${target}"
    sbverify --cert "${verify_cert}" "${target}" || die "Verification failed"
  fi
}

main() {
  local target_dir="$1"
  local key_dir="$2"

  if [[ $# -ne 2 ]]; then
    usage "command takes exactly 2 args"
  fi

  if ! type -P sbattach &>/dev/null; then
    die "Skip signing UEFI binaries (sbattach not found)."
  fi
  if ! type -P sbsign &>/dev/null; then
    die "Skip signing UEFI binaries (sbsign not found)."
  fi
  if ! type -P sbverify &>/dev/null; then
    die "Skip signing UEFI binaries (sbverify not found)."
  fi

  local bootloader_dir="${target_dir}/efi/boot"
  local syslinux_dir="${target_dir}/syslinux"
  local kernel_dir="${target_dir}"

  local verify_cert="${key_dir}/db/db.pem"
  if [[ ! -f "$verify_cert" ]]; then
    die "No verification cert: ${verify_cert}"
  fi

  local sign_cert="${key_dir}/db/db.children/db_child.pem"
  if [[ ! -f "${sign_cert}" ]]; then
    die "No signing cert: ${sign_cert}"
  fi

  local sign_key="${key_dir}/db/db.children/db_child.rsa"
  if [[ ! -f "${sign_key}" ]]; then
    die "No signing key: ${sign_key}"
  fi

  local working_dir="$(make_temp_dir)"

  for efi_file in "${bootloader_dir}/"*".efi"; do
    if [[ ! -f "${efi_file}" ]]; then
      continue
    fi
    sign_efi_file "${efi_file}" "${working_dir}" \
        "${sign_key}" "${sign_cert}" "${verify_cert}"
  done

  for syslinux_kernel_file in "${syslinux_dir}/vmlinuz."?; do
    if [[ ! -f "${syslinux_kernel_file}" ]]; then
      continue
    fi
    sign_efi_file "${syslinux_kernel_file}" "${working_dir}" \
        "${sign_key}" "${sign_cert}" "${verify_cert}"
  done

  local kernel_file="$(readlink -f "${kernel_dir}/vmlinuz")"
  if [[ -f "${kernel_file}" ]]; then
    sign_efi_file "${kernel_file}" "${working_dir}" \
        "${sign_key}" "${sign_cert}" "${verify_cert}"
  fi
}

main "$@"
