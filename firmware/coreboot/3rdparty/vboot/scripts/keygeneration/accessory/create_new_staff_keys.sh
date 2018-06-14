#!/bin/bash

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and functions.
. "$(dirname "$0")/common_leverage_hammer.sh"

main() {
  set -e

  leverage_hammer_to_create_key "staff" "$@"
}

main "$@"
