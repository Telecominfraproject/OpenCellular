#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# abort on error
set -e

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

if [ $# -ne 1 ]; then
    echo "Usage $0 <image>"
    exit 1
fi

IMAGE=$1
ROOTFS=$(make_temp_dir)
mount_image_partition_ro "$IMAGE" 3 "$ROOTFS"

if ! no_chronos_password $ROOTFS; then
    die "chronos password is set! Shouldn't be for release builds."
fi
