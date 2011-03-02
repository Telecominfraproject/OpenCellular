#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate .vbpubk and .vbprivk pairs for use by developer builds. These should
# be exactly like the real keys except that the private keys aren't secret.

# Load common constants and functions.
. "$(dirname "$0")/common.sh"

# Mapping are in common.sh.
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

# Create the normal keypairs
make_pair root_key                 $ROOT_KEY_ALGOID
make_pair firmware_data_key        $FIRMWARE_DATAKEY_ALGOID
make_pair dev_firmware_data_key    $DEV_FIRMWARE_DATAKEY_ALGOID
make_pair kernel_subkey            $KERNEL_SUBKEY_ALGOID
make_pair kernel_data_key          $KERNEL_DATAKEY_ALGOID

# Create the recovery and factory installer keypairs
make_pair recovery_key             $RECOVERY_KEY_ALGOID
make_pair recovery_kernel_data_key $RECOVERY_KERNEL_ALGOID
make_pair installer_kernel_data_key $INSTALLER_KERNEL_ALGOID

# Create the firmware keyblock for use only in Normal mode. This is redundant,
# since it's never even checked during Recovery mode.
make_keyblock firmware $FIRMWARE_KEYBLOCK_MODE firmware_data_key root_key

# Create the dev firmware keyblock for use only in Developer mode.
make_keyblock dev_firmware $DEV_FIRMWARE_KEYBLOCK_MODE dev_firmware_data_key root_key

# Create the recovery kernel keyblock for use only in Recovery mode.
make_keyblock recovery_kernel $RECOVERY_KERNEL_KEYBLOCK_MODE recovery_kernel_data_key recovery_key

# Create the normal kernel keyblock for use only in Normal mode.
make_keyblock kernel $KERNEL_KEYBLOCK_MODE kernel_data_key kernel_subkey

# Create the installer keyblock for use in Developer + Recovery mode
# For use in Factory Install and Developer Mode install shims.
make_keyblock installer_kernel $INSTALLER_KERNEL_KEYBLOCK_MODE installer_kernel_data_key recovery_key

# CAUTION: The public parts of most of these blobs must be compiled into the
# firmware, which is built separately (and some of which can't be changed after
# manufacturing). If you update these keys, you must coordinate the changes
# with the BIOS people or you'll be unable to boot the resulting images.

