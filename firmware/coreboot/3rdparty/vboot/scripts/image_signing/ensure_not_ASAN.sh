#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Abort on error.
set -e

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

usage() {
    echo "Usage $PROG image"
}

main() {
    if [ $# -ne 1 ]; then
        usage
        exit 1
    fi

    local image="$1"

    local rootfs=$(make_temp_dir)
    mount_image_partition_ro "$image" 3 "$rootfs"

    # This mirrors the check performed in the platform_ToolchainOptions
    # autotest.
    if readelf -s "$rootfs/opt/google/chrome/chrome" | \
       grep -q __asan_init; then
        exit 1
    fi
}
main "$@"
