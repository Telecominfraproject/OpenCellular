#!/bin/bash
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Wrapper script for signing firmware image using cbootimage.

# Determine script directory.
SCRIPT_DIR=$(dirname "$0")

# Load common constants and variables.
. "${SCRIPT_DIR}/common_minimal.sh"

# Abort on error.
set -e

usage() {
  cat<<EOF
Usage: $0 <type> <pkc_key> <firmware_image> <soc>

Signs <firmware_image> of <type> with <pkc_key> using cbootimage for <soc>.
where type is one of
      bootloader = sign bootloader image
      lp0_firmware = sign lp0 firmware
EOF
  exit 1
}

# Signs bootloader image using pkc_key provided for given soc
# Args: TYPE=bootloader PKC_KEY FIRMWARE_IMAGE SOC
sign_bootloader() {
  local type=$1
  local pkc_key="$(readlink -f "$2")"
  local firmware_image="$(readlink -f "$3")"
  local soc=$4

  local work_dir=$(make_temp_dir)
  local config_file=$(make_temp_file)
  local signed_fw=$(make_temp_file)

  pushd "${work_dir}" >/dev/null

  # Get bootloader length.
  #
  # Example:
  # $ bct_dump image.fastboot.bin
  # Version       = 0x00210001;
  # BlockSize     = 0x00008000;
  # ...
  # ...
  # # Bootloader[0].Length       = 69324;
  # ...
  # ...
  #
  # then, bl_length=69324 (size of bootloader that needs to be signed)
  local bl_length=$(bct_dump "${firmware_image}" | \
      sed -n '/Bootloader\[0\].Length/{ s/.*=\s*//; s/;//; p; q}')

  # Extract bootloader to sign.
  dd \
    if="${firmware_image}" \
    of="${signed_fw}.bl.tosig" \
    count="${bl_length}" \
    ibs=1 \
    skip=32768 >/dev/null 2>&1

  # Calculate rsa signature for bootloader.
  openssl \
    dgst -sha256 \
    -sigopt rsa_padding_mode:pss \
    -sigopt rsa_pss_saltlen:-1 \
    -sign "${pkc_key}" \
    -out "${signed_fw}.bl.sig" \
    "${signed_fw}.bl.tosig"

  # Update bootloader's rsa signature, aes hash and bct's aes hash.
  echo "RsaPssSigBlFile = ${signed_fw}.bl.sig;" > "${config_file}"
  echo "RehashBl;" >> "${config_file}"
  cbootimage \
    -s "${soc}" \
    -u "${config_file}" \
    "${firmware_image}" \
    "${signed_fw}.tmp" >/dev/null

  # Extract the part of bct which needs to be rsa signed.
  dd \
    if="${signed_fw}.tmp" \
    of="${signed_fw}.bct.tosig" \
    count=8944 \
    ibs=1 \
    skip=1296 >/dev/null 2>&1

  # Calculate rsa signature for bct.
  openssl \
    dgst -sha256 \
    -sigopt rsa_padding_mode:pss \
    -sigopt rsa_pss_saltlen:-1 \
    -sign "${pkc_key}" \
    -out "${signed_fw}.bct.sig" \
    "${signed_fw}.bct.tosig"

  # Create public key modulus from key file.
  openssl \
    rsa -in "${pkc_key}" \
    -noout \
    -modulus \
    -out "${signed_fw}.key.mod"

  # Remove prefix.
  cut \
    -d= \
    -f2 "${signed_fw}.key.mod" > "${signed_fw}.key.mod.tmp1"
  dd \
    if="${signed_fw}.key.mod.tmp1" \
    of="${signed_fw}.key.mod.tmp" \
    count=512 \
    ibs=1 >/dev/null 2>&1

  # Convert from hexdecimal to binary.
  perl -pe 's/([0-9a-f]{2})/chr hex $1/gie' \
    < "${signed_fw}.key.mod.tmp" \
    > "${signed_fw}.key.mod.bin"

  # Update bct's rsa signature and modulus.
  echo "RsaPssSigBctFile = ${signed_fw}.bct.sig;" > "${config_file}"
  echo "RsaKeyModulusFile = ${signed_fw}.key.mod.bin;" >> "${config_file}"
  cbootimage \
    -s "${soc}" \
    -u "${config_file}" \
    "${signed_fw}.tmp" \
    "${signed_fw}" >/dev/null

  # Calculate hash of public key modulus.
  objcopy \
    -I binary \
    --reverse-bytes=256 \
    "${signed_fw}.key.mod.bin" \
    "${signed_fw}.key.mod.bin.rev"
  openssl \
    dgst -sha256 \
    -binary \
    -out "${signed_fw}.key.sha" \
    "${signed_fw}.key.mod.bin.rev"

  popd >/dev/null

  # Copy signed firmware image and public key hash to current directory..
  mv "${signed_fw}" "${firmware_image}"
  mv "${signed_fw}.key.sha" "${firmware_image}.pubkey.sha"
}

