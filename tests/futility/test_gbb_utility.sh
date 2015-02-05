#!/bin/bash -eux
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

# Helper utility to modify binary blobs
REPLACE="${BUILD_RUN}/tests/futility/binary_editor"

# First, let's test the basic functionality

# For simplicity, we'll use the same size for all properties.
${FUTILITY} gbb_utility -c 16,0x10,16,0x10 ${TMP}.blob

# Flags
${FUTILITY} gbb_utility -s --flags=0xdeadbeef ${TMP}.blob
${FUTILITY} gbb_utility -g --flags ${TMP}.blob | grep -i 0xdeadbeef

# HWID length should include the terminating null - this is too long
if ${FUTILITY} gbb_utility -s --hwid="0123456789ABCDEF" ${TMP}.blob; then
  false;
fi
# This works
${FUTILITY} gbb_utility -s --hwid="0123456789ABCDE" ${TMP}.blob
# Read it back?
${FUTILITY} gbb_utility -g ${TMP}.blob | grep "0123456789ABCDE"

# Same kind of tests for the other fields, but they need binary files.

# too long
dd if=/dev/urandom bs=17 count=1 of=${TMP}.data1.toolong
dd if=/dev/urandom bs=17 count=1 of=${TMP}.data2.toolong
dd if=/dev/urandom bs=17 count=1 of=${TMP}.data3.toolong
if ${FUTILITY} gbb_utility -s --rootkey     ${TMP}.data1.toolong ${TMP}.blob; then false; fi
if ${FUTILITY} gbb_utility -s --recoverykey ${TMP}.data2.toolong ${TMP}.blob; then false; fi
if ${FUTILITY} gbb_utility -s --bmpfv       ${TMP}.data3.toolong ${TMP}.blob; then false; fi

# shorter than max should be okay, though
dd if=/dev/urandom bs=10 count=1 of=${TMP}.data1.short
dd if=/dev/urandom bs=10 count=1 of=${TMP}.data2.short
dd if=/dev/urandom bs=10 count=1 of=${TMP}.data3.short
${FUTILITY} gbb_utility -s \
  --rootkey     ${TMP}.data1.short \
  --recoverykey ${TMP}.data2.short \
  --bmpfv       ${TMP}.data3.short ${TMP}.blob
# read 'em back
${FUTILITY} gbb_utility -g \
  --rootkey     ${TMP}.read1 \
  --recoverykey ${TMP}.read2 \
  --bmpfv       ${TMP}.read3 ${TMP}.blob
# Verify (but remember, it's short)
cmp -n 10 ${TMP}.data1.short ${TMP}.read1
cmp -n 10 ${TMP}.data2.short ${TMP}.read2
cmp -n 10 ${TMP}.data3.short ${TMP}.read3

# Okay
dd if=/dev/urandom bs=16 count=1 of=${TMP}.data1
dd if=/dev/urandom bs=16 count=1 of=${TMP}.data2
dd if=/dev/urandom bs=16 count=1 of=${TMP}.data3
${FUTILITY} gbb_utility -s --rootkey     ${TMP}.data1 ${TMP}.blob
${FUTILITY} gbb_utility -s --recoverykey ${TMP}.data2 ${TMP}.blob
${FUTILITY} gbb_utility -s --bmpfv       ${TMP}.data3 ${TMP}.blob

# Read 'em back.
${FUTILITY} gbb_utility -g --rootkey     ${TMP}.read1 ${TMP}.blob
${FUTILITY} gbb_utility -g --recoverykey ${TMP}.read2 ${TMP}.blob
${FUTILITY} gbb_utility -g --bmpfv       ${TMP}.read3 ${TMP}.blob
# Verify
cmp ${TMP}.data1 ${TMP}.read1
cmp ${TMP}.data2 ${TMP}.read2
cmp ${TMP}.data3 ${TMP}.read3


# Okay, creating GBB blobs seems to work. Now let's make sure that corrupted
# blobs are rejected.

# Danger Will Robinson! We assume that ${TMP}.blob has this binary struct:
#
# Field                 Offset  Value
#
# signature:            0x0000  $GBB
# major_version:        0x0004  0x0001
# minor_version:        0x0006  0x0001
# header_size:          0x0008  0x00000080
# flags:                0x000c  0xdeadbeef
# hwid_offset:          0x0010  0x00000080
# hwid_size:            0x0014  0x00000010
# rootkey_offset:       0x0018  0x00000090
# rootkey_size:         0x001c  0x00000010
# bmpfv_offset:         0x0020  0x000000a0
# bmpfv_size:           0x0024  0x00000010
# recovery_key_offset:  0x0028  0x000000b0
# recovery_key_size:    0x002c  0x00000010
# pad:                  0x0030  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
#                       0x0040  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
#                       0x0050  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
#                       0x0060  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
#                       0x0070  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
# (HWID)                0x0080  30 31 32 33 34 35 36 37 38 39 41 42 43 44 45 00
# (rootkey)             0x0090  xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
# (bmpfv)               0x00a0  xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
# (recovery_key)        0x00b0  xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
#                       0x00c0  <EOF>
#

# bad major_version
cat ${TMP}.blob | ${REPLACE} 0x4 2 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

# header size too large
cat ${TMP}.blob | ${REPLACE} 0x8 0x81 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

# header size too small
cat ${TMP}.blob | ${REPLACE} 0x8 0x7f > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

# HWID not null-terminated is invalid
cat ${TMP}.blob | ${REPLACE} 0x8f 0x41 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

