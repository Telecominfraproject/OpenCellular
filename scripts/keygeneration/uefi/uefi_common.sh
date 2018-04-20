#!/bin/bash
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Common UEFI key generation functions.

. "$(dirname "$0")/../common.sh"

check_uefi_key_dir_name() {
  local key_dir="$1"
  local key_dir_fullpath="$(readlink -f "${key_dir}")"
  local key_dir_basename="$(basename "${key_dir_fullpath}")"
  if [[ "${key_dir_basename}" != "uefi" ]]; then
    die "Key directory base name is not \"uefi\""
  fi
}

# File to read current versions from.
UEFI_VERSION_FILE="uefi_key.versions"

# ARGS: <VERSION_TYPE> [UEFI_VERSION_FILE]
get_uefi_version() {
  local key="$1"
  local file="${2:-${UEFI_VERSION_FILE}}"
  awk -F= -vkey="${key}" '$1 == key { print $NF }' "${file}"
}

# Loads the current versions, prints them to stdout, and sets the global version
# variables: CURR_PK_KEY_VER CURR_KEK_KEY_VER CURR_DB_KEY_VER
# CURR_DB_CHILD_KEY_VER
load_current_uefi_key_versions() {
  local key_dir="$1"
  local UEFI_VERSION_FILE="${key_dir}/${UEFI_VERSION_FILE}"
  if [[ ! -f "${UEFI_VERSION_FILE}" ]]; then
    return 1
  fi
  CURR_PK_KEY_VER=$(get_uefi_version "pk_key_version")
  CURR_KEK_KEY_VER=$(get_uefi_version "kek_key_version")
  CURR_DB_KEY_VER=$(get_uefi_version "db_key_version")
  CURR_DB_CHILD_KEY_VER=$(get_uefi_version "db_child_key_version")

  cat <<EOF
Current UEFI Platform Key (PK) version: ${CURR_PK_KEY_VER}
Current UEFI Key Exchange Key (KEK) version: ${CURR_KEK_KEY_VER}
Current UEFI DB key version: ${CURR_DB_KEY_VER}
Current UEFI DB child key version: ${CURR_DB_CHILD_KEY_VER}
EOF
}

_CHROMIUM_OS_SUBJECT=\
'/C=US/ST=California/L=Mountain View/O=Google LLC./OU=Chromium OS'

_get_subj() {
  local title="$1"
  local version="$2"

  echo "${_CHROMIUM_OS_SUBJECT}/CN=${title} v${version}"
}

# Generate a pair of a private key and a self-signed cert at the current
# directory. Generated files are
#   $1/$1.rsa: The private key
#   $1/$1.pem: The self-signed cert in PEM format
_make_self_signed_pair() {
  local key_name="$1"
  local subj="$2"

  mkdir -p "${key_name}"
  pushd "${key_name}" > /dev/null
  openssl req -new -x509 -nodes -newkey rsa:2048 -sha256 \
      -keyout "${key_name}.rsa" -out "${key_name}.pem" \
      -subj "${subj}" -days 73000
  popd > /dev/null
}

# Generate a pair of a private key and a cert signed by the given CA.
# "$1" (the first argument) is the CA file name without extension.
# The results are signed by "$1/$1.{rsa,pem}", and are generated in
# "$1/$1.children" directory under the current directory. Generated files are
#   $1/$1.children/$2.rsa: The private key
#   $1/$1.children/$2.csr: The Certificate Signing Request
#   $1/$1.children/$2.pem: The certificate signed by "$1.{rsa,pem}"
_make_child_pair() {
  local ca_name="$1"  # Base filename without extension.
  local child_key_name="$2"
  local subj="$3"

  mkdir -p "${ca_name}/${ca_name}.children"
  pushd "${ca_name}/${ca_name}.children" > /dev/null
  openssl req -new -nodes -newkey rsa:2048 -sha256 \
      -keyout "${child_key_name}.rsa" -out "${child_key_name}.csr" \
      -subj "${subj}" -days 73000
  openssl x509 -req -sha256 -CA "../${ca_name}.pem" -CAkey "../${ca_name}.rsa" \
      -CAcreateserial -in "${child_key_name}.csr" \
      -out "${child_key_name}.pem" -days 73000
  popd > /dev/null
}

