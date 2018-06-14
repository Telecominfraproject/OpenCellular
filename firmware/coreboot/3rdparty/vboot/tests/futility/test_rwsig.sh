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

SIGS="1024 2048 2048_exp3 3072_exp3 4096 8192"
HASHES="SHA1 SHA256 SHA512"
EC_RW="EC_RW.bin"

set -o pipefail

infile=${DATADIR}/hammer_dev.bin
# Signing without private key should extract EC_RW.bin
${FUTILITY} sign --type rwsig --version 2 ${infile}
cmp ${EC_RW} ${DATADIR}/${EC_RW}

for s in $SIGS; do
    echo -n "$s " 1>&3

    for h in $HASHES; do
        pemfile=${TESTKEYS}/key_rsa${s}.pem
        outkeys=${TMP}.${s}_${h}
        outfile=${TMP}.${s}_${h}.bin

        ${FUTILITY} create --desc "Test key" --hash_alg ${h} \
                    ${pemfile} ${outkeys}

        # The input file should be correctly signed to start with
        ${FUTILITY} show --type rwsig ${infile}

        # Using the wrong key to verify it should fail
        if ${FUTILITY} show --type rwsig --pubkey ${outkeys}.vbpubk2 \
                       ${infile}; then
            exit 1
        fi

        cp ${infile} ${outfile}

	# Sign ec.bin with a new private key
        ${FUTILITY} sign --type rwsig --prikey ${outkeys}.vbprik2 \
                --version 2 ${outfile}
        # Check EC_RW.bin is produced
        [[ -e ${EC_RW} ]]

        ${FUTILITY} show --type rwsig --pubkey ${outkeys}.vbpubk2 ${outfile}
        ${FUTILITY} show --type rwsig ${outfile}
    done
done

# cleanup
rm -rf ${TMP}*
exit 0
