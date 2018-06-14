#!/bin/bash
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Script to run a test under qemu
#
# Usage:
#    test_using_qemu.sh (command line to run)
#
# Required environment variables:
#    BUILD_RUN - path to build directory inside chroot
#    HOME - home directory inside chroot
#    QEMU_RUN - path to QEMU binary inside chroot
#    SYSROOT - path to root for target platform, outside chroot

set -e

# Set up mounts
sudo mkdir -p "${SYSROOT}/proc" "${SYSROOT}/dev"
sudo mount --bind /proc "${SYSROOT}/proc"
sudo mount --bind /dev "${SYSROOT}/dev"

# Don't exit on error, so we can capture the error code
set +e
sudo chroot ${SYSROOT} ${QEMU_RUN} -drop-ld-preload \
    -E LD_LIBRARY_PATH=/lib64:/lib:/usr/lib64:/usr/lib \
    -E HOME=${HOME} \
    -E BUILD=${BUILD_RUN} \
    -- $*
exit_code=$?
set -e

# Clean up mounts
sudo umount -l "${SYSROOT}/proc"
sudo umount -l "${SYSROOT}/dev"

# Pass through exit code from command
exit $exit_code
