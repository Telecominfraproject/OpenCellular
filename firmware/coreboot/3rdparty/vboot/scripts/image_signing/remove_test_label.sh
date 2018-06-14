#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Remove the test label from lsb-release to prepare an image for
# signing using the official keys.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

set -e
image=$1

rootfs=$(make_temp_dir)
mount_image_partition ${image} 3 ${rootfs}
sed -i 's/test//' "${rootfs}/etc/lsb-release"
