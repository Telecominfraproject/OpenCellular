#!/bin/bash -eux
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}

# Work in scratch directory
cd "$OUTDIR"
TMP="$me.tmp"

# Good FMAP
"$FUTILITY" dump_fmap -f "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_f.txt" "$TMP"

"$FUTILITY" dump_fmap -p "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_p.txt" "$TMP"

"$FUTILITY" dump_fmap -h "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_h.txt" "$TMP"

# This should fail because the input file is truncated and doesn't really
# contain the stuff that the FMAP claims it does.
! "$FUTILITY" dump_fmap -x "${SCRIPTDIR}/data_fmap.bin" FMAP

# However, this should work.
"$FUTILITY" dump_fmap -x "${SCRIPTDIR}/data_fmap.bin" SI_DESC > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_x.txt" "$TMP"


# This FMAP has problems, and will fail.
! "$FUTILITY" dump_fmap -h "${SCRIPTDIR}/data_fmap2.bin" > "$TMP"
cmp "${SCRIPTDIR}/data_fmap2_expect_h.txt" "$TMP"

"$FUTILITY" dump_fmap -hh "${SCRIPTDIR}/data_fmap2.bin" > "$TMP"
cmp "${SCRIPTDIR}/data_fmap2_expect_hh.txt" "$TMP"

"$FUTILITY" dump_fmap -hhH "${SCRIPTDIR}/data_fmap2.bin" > "$TMP"
cmp "${SCRIPTDIR}/data_fmap2_expect_hhH.txt" "$TMP"


# cleanup
rm -f "$TMP" FMAP SI_DESC
exit 0
