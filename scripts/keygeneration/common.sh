#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Common key generation functions.

SCRIPT_DIR="$(dirname "$0")"

# Algorithm ID mappings:
RSA1024_SHA1_ALGOID=0
RSA1024_SHA256_ALGOID=1
RSA1024_SHA512_ALGOID=2
RSA2048_SHA1_ALGOID=3
RSA2048_SHA256_ALGOID=4
RSA2048_SHA512_ALGOID=5
RSA4096_SHA1_ALGOID=6
RSA4096_SHA256_ALGOID=7
RSA4096_SHA512_ALGOID=8
RSA8192_SHA1_ALGOID=9
RSA8192_SHA256_ALGOID=10
RSA8192_SHA512_ALGOID=11
alg_to_keylen() {
  echo $(( 1 << (10 + ($1 / 3)) ))
}

# Default algorithms.
EC_ROOT_KEY_ALGOID=${RSA4096_SHA256_ALGOID}
EC_DATAKEY_ALGOID=${RSA4096_SHA256_ALGOID}

ROOT_KEY_ALGOID=${RSA8192_SHA512_ALGOID}
RECOVERY_KEY_ALGOID=${RSA8192_SHA512_ALGOID}

FIRMWARE_DATAKEY_ALGOID=${RSA4096_SHA256_ALGOID}
DEV_FIRMWARE_DATAKEY_ALGOID=${RSA4096_SHA256_ALGOID}

RECOVERY_KERNEL_ALGOID=${RSA8192_SHA512_ALGOID}
INSTALLER_KERNEL_ALGOID=${RSA8192_SHA512_ALGOID}
KERNEL_SUBKEY_ALGOID=${RSA4096_SHA256_ALGOID}
KERNEL_DATAKEY_ALGOID=${RSA2048_SHA256_ALGOID}

# Keyblock modes determine which boot modes a signing key is valid for use
# in verification.
EC_KEYBLOCK_MODE=7  # Only allow RW EC firmware in non-recovery.
FIRMWARE_KEYBLOCK_MODE=7  # Only allow RW firmware in non-recovery.
DEV_FIRMWARE_KEYBLOCK_MODE=6  # Only allow in dev mode.
RECOVERY_KERNEL_KEYBLOCK_MODE=11 # Only in recovery mode.
KERNEL_KEYBLOCK_MODE=7  # Only allow in non-recovery.
INSTALLER_KERNEL_KEYBLOCK_MODE=10  # Only allow in Dev + Recovery.

# Emit .vbpubk and .vbprivk using given basename and algorithm
# NOTE: This function also appears in ../../utility/dev_make_keypair. Making
# the two implementations the same would require some common.sh, which is more
# likely to cause problems than just keeping an eye out for any differences. If
# you feel the need to change this file, check the history of that other file
# to see what may need updating here too.
function make_pair {
  local base=$1
  local alg=$2
  local key_version=${3:-1}
  local len=$(alg_to_keylen $alg)

  echo "creating $base keypair (version = $key_version)..."

  # make the RSA keypair
  openssl genrsa -F4 -out "${base}_${len}.pem" $len
  # create a self-signed certificate
  openssl req -batch -new -x509 -key "${base}_${len}.pem" \
    -out "${base}_${len}.crt"
  # generate pre-processed RSA public key
  dumpRSAPublicKey -cert "${base}_${len}.crt" > "${base}_${len}.keyb"

  # wrap the public key
  vbutil_key \
    --pack "${base}.vbpubk" \
    --key "${base}_${len}.keyb" \
    --version  "${key_version}" \
    --algorithm $alg

  # wrap the private key
  vbutil_key \
    --pack "${base}.vbprivk" \
    --key "${base}_${len}.pem" \
    --algorithm $alg

  # remove intermediate files
  rm -f "${base}_${len}.pem" "${base}_${len}.crt" "${base}_${len}.keyb"
}


# Emit a .keyblock containing flags and a public key, signed by a private key
# flags are the bitwise OR of these (passed in decimal, though)
#   0x01  Developer switch off
#   0x02  Developer switch on
#   0x04  Not recovery mode
#   0x08  Recovery mode
function make_keyblock {
  local base=$1
  local flags=$2
  local pubkey=$3
  local signkey=$4

  echo "creating $base keyblock..."

  # create it
  vbutil_keyblock \
    --pack "${base}.keyblock" \
    --flags $flags \
    --datapubkey "${pubkey}.vbpubk" \
    --signprivate "${signkey}.vbprivk"

  # verify it
  vbutil_keyblock \
    --unpack "${base}.keyblock" \
    --signpubkey "${signkey}.vbpubk"
}

# File to read current versions from.
VERSION_FILE="key.versions"

