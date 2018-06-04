#!/bin/bash -eux
# Copyright 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# Good FMAP
"$FUTILITY" dump_fmap -F "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_f.txt" "$TMP"

"$FUTILITY" dump_fmap -p "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_p.txt" "$TMP"

"$FUTILITY" dump_fmap -h "${SCRIPTDIR}/data_fmap.bin"  > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_h.txt" "$TMP"


# This should fail because the input file is truncated and doesn't really
# contain the stuff that the FMAP claims it does.
if "$FUTILITY" dump_fmap -x "${SCRIPTDIR}/data_fmap.bin" FMAP; then false; fi

# This should fail too
if "$FUTILITY" show "${SCRIPTDIR}/data_fmap.bin"; then false; fi

# However, this should work.
"$FUTILITY" dump_fmap -x "${SCRIPTDIR}/data_fmap.bin" SI_DESC > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_x.txt" "$TMP"

# Redirect dumping to a different place
"$FUTILITY" dump_fmap -x "${SCRIPTDIR}/data_fmap.bin" SI_DESC:FOO > "$TMP"
cmp "${SCRIPTDIR}/data_fmap_expect_x2.txt" "$TMP"
cmp SI_DESC FOO

# This FMAP has problems, and should fail.
if "$FUTILITY" dump_fmap -h "${SCRIPTDIR}/data_fmap2.bin" > "$TMP"; then false; fi
cmp "${SCRIPTDIR}/data_fmap2_expect_h.txt" "$TMP"

"$FUTILITY" dump_fmap -hh "${SCRIPTDIR}/data_fmap2.bin" > "$TMP"
cmp "${SCRIPTDIR}/data_fmap2_expect_hh.txt" "$TMP"

"$FUTILITY" dump_fmap -hhH "${SCRIPTDIR}/data_fmap2.bin" > "$TMP"
cmp "${SCRIPTDIR}/data_fmap2_expect_hhH.txt" "$TMP"


# cleanup
rm -f ${TMP}* FMAP SI_DESC FOO
exit 0
