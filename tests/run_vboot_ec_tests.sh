#!/bin/bash -eu

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run verified boot firmware and kernel verification tests.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

check_test_keys

for priv in ${TESTKEY_DIR}/*.vbprivk; do
  root=$(basename ${i%.vbprivk})
  pub="${priv%.vbprivk}.vbpubk"
  echo "Trying $root ..."
  ${TEST_DIR}/vboot_ec_tests "$priv" "$pub"
done
