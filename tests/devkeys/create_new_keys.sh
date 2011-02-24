#!/bin/bash
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate .vbpubk and .vbprivk pairs for use by developer builds. These should
# be exactly like the real keys except that the private keys aren't secret.


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

# Emit .vbpubk and .vbprivk using given basename and algorithm
# NOTE: This function also appears in ../../utility/dev_make_keypair. Making
# the two implementations the same would require some common.sh, which is more
# likely to cause problems than just keeping an eye out for any differences. If
# you feel the need to change this file, check the history of that other file
# to see what may need updating here too.
function make_pair {
  local base=$1
  local alg=$2
  local len=$(alg_to_keylen $alg)

  echo "creating $base keypair..."

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
    --version 1 \
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



# Create the normal keypairs
make_pair root_key                 11
make_pair firmware_data_key        7
make_pair dev_firmware_data_key    7
make_pair kernel_subkey            7
make_pair kernel_data_key          4

# Create the recovery and factory installer keypairs
make_pair recovery_key             11
make_pair recovery_kernel_data_key 11
make_pair installer_kernel_data_key 11

# Create the firmware keyblock for use only in Normal mode. This is redundant,
# since it's never even checked during Recovery mode.
make_keyblock firmware 7 firmware_data_key root_key

# Create the dev firmware keyblock for use only in Developer mode.
make_keyblock dev_firmware 6 dev_firmware_data_key root_key

# Create the recovery kernel keyblock for use only in Recovery mode.
make_keyblock recovery_kernel 11 recovery_kernel_data_key recovery_key

# Create the normal kernel keyblock for use only in Normal mode.
make_keyblock kernel 7 kernel_data_key kernel_subkey

# Create the installer keyblock for use in Developer + Recovery mode
# For use in Factory Install and Developer Mode install shims.
make_keyblock installer_kernel 10 installer_kernel_data_key recovery_key

# CAUTION: The public parts of most of these blobs must be compiled into the
# firmware, which is built separately (and some of which can't be changed after
# manufacturing). If you update these keys, you must coordinate the changes
# with the BIOS people or you'll be unable to boot the resulting images.

