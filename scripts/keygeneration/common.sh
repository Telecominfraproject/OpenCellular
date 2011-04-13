#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Common key generation functions.

SCRIPT_DIR="$(dirname "$0")"

# 0 = (RSA1024 SHA1)
# 1 = (RSA1024 SHA256)
# 2 = (RSA1024 SHA512)
# 3 = (RSA2048 SHA1)
# 4 = (RSA2048 SHA256)
# 5 = (RSA2048 SHA512)
# 6 = (RSA4096 SHA1)
# 7 = (RSA4096 SHA256)
# 8 = (RSA4096 SHA512)
# 9 = (RSA8192 SHA1)
# 10 = (RSA8192 SHA256)
# 11 = (RSA8192 SHA512)
function alg_to_keylen {
  echo $(( 1 << (10 + ($1 / 3)) ))
}

# Default alrogithms.
ROOT_KEY_ALGOID=11
RECOVERY_KEY_ALGOID=11

FIRMWARE_DATAKEY_ALGOID=7
DEV_FIRMWARE_DATAKEY_ALGOID=7

RECOVERY_KERNEL_ALGOID=11
INSTALLER_KERNEL_ALGOID=11
KERNEL_SUBKEY_ALGOID=7
KERNEL_DATAKEY_ALGOID=4

# Keyblock modes determine which boot modes a signing key is valid for use
# in verification.
FIRMWARE_KEYBLOCK_MODE=7
DEV_FIRMWARE_KEYBLOCK_MODE=6  # Only allow in dev mode.
RECOVERY_KERNEL_KEYBLOCK_MODE=11
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


