#!/bin/sh
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Tests emerging all the ebuilds that use vboot_reference either as an
# ebuild dependency or by checking out the code and compiling it in a
# different ebuild. This is meant to be run from the chroot as part of testing
# a new change in vboot_reference.

# Required ebuilds:
TEST_EBUILDS="
  sys-boot/chromeos-bootimage
  sys-boot/chromeos-u-boot
  sys-boot/coreboot
  sys-boot/depthcharge
  chromeos-base/chromeos-cryptohome
  chromeos-base/chromeos-ec
  chromeos-base/chromeos-installer
  chromeos-base/chromeos-initramfs
  chromeos-base/chromeos-login
  chromeos-base/update_engine
  chromeos-base/vboot_reference
  chromeos-base/verity
"

set -e

# Check running inside the chroot.
if [ ! -e /etc/cros_chroot_version ]; then
  echo "You must run this inside the chroot." >&2
  exit 1
fi

# Detect the target board.
if [ "x${BOARD}" == "x" ]; then
  if [ -e ~/trunk/src/scripts/.default_board ]; then
    BOARD="`cat ~/trunk/src/scripts/.default_board`"
  else
    echo "You must pass BOARD environment variable or set a default board." >&2
    exit 1
  fi
fi

VBOOT_REF_DIR="$(dirname "$0")"
echo "Running tests for board '${BOARD}' from ${VBOOT_REF_DIR}"

cd "${VBOOT_REF_DIR}"

echo "Running make runtests..."
make runtests -j32

echo "Removing build artifacts."
rm -rf build build-main

echo "Running emerge tests (runs cros_workon start)."
# Ignore errors about already working on those repos.
cros_workon-${BOARD} start ${TEST_EBUILDS} || true

USE=depthcharge emerge-${BOARD} ${TEST_EBUILDS}
