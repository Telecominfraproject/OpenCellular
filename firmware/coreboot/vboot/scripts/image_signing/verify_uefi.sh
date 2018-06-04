#!/bin/bash
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/common.sh"

set -e

usage() {
  cat <<EOF
Usage: $PROG /path/to/target/dir /path/to/esp/dir /path/to/uefi/keys/dir

Verify signatures of UEFI binaries in the target directory.
EOF
  if [[ $# -gt 0 ]]; then
    error "$*"
    exit 1
  fi
  exit 0
}

main() {
  local target_dir="$1"
  local esp_dir="$2"
  local key_dir="$3"

  if [[ $# -ne 3 ]]; then
    usage "command takes exactly 3 args"
  fi

  if ! type -P sbverify &>/dev/null; then
    die "Cannot verify UEFI signatures (sbverify not found)."
  fi

  local bootloader_dir="${target_dir}/efi/boot"
  local syslinux_dir="${target_dir}/syslinux"
  local kernel_dir="${target_dir}"
  local gsetup_dir="${esp_dir}/EFI/Google/GSetup"

  if [[ ! -f "${gsetup_dir}/pk/pk.der" ]]; then
    die "No PK cert"
  fi

  local db_cert_der="${gsetup_dir}/db/db.der"
  if [[ ! -f "${db_cert_der}" ]]; then
    die "No DB cert"
  fi

  local cert="${key_dir}/db/db.pem"

  local working_dir="$(make_temp_dir)"
  local gsetup_cert="${working_dir}/cert.pem"
  openssl x509 -in "${db_cert_der}" -inform DER \
      -out "${gsetup_cert}" -outform PEM

  for efi_file in "${bootloader_dir}"/*.efi; do
    if [[ ! -f "${efi_file}" ]]; then
      continue
    fi
    sbverify --cert "${cert}" "${efi_file}" ||
        die "Verification failed. file:${efi_file} cert:${cert}"
    sbverify --cert "${gsetup_cert}" "${efi_file}" ||
        die "Verification failed. file:${efi_file} cert:${gsetup_cert}"
  done

  for syslinux_kernel_file in "${syslinux_dir}"/vmlinuz.?; do
    if [[ ! -f "${syslinux_kernel_file}" ]]; then
      continue
    fi
    sbverify --cert "${cert}" "${syslinux_kernel_file}" ||
        warn "Verification failed. file:${syslinux_kernel_file} cert:${cert}"
    sbverify --cert "${gsetup_cert}" "${syslinux_kernel_file}" ||
        warn "Verification failed. file:${syslinux_kernel_file}" \
            "cert:${gsetup_cert}"
  done

  local kernel_file="$(readlink -f "${kernel_dir}/vmlinuz")"
  if [[ -f "${kernel_file}" ]]; then
    sbverify --cert "${cert}" "${kernel_file}" ||
        warn "Verification failed: file:${kernel_file} cert:${cert}"
    sbverify --cert "${gsetup_cert}" "${kernel_file}" ||
        warn "Verification failed: file:${kernel_file} cert:${gsetup_cert}"
  fi
}

main "$@"
