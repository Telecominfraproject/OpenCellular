#!/bin/bash -eux
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# The signed input images are signed with dev keys. We resign the unsigned
# images with the same keypair, to make sure that we're producing identical
# binaries.

DATADIR="${SCRIPTDIR}/data"
TESTS="dingdong hoho minimuffin zinger"

set -o pipefail

count=0
for test in $TESTS; do

    : $(( count++ ))
    echo -n "$count " 1>&3

    pemfile=${DATADIR}/${test}.pem
    infile=${DATADIR}/${test}.unsigned
    goodfile=${DATADIR}/${test}.signed
    outfile=${TMP}.${test}.new

    # Signing the whole thing with futility should produce identical results
    ${FUTILITY} sign --type usbpd1 --pem ${pemfile} ${infile} ${outfile}
    cmp ${goodfile} ${outfile}

    # Now try signing just the RW part
    size=$(stat -c '%s' ${infile})
    half=$(( size / 2 ))

    newin=${TMP}.${test}.rw_in
    dd if=${infile} bs=${half} count=1 skip=1 of=${newin}
    newgood=${TMP}.${test}.rw_ok
    dd if=${goodfile} bs=${half} count=1 skip=1 of=${newgood}
    newout=${TMP}.${test}.rw_out

    # Sign the RW part alone
    ${FUTILITY} sign --type usbpd1 --pem ${pemfile} \
        --ro_size 0 \
        ${newin} ${newout}
    cmp ${newgood} ${newout}

done

# cleanup
rm -rf ${TMP}*
exit 0