# ARGS: <VERSION_TYPE> [VERSION_FILE]
get_version() {
  awk -F= '/^'$1'\>/ { print $NF }' "${2:-${VERSION_FILE}}"
}

# Loads the current versions prints them to stdout and sets the global version
# variables: CURR_FIRMKEY_VER CURR_FIRM_VER CURR_KERNKEY_VER CURR_KERN_VER
load_current_versions() {
  local key_dir=$1
  local VERSION_FILE="${key_dir}/${VERSION_FILE}"
  if [[ ! -f ${VERSION_FILE} ]]; then
    return 1
  fi
  CURR_FIRMKEY_VER=$(get_version "firmware_key_version")
  # Firmware version is the kernel subkey version.
  CURR_FIRM_VER=$(get_version "firmware_version")
  # Kernel data key version is the kernel key version.
  CURR_KERNKEY_VER=$(get_version "kernel_key_version")
  CURR_KERN_VER=$(get_version "kernel_version")

  cat <<EOF
Current Firmware key version: ${CURR_FIRMKEY_VER}
Current Firmware version: ${CURR_FIRM_VER}
Current Kernel key version: ${CURR_KERNKEY_VER}
Current Kernel version: ${CURR_KERN_VER}
EOF
}

# Make backups of existing kernel subkeys and keyblocks that will be revved.
# Backup format:
# for keyblocks: <keyblock_name>.v<datakey version>.v<subkey version>.keyblock
# Args: SUBKEY_VERSION DATAKEY_VERSION
backup_existing_kernel_keyblock() {
  if [[ ! -e kernel.keyblock ]]; then
    return
  fi
  mv --no-clobber kernel.{keyblock,"v$2.v$1.keyblock"}
}

# Make backups of existing kernel subkeys and keyblocks that will be revved.
# Backup format:
# for keys: <key_name>.v<version>.vb{pub|priv}k
# for keyblocks: <keyblock_name>.v<datakey version>.v<subkey version>.keyblock
# Args: SUBKEY_VERSION DATAKEY_VERSION
backup_existing_kernel_subkeys() {
  local subkey_ver=$1
  local datakey_ver=$2
  # --no-clobber to prevent accidentally overwriting existing
  # backups.
  mv --no-clobber kernel_subkey.{vbprivk,"v${subkey_ver}.vbprivk"}
  mv --no-clobber kernel_subkey.{vbpubk,"v${subkey_ver}.vbpubk"}
  backup_existing_kernel_keyblock ${subkey_ver} ${datakey_ver}
}

# Make backups of existing kernel data keys and keyblocks that will be revved.
# Backup format:
# for keys: <key_name>.v<version>.vb{pub|priv}k
# for keyblocks: <keyblock_name>.v<datakey version>.v<subkey version>.keyblock
# Args: SUBKEY_VERSION DATAKEY_VERSION
backup_existing_kernel_data_keys() {
  local subkey_ver=$1
  local datakey_ver=$2
  # --no-clobber to prevent accidentally overwriting existing
  # backups.
  mv --no-clobber kernel_data_key.{vbprivk,"v${datakey_ver}.vbprivk"}
  mv --no-clobber kernel_data_key.{vbpubk,"v${datakey_ver}.vbpubk"}
  backup_existing_kernel_keyblock ${subkey_ver} ${datakey_ver}
}

# Make backups of existing firmware keys and keyblocks that will be revved.
# Backup format:
# for keys: <key_name>.v<version>.vb{pub|priv}k
# for keyblocks: <keyblock_name>.v<datakey version>.v<subkey version>.keyblock
# Args: SUBKEY_VERSION DATAKEY_VERSION
backup_existing_firmware_keys() {
  local subkey_ver=$1
  local datakey_ver=$2
  mv --no-clobber firmware_data_key.{vbprivk,"v${subkey_ver}.vbprivk"}
  mv --no-clobber firmware_data_key.{vbpubk,"v${subkey_ver}.vbpubk"}
  mv --no-clobber firmware.{keyblock,"v${datakey_ver}.v${subkey_ver}.keyblock"}
}


# Write new key version file with the updated key versions.
# Args: FIRMWARE_KEY_VERSION FIRMWARE_VERSION KERNEL_KEY_VERSION
#       KERNEL_VERSION
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

# Returns the incremented version number of the passed in key from the version
# file.  The options are "firmware_key_version", "firmware_version",
# "kernel_key_version", or "kernel_version".
# ARGS: KEY_DIR <key_name>
increment_version() {
  local key_dir=$1
  local VERSION_FILE="${key_dir}/${VERSION_FILE}"
  local old_version=$(get_version $2)
  local new_version=$(( ${old_version} + 1 ))

  if [[ ${new_version} -gt 0xffff ]]; then
    echo "Version overflow!" >&2
    return 1
  fi
  echo ${new_version}
}
