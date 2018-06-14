#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# No args returns nonzero exit code
${FUTILITY} && false

# It's weird but okay if the command is a full path.
${FUTILITY} /fake/path/to/help  > "$TMP"
grep Usage "$TMP"

# Make sure logging does something.
LOG="/tmp/futility.log"
[ -f ${LOG} ] && mv ${LOG} ${LOG}.backup
touch ${LOG}
${FUTILITY} help
grep ${FUTILITY} ${LOG}
rm -f ${LOG}
[ -f ${LOG}.backup ] && mv ${LOG}.backup ${LOG}

# Use some known digests to verify that things work...
DEVKEYS=${SRCDIR}/tests/devkeys
SHA=e78ce746a037837155388a1096212ded04fb86eb

# all progs in the pipelines should work
set -o pipefail

# If it's invoked as the name of a command we know, it should do that command
ln -sf ${FUTILITY} vbutil_key
./vbutil_key --unpack ${DEVKEYS}/installer_kernel_data_key.vbpubk | grep ${SHA}
ln -sf ${FUTILITY} vbutil_keyblock
./vbutil_keyblock --unpack ${DEVKEYS}/installer_kernel.keyblock | grep ${SHA}
cp ${FUTILITY} show
./show ${SCRIPTDIR}/data/rec_kernel_part.bin | grep ${SHA}

# If it's invoked by any other name, expect the command to be the first arg.
ln -sf ${FUTILITY} muggle
./muggle vbutil_key --unpack ${DEVKEYS}/installer_kernel_data_key.vbpubk \
  | grep ${SHA}
ln -sf ${FUTILITY} buggle
./buggle vbutil_keyblock --unpack ${DEVKEYS}/installer_kernel.keyblock \
  | grep ${SHA}
cp ${FUTILITY} boo
./boo show ${SCRIPTDIR}/data/rec_kernel_part.bin | grep ${SHA}


# we expect the first command fail, but the output to match anyway
set +o pipefail

# If it can't figure out the command at all, it should complain.
${FUTILITY} muggle | grep Usage:
./buggle futility | grep Usage:
./boo | grep Usage:

# cleanup
rm -f ${TMP}* vbutil_key vbutil_keyblock show muggle buggle boo
exit 0
