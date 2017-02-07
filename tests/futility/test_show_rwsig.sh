#!/bin/bash -eux
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

DATADIR="${SCRIPTDIR}/data"
TESTKEYS=${SRCDIR}/tests/testkeys

# Do not test 8192 as the signature length is > 1024 bytes
SIGS="1024 2048 4096 2048_exp3"
HASHES="SHA1 SHA256 SHA512"

set -o pipefail

for s in $SIGS; do
    echo -n "$s " 1>&3

    for h in $HASHES; do
        pemfile=${TESTKEYS}/key_rsa${s}.pem
        outfile=${TMP}.${s}_${h}.new
        infile=${DATADIR}/random_noise.bin
        outkeys=${TMP}.${s}_${h}
        outsig=${TMP}.${s}_${h}.signature

        ${FUTILITY} create --desc "Test key" --hash_alg ${h} \
                    ${pemfile} ${outkeys}

        ${FUTILITY} sign --type rwsig --prikey ${outkeys}.vbprik2 \
                ${infile} ${outsig}
        dd if=/dev/zero bs=$((4096 + 1024)) count=1 of=${outfile}
        dd if=${infile} of=${outfile} conv=notrunc
        dd if=${outsig} of=${outfile} bs=4096 seek=1 conv=notrunc

        ${FUTILITY} show --type rwsig --pubkey ${outkeys}.vbpubk2 ${outfile}
    done
done

# cleanup
rm -rf ${TMP}*
exit 0
