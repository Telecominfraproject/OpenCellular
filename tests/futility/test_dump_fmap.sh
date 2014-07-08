#!/bin/bash -eu
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}

if [ -e "$OLDDIR/dump_fmap" ] ; then
  echo old dump_fmap binary still exists 1>&2
  exit 1
fi

TMP="$OUTDIR/$me.tmp"

"$FUTILITY" dump_fmap -f "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_f.txt" "$TMP"

"$FUTILITY" dump_fmap -p "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_p.txt" "$TMP"

"$FUTILITY" dump_fmap -h "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_h.txt" "$TMP"

# This should fail because the input file is truncated and doesn't really
# contain the stuff that the FMAP claims it does.
cd "$OUTDIR"  # TODO: we really need a directory argument for dump_fmap.
if "$FUTILITY" dump_fmap -x "${SCRIPTDIR}/data_fmap.bin" FMAP ; then
  echo Wait, that was supposed to fail. 1>&2
  exit 1
else
  rm -f FMAP
fi

# cleanup
rm -f "$TMP"
exit 0
