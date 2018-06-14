#!/bin/bash
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generates a keyblock containing a public key and signed using the given
# signing key.

# Load common constants and functions.
. "$(dirname "$0")/common.sh"

if [ $# -ne 4 ]; then
  cat <<EOF
Usage: $0 <in_public_key> <in_signing_key> <flags> <out_keyblock>

Emits <out_keyblock>.keyblock containing <in_public_key>.vbpubk signed with
<in_signing_key>.vbprivk with the given keyblock <flags>.
EOF
  exit 1
fi

in_pubkey=$1
in_signkey=$2
keyblock_flags=$3
out_keyblock=$4

make_keyblock $out_keyblock $keyblock_flags $in_pubkey $in_signkey
