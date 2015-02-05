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

# Demonstrate that the sha1sums are the same for all the keys created from the
# same .pem files, both public and private, vb1 and vb21.
for sig in rsa1024 rsa2048 rsa4096 rsa8192; do
  pem_sum=$(${FUTILITY} show "${TESTKEYS}/key_${sig}.pem" |
    awk '/sha1sum/ {print $3}')
  key_sums=$(${FUTILITY} show ${TMP}_key_${sig}.* |
    awk '/sha1sum/ {print $3}' | uniq)
  # note that this also tests that all the key_sums are the same
  [ "$pem_sum" = "$key_sums" ]
done

# cleanup
rm -rf ${TMP}*
exit 0