# HWID of length zero is okay
cat ${TMP}.blob | ${REPLACE} 0x14 0x00 > ${TMP}.blob.ok
${FUTILITY} gbb_utility ${TMP}.blob.ok
# And HWID of length 1 consisting only of '\0' is okay, too.
cat ${TMP}.blob | ${REPLACE} 0x14 0x01 | ${REPLACE} 0x80 0x00 > ${TMP}.blob.ok
${FUTILITY} gbb_utility ${TMP}.blob.ok

# zero-length HWID not null-terminated is invalid
cat ${TMP}.blob | ${REPLACE} 0x8f 0x41 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

#  hwid_offset < GBB_HEADER_SIZE is invalid
cat ${TMP}.blob | ${REPLACE} 0x10 0x7f > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi
cat ${TMP}.blob | ${REPLACE} 0x10 0x00 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

#  rootkey_offset < GBB_HEADER_SIZE is invalid
cat ${TMP}.blob | ${REPLACE} 0x18 0x7f > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi
cat ${TMP}.blob | ${REPLACE} 0x18 0x00 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

#  bmpfv_offset < GBB_HEADER_SIZE is invalid
cat ${TMP}.blob | ${REPLACE} 0x20 0x7f > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi
cat ${TMP}.blob | ${REPLACE} 0x20 0x00 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

#  recovery_key_offset < GBB_HEADER_SIZE is invalid
cat ${TMP}.blob | ${REPLACE} 0x28 0x7f > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi
cat ${TMP}.blob | ${REPLACE} 0x28 0x00 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility ${TMP}.blob.bad; then false; fi

#  hwid: offset + size  == end of file is okay; beyond is invalid
cat ${TMP}.blob | ${REPLACE} 0x14 0x40 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g ${TMP}.blob.bad
cat ${TMP}.blob | ${REPLACE} 0x14 0x41 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -g ${TMP}.blob.bad; then false; fi

#  rootkey: offset + size  == end of file is okay; beyond is invalid
cat ${TMP}.blob | ${REPLACE} 0x1c 0x30 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g ${TMP}.blob.bad
cat ${TMP}.blob | ${REPLACE} 0x1c 0x31 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -g ${TMP}.blob.bad; then false; fi

#  bmpfv: offset + size  == end of file is okay; beyond is invalid
cat ${TMP}.blob | ${REPLACE} 0x24 0x20 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g ${TMP}.blob.bad
cat ${TMP}.blob | ${REPLACE} 0x24 0x21 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -g ${TMP}.blob.bad; then false; fi

#  recovery_key: offset + size  == end of file is okay; beyond is invalid
cat ${TMP}.blob | ${REPLACE} 0x2c 0x10 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g ${TMP}.blob.bad
cat ${TMP}.blob | ${REPLACE} 0x2c 0x11 > ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -g ${TMP}.blob.bad; then false; fi

# hwid_size == 0 doesn't complain, but can't be set
cat ${TMP}.blob | ${REPLACE} 0x14 0x00 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -s --hwid="A" ${TMP}.blob.bad; then false; fi

# rootkey_size == 0 gives warning, gets nothing, can't be set
cat ${TMP}.blob | ${REPLACE} 0x1c 0x00 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g --rootkey     ${TMP}.read1 ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -s --rootkey  ${TMP}.data1 ${TMP}.blob.bad; then false; fi

# bmpfv_size == 0 gives warning, gets nothing, can't be set
cat ${TMP}.blob | ${REPLACE} 0x24 0x00 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g --bmpfv       ${TMP}.read3 ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -s --bmpfv    ${TMP}.data3 ${TMP}.blob.bad; then false; fi

# recovery_key_size == 0 gives warning, gets nothing, can't be set
cat ${TMP}.blob | ${REPLACE} 0x2c 0x00 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g --recoverykey ${TMP}.read2 ${TMP}.blob.bad
if ${FUTILITY} gbb_utility -s --recoverykey ${TMP}.data2 ${TMP}.blob.bad; then false; fi


# GBB v1.2 adds a sha256 digest field in what was previously padding:
#
# hwid_digest:          0x0030  xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
#                       0x0040  xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
# pad:                  0x0050  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
#                       0x0060  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
#                       0x0070  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
# (HWID)                0x0080  30 31 32 33 34 35 36 37 38 39 41 42 43 44 45 00

# See that the digest is updated properly.
hwid="123456789ABCDEF"
${FUTILITY} gbb_utility -s --hwid=${hwid} ${TMP}.blob
expect=$(echo -n "$hwid" | sha256sum | cut -d ' ' -f 1)
[ $(echo -n ${expect} | wc -c) == "64" ]
${FUTILITY} gbb_utility -g --digest ${TMP}.blob | grep ${expect}

# Garble the digest, see that it's noticed.
# (assuming these zeros aren't present)
cat ${TMP}.blob | ${REPLACE} 0x33 0x00 0x00 0x00 0x00 0x00 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g --digest ${TMP}.blob.bad | grep '0000000000'
${FUTILITY} gbb_utility -g --digest ${TMP}.blob.bad | grep 'invalid'

# Garble the HWID. The digest is unchanged, but now invalid.
cat ${TMP}.blob | ${REPLACE} 0x84 0x70 0x71 0x72 > ${TMP}.blob.bad
${FUTILITY} gbb_utility -g --digest ${TMP}.blob.bad | grep 'invalid'

# cleanup
rm -f ${TMP}*
exit 0
