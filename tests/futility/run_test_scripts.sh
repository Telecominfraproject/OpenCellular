#!/bin/bash -eu
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Load common constants and variables.
SCRIPTDIR=$(dirname $(readlink -f "$0"))
. "$SCRIPTDIR/common.sh"

# Mandatory arg is the directory where futility is installed.
[ -z "${1:-}" ] && error "Directory argument is required"
BINDIR="$1"
shift

FUTILITY="$BINDIR/futility"
OLDDIR="$BINDIR/old_bins"


# The Makefile should export the $BUILD directory, but if it's not just warn
# and guess (mostly so we can run the script manually).
if [ -z "${BUILD:-}" ]; then
  BUILD=$(dirname "${BINDIR}")
  yellow "Assuming \$BUILD=$BUILD"
fi
OUTDIR="${BUILD}/tests/futility_test_results"
[ -d "$OUTDIR" ] || mkdir -p "$OUTDIR"


# Let each test know where to find things...
export FUTILITY
export OLDDIR
export SCRIPTDIR
export OUTDIR

# These are the scripts to run. Binaries are invoked directly by the Makefile.
TESTS="
${SCRIPTDIR}/test_main.sh
${SCRIPTDIR}/test_dump_fmap.sh
"


# Get ready...
pass=0
progs=0

##############################################################################
# But first, we'll just test the wrapped functions. This will go away when
# everything is built in (chromium:196079).

# Here are the old programs to be wrapped
# TODO(crbug.com/224734): dev_debug_vboot isn't tested right now.
PROGS=${*:-cgpt crossystem dev_sign_file dumpRSAPublicKey
           dump_kernel_config enable_dev_usb_boot gbb_utility
           tpm_init_temp_fix tpmc vbutil_firmware vbutil_kernel
           vbutil_what_keys}

# For now just compare results of invoking each program with no args.
# TODO: Create true rigorous tests for every program.
echo "-- old_bins --"
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

# How many wrapped executables are left to incorporate? Did we check them all?
xprogs=$(find ${OLDDIR} -type f -perm /111 | wc -l)
if [ $xprogs -gt 0 ]; then
  yellow "${progs}/${xprogs} wrapped executables tested"
fi


##############################################################################
# Invoke the scripts that test the builtin functions.

echo "-- builtin --"
for i in $TESTS; do
  j=${i##*/}

  : $(( progs++ ))

  echo -n "$j ... "
  rm -f "${OUTDIR}/$j."*
  rc=$("$i" "$FUTILITY" 1>"${OUTDIR}/$j.stdout" \
       2>"${OUTDIR}/$j.stderr" || echo "$?")
  echo "${rc:-0}" > "${OUTDIR}/$j.return"
  if [ ! "$rc" ]; then
    green "passed"
    : $(( pass++ ))
    rm -f ${OUTDIR}/$j.{stdout,stderr,return}
  else
    red "failed"
  fi

done

##############################################################################
# How'd we do?

if [ "$pass" -eq "$progs" ]; then
  green "Success: $pass / $progs passed"
  exit 0
fi

red "FAIL: $pass / $progs passed"
exit 1
