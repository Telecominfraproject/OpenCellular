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

declare -i num_hash

# Verify a BDB
#
# $1: Key digest file
# $2: Any remaining option passed to futility bdb --verify
verify() {
	local key_digest=${1:-${BDBKEY_DIGEST}}
	local extra_option=${2:-}
	${FUTILITY} bdb --verify ${BDB_FILE} --key_digest ${key_digest} \
		${extra_option}
}

get_num_hash() {
	printf "%d" \
		$(${FUTILITY} show ${BDB_FILE} \
			| grep '# of Hashes' | cut -d':' -f 2)
}

# Tests field matches a specified value in a BDB
# e.g. check_field 'Data Version:' 2 returns error if the data version isn't 2.
check_field() {
	# Find the field
	x=$(${FUTILITY} show ${BDB_FILE} | grep "${1}")
	[ "${x}" ] || return 1
	# Remove the field name
	x=${x##*:}
	[ "${x}" ] || return 1
	# Remove the leading and trailing spaces
	x=${x//[[:blank:]]/}
	[ "${x}" == "${2}" ] || return 1
}

# Demonstrate bdb --create can create a valid BDB
${FUTILITY} bdb --create ${BDB_FILE} \
	--bdbkey_pri ${BDBKEY_PRI} --bdbkey_pub ${BDBKEY_PUB} \
	--datakey_pub ${DATAKEY_PUB} --datakey_pri ${DATAKEY_PRI}
verify

# Demonstrate bdb --add can  add a new hash
num_hash=$(get_num_hash)
${FUTILITY} bdb --add ${BDB_FILE} \
	--data ${DATA_FILE} --partition 1 --type 2 --offset 3 --load_address 4
# Use futility show command to verify the hash is added
num_hash+=1
[ $(get_num_hash) -eq $num_hash ]
# TODO: verify partition, type, offset, and load_address

# Demonstrate futility bdb --resign can resign the BDB
data_version=2
${FUTILITY} bdb --resign ${BDB_FILE} --datakey_pri ${DATAKEY_PRI} \
	--data_version $data_version
verify
check_field "Data Version:" $data_version

# Demonstrate futility bdb --resign can resign with a new data key
# Note resigning with a new data key requires a private BDB key as well
${FUTILITY} bdb --resign ${BDB_FILE} \
	--bdbkey_pri ${BDBKEY_PRI} \
	--datakey_pri ${BDBKEY_PRI} --datakey_pub ${BDBKEY_PUB}
verify

# Demonstrate futility bdb --resign can resign with a new BDB key
${FUTILITY} bdb --resign ${BDB_FILE} \
	--bdbkey_pri ${DATAKEY_PRI} --bdbkey_pub ${DATAKEY_PUB}
verify ${DATAKEY_DIGEST}

# Demonstrate futility bdb --verify can return success when key digest doesn't
# match but --ignore_key_digest is specified.
verify ${BDBKEY_DIGEST} --ignore_key_digest

# cleanup
rm -rf ${TMP}*
exit 0
