#!/bin/bash -eux
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"
BDB_FILE=bdb.bin

TESTKEY_DIR=${SRCDIR}/tests/testkeys
TESTDATA_DIR=${SRCDIR}/tests/testdata

BDBKEY_PUB=${TESTKEY_DIR}/bdbkey.keyb
BDBKEY_PRI=${TESTKEY_DIR}/bdbkey.pem
DATAKEY_PUB=${TESTKEY_DIR}/datakey.keyb
DATAKEY_PRI=${TESTKEY_DIR}/datakey.pem
BDBKEY_DIGEST=${TESTDATA_DIR}/bdbkey_digest.bin
DATAKEY_DIGEST=${TESTDATA_DIR}/datakey_digest.bin
DATA_FILE=${TESTDATA_DIR}/sp-rw.bin

verify() {
	local key_digest=${1:-${BDBKEY_DIGEST}}
	${FUTILITY} bdb --verify ${BDB_FILE} --key_digest ${key_digest}
}

# Demonstrate bdb --create can create a valid BDB
${FUTILITY} bdb --create ${BDB_FILE} \
	--bdbkey_pri ${BDBKEY_PRI} --bdbkey_pub ${BDBKEY_PUB} \
	--datakey_pub ${DATAKEY_PUB} --datakey_pri ${DATAKEY_PRI}
verify

# Demonstrate bdb --add can  add a new hash
${FUTILITY} bdb --add ${BDB_FILE} \
	--data ${DATA_FILE} --partition 1 --type 2 --offset 3 --load_address 4
# TODO: Use futility show command to verify the hash is added

# cleanup
rm -rf ${TMP}*
exit 0
