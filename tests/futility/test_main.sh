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
LOG="/tmp/futility.log"
[ -f ${LOG} ] && mv ${LOG} ${LOG}.backup
touch ${LOG}
"$FUTILITY" help
grep "$FUTILITY" ${LOG}
rm -f ${LOG}
[ -f ${LOG}.backup ] && mv ${LOG}.backup ${LOG}

# Make sure deprecated functions fail via symlink
DEPRECATED="dev_sign_file"

for i in $DEPRECATED; do
  ln -sf "$FUTILITY" $i
  if ./$i 2>${TMP}.outmsg ; then false; fi
  grep deprecated ${TMP}.outmsg
  # They may still fail when invoked through futility
  # but with a different error message.
  "$FUTILITY" $i 1>${TMP}.outmsg2 2>&1 || true
  if grep deprecated ${TMP}.outmsg2; then false; fi
  rm -f $i
done

# cleanup
rm -f ${TMP}*
exit 0
