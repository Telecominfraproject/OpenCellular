#!/bin/bash -eux
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}

# Work in scratch directory
cd "$OUTDIR"
TMP="$me.tmp"

# Built-in do-nothing commands.
# TODO(crbug.com/224734): Remove these when we have enough built-in commands
"$FUTILITY" foo hi
"$FUTILITY" bar there
"$FUTILITY" hey boys

# No args returns nonzero exit code
"$FUTILITY" && false

"$FUTILITY" help > "$TMP"
grep Usage "$TMP"
# TODO(crbug.com/224734): Make sure all built-in commands have help, too.

# It's weird but okay if the command is a full path.
"$FUTILITY" /fake/path/to/help  > "$TMP"
grep Usage "$TMP"

# Make sure logging does something.
# Note: This will zap any existing log file. Too bad.
LOG="/tmp/futility.log"
rm -f "$LOG"
touch "$LOG"
"$FUTILITY" help
grep "$FUTILITY" "$LOG"
rm "$LOG"

# cleanup
rm -f "$TMP"
exit 0
