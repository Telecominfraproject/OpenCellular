#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate .vbpubk and .vbprivk pairs with the given algorithm id.

# Load common constants and functions.
. "$(dirname "$0")/common.sh"

if [ $# -ne 2 ]; then
  cat <<EOF
Usage: $0 <algoid> <out_keypair>

Output: <out_keypair>.vbprivk and <out_keypair>.vbpubk
EOF
  exit 1
fi

algoid=$1
out_keypair=$2

make_pair $out_keypair $algoid