make_pk_keypair() {
  local version="$1"
  _make_self_signed_pair pk \
      "$(_get_subj "UEFI Platform Key" "${version}")"
}

make_kek_keypair() {
  local version="$1"
  _make_self_signed_pair kek \
      "$(_get_subj "UEFI Key Exchange Key" "${version}")"
}

make_db_keypair() {
  local version="$1"
  _make_self_signed_pair db \
      "$(_get_subj "UEFI DB Key" "${version}")"
}

make_db_child_keypair() {
  local db_key_version="$1"
  local child_key_version="$2"
  _make_child_pair db db_child \
      "$(_get_subj "UEFI DB Child Key" \
          "${db_key_version}.${child_key_version}")"
}

_backup_existing_self_signed_pair() {
  local key_name="$1"
  local version="$2"
  pushd "${key_name}" > /dev/null
  mv --no-clobber "${key_name}".{rsa,"v${version}.rsa"}
  mv --no-clobber "${key_name}".{pem,"v${version}.pem"}
  popd > /dev/null
}

_backup_existing_self_signed_pair_and_children() {
  local key_name="$1"
  local version="$2"
  _backup_existing_self_signed_pair "${key_name}" "${version}"
  pushd "${key_name}" > /dev/null
  mv --no-clobber "${key_name}".{children,"v${version}.children"}
  popd > /dev/null
}

_backup_existing_child_pair() {
  local ca_name="$1"
  local child_key_name="$2"
  local child_key_version="$3"
  pushd "${ca_name}/${ca_name}.children" > /dev/null
  mv --no-clobber "${child_key_name}".{rsa,"v${child_key_version}.rsa"}
  mv --no-clobber "${child_key_name}".{csr,"v${child_key_version}.csr"}
  mv --no-clobber "${child_key_name}".{pem,"v${child_key_version}.pem"}
  popd
}

# Make backup of existing pk keypair.
# Backup format: pk.v<pk key version>.{rsa,pem}
backup_existing_pk_keypair() {
  local pk_key_version="$1"
  _backup_existing_self_signed_pair pk "${pk_key_version}"
}

# Make backup of existing kek keypair.
# Backup format: kek.v<kek key version>.{rsa,pem}
backup_existing_kek_keypair() {
  local kek_key_version="$1"
  _backup_existing_self_signed_pair kek "${kek_key_version}"
}

# Make backup of existing db keypair and children.
# Backup format:
# for db keypair: db.v<db key version>.{rsa,pem}
# for child keypair: db.v<db key version>.childern/child*.{rsa,csr,pem}
backup_existing_db_keypair_and_children() {
  local db_key_version="$1"
  _backup_existing_self_signed_pair_and_children db "${db_key_version}"
}

# Make backup of existing db child keypair.
# Backup format: db.children/child.v<db child key version>.{rsa,csr,pem}
backup_existing_db_child_keypair() {
  local db_child_key_version="$1"
  _backup_existing_child_pair db db_child "${db_child_key_version}"
}

# Write new key version file with the updated key versions.
# Args: PK_KEY_VERSION KEK_KEY_VERSION DB_KEY_VERSION DB_CHILD_KEY_VERSION
write_updated_uefi_version_file() {
  local pk_key_version="$1"
  local kek_key_version="$2"
  local db_key_version="$3"
  local db_child_key_version="$4"

  cat > "${UEFI_VERSION_FILE}" <<EOF
pk_key_version=${pk_key_version}
kek_key_version=${kek_key_version}
db_key_version=${db_key_version}
db_child_key_version=${db_child_key_version}
EOF
}

# Returns the incremented version number of the passed in key from the version
# file.  The options are "pk_key_version", "kek_key_version", "db_key_version",
# or "db_child_key_version".
# ARGS: KEY_DIR <key_name>
increment_uefi_version() {
  local key_dir="$1"
  local UEFI_VERSION_FILE="${key_dir}/${UEFI_VERSION_FILE}"
  local old_version=$(get_uefi_version "$2")
  local new_version=$(( old_version + 1 ))

  if [[ ${new_version} -gt 0xffff ]]; then
    echo "Version overflow!" >&2
    return 1
  fi
  echo ${new_version}
}
