#!/bin/bash -eux
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"


# The first part of this is a script version of the compiled test by the same
# name, to ensure we have working results.

# Args are <expected_type>, <file_to_probe>
test_case() {
    local result
    result=$(${FUTILITY} show -t "${SRCDIR}/$2" | awk '{print $NF}')
    [ "$1" = "$result" ]
}

# Arg is <file_to_probe>
fail_case() {
    if ${FUTILITY} show -t "$1" ; then false; else true; fi
}

# Known types
test_case "unknown"         "tests/futility/data/short_junk.bin"
test_case "unknown"         "tests/futility/data/random_noise.bin"
test_case "pubkey"          "tests/devkeys/root_key.vbpubk"
test_case "keyblock"        "tests/devkeys/kernel.keyblock"
test_case "fw_pre"          "tests/futility/data/fw_vblock.bin"
test_case "gbb"	            "tests/futility/data/fw_gbb.bin"
test_case "bios"            "tests/futility/data/bios_zgb_mp.bin"
test_case "oldbios"         "tests/futility/data/bios_mario_mp.bin"
test_case "kernel"          "tests/futility/data/kern_preamble.bin"
# We don't have a way to identify these (yet?)
# test_case "RAW_FIRMWARE"
# test_case "RAW_KERNEL"
# test_case "CHROMIUMOS_DISK"
test_case "prikey"	    "tests/devkeys/root_key.vbprivk"
test_case "pubkey21"        "tests/futility/data/sample.vbpubk2"
test_case "prikey21"        "tests/futility/data/sample.vbprik2"
test_case "pem"             "tests/testkeys/key_rsa2048.pem"
test_case "pem"             "tests/testkeys/key_rsa8192.pub.pem"

# Expect failure here.
fail_case "/Sir/Not/Appearing/In/This/Film"
fail_case "${SRCDIR}"
fail_case "/dev/zero"


# Now test the show command when the file type is intentionally wrong. It
# often won't work, but it certainly shouldn't core dump.

# We'll ask futility to tell us what types it supports
TYPES=$(${FUTILITY} show --type help | awk '/^ +/ {print $1}')

# And we'll just reuse the same files above.
FILES=$(awk '/^test_case / {print $NF}' $0 | tr -d '"')

# futility should normally exit with either 0 or 1. Make sure that happens.
# NOTE: /bin/bash returns values > 125 for special problems like signals.
# I welcome patches to do this more portably.
for type in $TYPES; do
    for file in $FILES; do
        ${FUTILITY} show --type ${type} "${SRCDIR}/${file}" && rc=$? || rc=$?
        [ "$rc" -le 2 ]
    done
done


# cleanup
rm -rf ${TMP}*
exit 0
