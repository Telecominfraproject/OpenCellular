#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# some stuff we'll need
DEVKEYS=${SRCDIR}/tests/devkeys
TESTKEYS=${SRCDIR}/tests/testkeys
SIGNER=${SRCDIR}/tests/external_rsa_signer.sh


# Create a copy of an existing keyblock, using the old way
${FUTILITY} vbutil_keyblock --pack ${TMP}.keyblock0 \
  --datapubkey ${DEVKEYS}/firmware_data_key.vbpubk \
  --flags 7 \
  --signprivate ${DEVKEYS}/root_key.vbprivk

# Check it.
${FUTILITY} vbutil_keyblock --unpack ${TMP}.keyblock0 \
  --signpubkey ${DEVKEYS}/root_key.vbpubk

# It should be the same as the dev-key firmware keyblock
cmp ${DEVKEYS}/firmware.keyblock ${TMP}.keyblock0


# Now create it the new way
${FUTILITY} sign --debug \
  --datapubkey ${DEVKEYS}/firmware_data_key.vbpubk \
  --flags 7 \
  --signprivate ${DEVKEYS}/root_key.vbprivk \
  --outfile ${TMP}.keyblock1

# It should be the same too.
cmp ${DEVKEYS}/firmware.keyblock ${TMP}.keyblock1


# Create a keyblock without signing it.

# old way
${FUTILITY} vbutil_keyblock --pack ${TMP}.keyblock0 \
  --datapubkey ${DEVKEYS}/firmware_data_key.vbpubk \
  --flags 14

# new way
${FUTILITY} sign --debug \
  --flags 14 \
  ${DEVKEYS}/firmware_data_key.vbpubk \
  ${TMP}.keyblock1

cmp ${TMP}.keyblock0 ${TMP}.keyblock1


# Create one using PEM args

# old way
${FUTILITY} vbutil_keyblock --pack ${TMP}.keyblock2 \
  --datapubkey ${DEVKEYS}/firmware_data_key.vbpubk \
  --signprivate_pem ${TESTKEYS}/key_rsa4096.pem \
  --pem_algorithm 8 \
  --flags 9

# verify it
${FUTILITY} vbutil_keyblock --unpack ${TMP}.keyblock2 \
  --signpubkey ${TESTKEYS}/key_rsa4096.sha512.vbpubk

# new way
${FUTILITY} sign --debug \
  --pem_signpriv ${TESTKEYS}/key_rsa4096.pem \
  --pem_algo 8 \
  --flags 9 \
  ${DEVKEYS}/firmware_data_key.vbpubk \
  ${TMP}.keyblock3

cmp ${TMP}.keyblock2 ${TMP}.keyblock3

# Try it with an external signer

# old way
${FUTILITY} vbutil_keyblock --pack ${TMP}.keyblock4 \
  --datapubkey ${DEVKEYS}/firmware_data_key.vbpubk \
  --signprivate_pem ${TESTKEYS}/key_rsa4096.pem \
  --pem_algorithm 8 \
  --flags 19 \
  --externalsigner ${SIGNER}

# verify it
${FUTILITY} vbutil_keyblock --unpack ${TMP}.keyblock4 \
  --signpubkey ${TESTKEYS}/key_rsa4096.sha512.vbpubk

# new way
${FUTILITY} sign --debug \
  --pem_signpriv ${TESTKEYS}/key_rsa4096.pem \
  --pem_algo 8 \
  --pem_external ${SIGNER} \
  --flags 19 \
  ${DEVKEYS}/firmware_data_key.vbpubk \
  ${TMP}.keyblock5

cmp ${TMP}.keyblock4 ${TMP}.keyblock5


# cleanup
rm -rf ${TMP}*
exit 0
