#!/bin/bash
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Where are the programs I'm testing against?
[ -z "${1:-}" ] && error "Directory argument is required"
BINDIR="$1"
shift

FUTILITY="$BINDIR/futility"
OLDDIR="$BINDIR/old_bins"

BUILD=$(dirname "${BINDIR}")

# Here are the old programs to be wrapped
# FIXME(chromium-os:37062): There are others besides these.
# FIXME: dev_debug_vboot isn't tested right now.
PROGS=${*:-cgpt crossystem dev_sign_file dumpRSAPublicKey
           dump_fmap dump_kernel_config enable_dev_usb_boot gbb_utility
           tpm_init_temp_fix tpmc vbutil_firmware vbutil_kernel vbutil_key
           vbutil_keyblock vbutil_what_keys}

# Get ready
pass=0
progs=0
pwd
OUTDIR="${BUILD}/tests/futility_test_dir"
[ -d "$OUTDIR" ] || mkdir -p "$OUTDIR"

# For now just compare results of invoking each program with no args.
# FIXME(chromium-os:37062): Create true rigorous tests for every program.
for i in $PROGS; do
  : $(( progs++ ))

  # Try the real thing first
  echo -n "$i ... "
  rc=$("${OLDDIR}/$i" \
    1>"${OUTDIR}/$i.stdout.orig" 2>"${OUTDIR}/$i.stderr.orig" \
    || echo "$?")
  echo "${rc:-0}" > "${OUTDIR}/$i.return.orig"

  # Then try the symlink
  rc=$("$BINDIR/$i" 1>"${OUTDIR}/$i.stdout.link" \
       2>"${OUTDIR}/$i.stderr.link" || echo "$?")
  echo "${rc:-0}" > "${OUTDIR}/$i.return.link"

  # And finally try the explicit wrapper
  rc=$("$FUTILITY" "$i" 1>"${OUTDIR}/$i.stdout.futil" \
       2>"${OUTDIR}/$i.stderr.futil" || echo "$?")
  echo "${rc:-0}" > "${OUTDIR}/$i.return.futil"

  # Different?
  if cmp -s "${OUTDIR}/$i.return.orig" "${OUTDIR}/$i.return.link" &&
     cmp -s "${OUTDIR}/$i.stdout.orig" "${OUTDIR}/$i.stdout.link" &&
     cmp -s "${OUTDIR}/$i.stderr.orig" "${OUTDIR}/$i.stderr.link" &&
     cmp -s "${OUTDIR}/$i.return.orig" "${OUTDIR}/$i.return.futil" &&
     cmp -s "${OUTDIR}/$i.stdout.orig" "${OUTDIR}/$i.stdout.futil" &&
     cmp -s "${OUTDIR}/$i.stderr.orig" "${OUTDIR}/$i.stderr.futil" ; then
    green "passed"
    : $(( pass++ ))
    rm -f ${OUTDIR}/$i.{stdout,stderr,return}.{orig,link,futil}
  else
    red "failed"
  fi
done

# done
if [ "$pass" -eq "$progs" ]; then
  green "Success: $pass / $progs passed"
  exit 0
fi

red "FAIL: $pass / $progs passed"
exit 1
