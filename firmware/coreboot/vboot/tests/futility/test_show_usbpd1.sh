#!/bin/bash -eux
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

DATADIR="${SCRIPTDIR}/data"
TESTS="dingdong hoho minimuffin zinger"
TESTKEYS=${SRCDIR}/tests/testkeys

SIGS="1024 2048 4096 8192"
HASHES="SHA1 SHA256 SHA512"

set -o pipefail

for s in $SIGS; do

    echo -n "$s " 1>&3

    for test in $TESTS; do

        infile=${DATADIR}/${test}.unsigned

        for h in $HASHES; do

            pemfile=${TESTKEYS}/key_rsa${s}.pem
            outfile=${TMP}.${test}_${s}_${h}.new

            # sign it
            ${FUTILITY} sign --type usbpd1 --pem ${pemfile} ${infile} ${outfile}

            # make sure it identifies correctly
            ${FUTILITY} verify ${outfile}

        done
    done
done

# cleanup
rm -rf ${TMP}*
exit 0