# Signs lp0 firmware image using pkc_key provided for given soc
# Args: TYPE=lp0_firmware PKC_KEY FIRMWARE_IMAGE SOC
sign_lp0_firmware() {
  local type=$1
  local pkc_key="$(readlink -f "$2")"
  local firmware_image="$(readlink -f "$3")"
  local soc=$4

  local work_dir=$(make_temp_dir)
  local signed_fw=$(make_temp_file)

  pushd "${work_dir}" >/dev/null

  cp "${firmware_image}" "${signed_fw}"

  # Extract the part of the binary which needs to be signed.
  dd \
    if="${firmware_image}" \
    of="${signed_fw}.tosig" \
    ibs=1 \
    skip=544 >/dev/null 2>&1

  # Calculate rsa-pss signature.
  openssl \
    dgst -sha256 \
    -sigopt rsa_padding_mode:pss \
    -sigopt rsa_pss_saltlen:-1 \
    -sign "${pkc_key}" \
    -out "${signed_fw}.rsa.sig" \
    "${signed_fw}.tosig"

  # Reverse rsa signature to meet tegra soc ordering requirement.
  objcopy \
    -I binary \
    --reverse-bytes=256 \
    "${signed_fw}.rsa.sig" \
    "${signed_fw}.rsa.sig.rev"

  # Inject rsa-pss signature into the binary image's header.
  dd \
    if="${signed_fw}.rsa.sig.rev" \
    of="${signed_fw}" \
    count=256 \
    obs=1 \
    seek=288 \
    conv=notrunc >/dev/null 2>&1

  # Generate public key modulus from key file.
  openssl \
    rsa -in "${pkc_key}" \
    -noout \
    -modulus \
    -out "${signed_fw}.key.mod"

  # Remove prefix.
  cut \
    -d= \
    -f2 "${signed_fw}.key.mod" > "${signed_fw}.key.mod.tmp1"

  dd \
    if="${signed_fw}.key.mod.tmp1" \
    of="${signed_fw}.key.mod.tmp" \
    count=512 \
    ibs=1 >/dev/null 2>&1

  # Convert from hexdecimal to binary.
  perl -pe 's/([0-9a-f]{2})/chr hex $1/gie' \
    < "${signed_fw}.key.mod.tmp" \
    > "${signed_fw}.key.mod.bin"

  # Reverse byte order.
  objcopy \
    -I binary \
    --reverse-bytes=256 \
    "${signed_fw}.key.mod.bin" \
    "${signed_fw}.key.mod.bin.rev"

  # Inject public key modulus into the binary image's header.
  dd \
    if="${signed_fw}.key.mod.bin.rev" \
    of="${signed_fw}" \
    count=256 \
    obs=1 \
    seek=16 \
    conv=notrunc >/dev/null 2>&1

  popd >/dev/null
  mv "${signed_fw}" "${firmware_image}"
}

main() {
  if [[ $# -ne 4 ]]; then
    usage
  fi

  local type=$1

  case ${type} in
    bootloader)
      sign_bootloader "$@"
      ;;
    lp0_firmware)
      sign_lp0_firmware "$@"
      ;;
    *)
      usage
      ;;
  esac
}

main "$@"
