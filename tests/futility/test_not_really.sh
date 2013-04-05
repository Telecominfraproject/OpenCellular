#!/bin/bash -eu
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}

TMP="$OUTDIR/$me.tmp"

echo "FUTILITY=$FUTILITY" > "$TMP"
echo "SCRIPTDIR=$SCRIPTDIR" >> "$TMP"

exit 0
