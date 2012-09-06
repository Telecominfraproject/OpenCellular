#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to increment kernel subkey and datakey for firmware updates.
# Used when revving versions for a firmware update.

# Load common constants and variables.
. "${0%/*}"/common.sh

# Abort on errors.
set -e

if [ $# -ne 1 ]; then
  cat <<EOF
Usage: $0 <keyset directory>

Increments the kernel subkey, data key and firmware version in the
specified keyset.
EOF
  exit 1
fi

KEY_DIR=$1

# File to read current versions from.
VERSION_FILE="key.versions"

# ARGS: <version_type>
get_version() {
  local version_type=$1
  version=$(sed -n "s#^${version_type}=\(.*\)#\1#pg" ${VERSION_FILE})
  echo $version
}

# Make backups of existing keys and keyblocks that will be revved.
# Backup format:
# for keys: <key_name>.v<version>.vb{pub|priv}k
# for keyblocks: <keyblock_name>.v<datakey version>.v<subkey version>.keyblock
# Args: SUBKEY_VERSION DATAKEY_VERSION
backup_existing_kernel_keys() {
  subkey_version=$1
  datakey_version=$2
  # --no-clobber to prevent accidentally overwriting existing
  # backups.
  mv --no-clobber kernel_subkey.{vbprivk,"v${subkey_version}.vbprivk"}
  mv --no-clobber kernel_subkey.{vbpubk,"v${subkey_version}.vbpubk"}
  mv --no-clobber kernel_data_key.{vbprivk,"v${datakey_version}.vbprivk"}
  mv --no-clobber kernel_data_key.{vbpubk,"v${datakey_version}.vbpubk"}
  mv --no-clobber kernel.{keyblock,"v${datakey_version}.v${subkey_version}.keyblock"}
}

# Write new key version file with the updated key versions.
# Args: FIRMWARE_KEY_VERSION FIRMWARE_VERSION KERNEL_KEY_VERSION KERNEL_VERSION
write_updated_version_file() {
  local firmware_key_version=$1
  local firmware_version=$2
  local kernel_key_version=$3
  local kernel_version=$4

  cat > ${VERSION_FILE} <<EOF
firmware_key_version=${firmware_key_version}
firmware_version=${firmware_version}
kernel_key_version=${kernel_key_version}
kernel_version=${kernel_version}
EOF
}
  

main() {
  local key_dir=$1
  cd "${key_dir}"
  current_fkey_version=$(get_version "firmware_key_version")
  # Firmware version is the kernel subkey version.
  current_ksubkey_version=$(get_version "firmware_version")
  # Kernel data key version is the kernel key version.
  current_kdatakey_version=$(get_version "kernel_key_version")
  current_kernel_version=$(get_version "kernel_version")

  cat <<EOF
Current Firmware key version: ${current_fkey_version}
Current Firmware version: ${current_ksubkey_version}
Current Kernel key version: ${current_kdatakey_version}
Current Kernel version: ${current_kernel_version}
EOF

  backup_existing_kernel_keys $current_ksubkey_version $current_kdatakey_version

  new_ksubkey_version=$(( current_ksubkey_version + 1 ))
  new_kdatakey_version=$(( current_kdatakey_version + 1 ))

  if [ $new_kdatakey_version -gt 65535 ] || [ $new_kdatakey_version -gt 65535 ];
  then
    echo "Version overflow!"
    exit 1
  fi

  cat <<EOF 
Generating new kernel subkey, data keys and new kernel keyblock.

New Firmware version (due to kernel subkey change): ${new_ksubkey_version}.
New Kernel key version (due to kernel datakey change): ${new_kdatakey_version}.
EOF
  make_pair kernel_subkey $KERNEL_SUBKEY_ALGOID $new_ksubkey_version
  make_pair kernel_data_key $KERNEL_DATAKEY_ALGOID $new_kdatakey_version
  make_keyblock kernel $KERNEL_KEYBLOCK_MODE kernel_data_key kernel_subkey

  write_updated_version_file $current_fkey_version $new_ksubkey_version \
    $new_kdatakey_version $current_kernel_version
}

main $@
