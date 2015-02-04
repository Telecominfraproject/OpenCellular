#!/bin/bash -eux
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# Current vb1 keys, including original .pem files.
TESTKEYS=${SRCDIR}/tests/testkeys

# Demonstrate that we can recreate the same vb1 keys without the .keyb files
for sig in rsa1024 rsa2048 rsa4096 rsa8192; do
  for hash in sha1 sha256 sha512; do
    ${FUTILITY} --vb1 create --hash_alg "${hash}" \
      "${TESTKEYS}/key_${sig}.pem" "${TMP}_key_${sig}.${hash}"
    cmp "${TESTKEYS}/key_${sig}.${hash}.vbprivk" \
      "${TMP}_key_${sig}.${hash}.vbprivk"
    cmp "${TESTKEYS}/key_${sig}.${hash}.vbpubk" \
      "${TMP}_key_${sig}.${hash}.vbpubk"
  done
done


# Demonstrate that we can create some vb21 keypairs. This doesn't prove
# prove anything until we've used them to sign some stuff, though.
for sig in rsa1024 rsa2048 rsa4096 rsa8192; do
  for hash in sha1 sha256 sha512; do
    ${FUTILITY} --vb21 create --hash_alg "${hash}" \
      "${TESTKEYS}/key_${sig}.pem" "${TMP}_key_${sig}.${hash}"
  done
done

# cleanup
rm -rf ${TMP}*
exit 0
