#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# No args returns nonzero exit code
"$FUTILITY" && false

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

# Make sure deprecated functions fail via symlink
ln -sf "$FUTILITY" dev_sign_file
if ./dev_sign_file 2>${TMP}.outmsg ; then false; fi
grep deprecated ${TMP}.outmsg
# They may still fail when invoked through futility (this one does),
# but with a different error message.
"$FUTILITY" dev_sign_file 1>${TMP}.outmsg2 2>&1 || true
if grep deprecated ${TMP}.outmsg2; then false; fi


# cleanup
rm -f ${TMP}* ./dev_sign_file
exit 0
