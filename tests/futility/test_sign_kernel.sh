#!/bin/bash -eux
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

me=${0##*/}
TMP="$me.tmp"

# Work in scratch directory
cd "$OUTDIR"

DEVKEYS=${SRCDIR}/tests/devkeys

echo "hi there" > ${TMP}.config.txt
echo "hello boys" > ${TMP}.config2.txt
dd if=/dev/urandom bs=512 count=1 of=${TMP}.bootloader.bin
dd if=/dev/urandom bs=512 count=1 of=${TMP}.bootloader2.bin

# default padding
padding=49152

try_arch () {
  local arch=$1

  echo -n "${arch}: 1 " 1>&3

  # pack it up the old way
  ${FUTILITY} vbutil_kernel --debug \
    --pack ${TMP}.blob1.${arch} \
    --keyblock ${DEVKEYS}/recovery_kernel.keyblock \
    --signprivate ${DEVKEYS}/recovery_kernel_data_key.vbprivk \
    --version 1 \
    --config ${TMP}.config.txt \
    --bootloader ${TMP}.bootloader.bin \
    --vmlinuz ${SCRIPTDIR}/data/vmlinuz-${arch}.bin \
    --arch ${arch} \
    --pad ${padding} \
    --kloadaddr 0x11000

  # verify the old way
  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob1.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/recovery_key.vbpubk
  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob1.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/recovery_key.vbpubk --debug

  # pack it up the new way
  ${FUTILITY} sign --debug \
    --keyblock ${DEVKEYS}/recovery_kernel.keyblock \
    --signprivate ${DEVKEYS}/recovery_kernel_data_key.vbprivk \
    --version 1 \
    --config ${TMP}.config.txt \
    --bootloader ${TMP}.bootloader.bin \
    --vmlinuz ${SCRIPTDIR}/data/vmlinuz-${arch}.bin \
    --arch ${arch} \
    --pad ${padding} \
    --kloadaddr 0x11000 \
    --outfile ${TMP}.blob2.${arch}

  # they should be identical
  cmp ${TMP}.blob1.${arch} ${TMP}.blob2.${arch}

  echo -n "2 " 1>&3

  # repack it the old way
  ${FUTILITY} vbutil_kernel \
    --repack ${TMP}.blob3.${arch} \
    --oldblob ${TMP}.blob1.${arch} \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin

  # verify the old way
  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob3.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk
  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob3.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk

  # repack it the new way
  ${FUTILITY} sign --debug \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin \
    ${TMP}.blob2.${arch} \
    ${TMP}.blob4.${arch}

  # they should be identical
  cmp ${TMP}.blob3.${arch} ${TMP}.blob4.${arch}

  echo -n "3 " 1>&3

  # repack it the new way, in-place
  cp ${TMP}.blob2.${arch} ${TMP}.blob5.${arch}
  ${FUTILITY} sign --debug \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin \
    ${TMP}.blob5.${arch}

  # they should be identical
  cmp ${TMP}.blob3.${arch} ${TMP}.blob5.${arch}


  # and now just the vblocks...
  echo -n "4 " 1>&3

  dd bs=${padding} count=1 if=${TMP}.blob1.${arch} of=${TMP}.blob1.${arch}.vb0
  ${FUTILITY} vbutil_kernel \
    --pack ${TMP}.blob1.${arch}.vb1 \
    --vblockonly \
    --keyblock ${DEVKEYS}/recovery_kernel.keyblock \
    --signprivate ${DEVKEYS}/recovery_kernel_data_key.vbprivk \
    --version 1 \
    --config ${TMP}.config.txt \
    --bootloader ${TMP}.bootloader.bin \
    --vmlinuz ${SCRIPTDIR}/data/vmlinuz-${arch}.bin \
    --arch ${arch} \
    --pad ${padding} \
    --kloadaddr 0x11000
  cmp ${TMP}.blob1.${arch}.vb0 ${TMP}.blob1.${arch}.vb1

  dd bs=${padding} count=1 if=${TMP}.blob2.${arch} of=${TMP}.blob2.${arch}.vb0
  ${FUTILITY} sign --debug \
    --keyblock ${DEVKEYS}/recovery_kernel.keyblock \
    --signprivate ${DEVKEYS}/recovery_kernel_data_key.vbprivk \
    --version 1 \
    --config ${TMP}.config.txt \
    --bootloader ${TMP}.bootloader.bin \
    --vmlinuz ${SCRIPTDIR}/data/vmlinuz-${arch}.bin \
    --arch ${arch} \
    --pad ${padding} \
    --kloadaddr 0x11000 \
    --vblockonly \
    ${TMP}.blob2.${arch}.vb1

  cmp ${TMP}.blob2.${arch}.vb0 ${TMP}.blob2.${arch}.vb1

  # and verify it the new way
  dd bs=${padding} skip=1 if=${TMP}.blob2.${arch} of=${TMP}.blob2.${arch}.kb1
  ${FUTILITY} verify --debug \
    --pad ${padding} \
    --publickey ${DEVKEYS}/recovery_key.vbpubk \
    --fv ${TMP}.blob2.${arch}.kb1 \
    ${TMP}.blob2.${arch}.vb1

  echo -n "5 " 1>&3

  dd bs=${padding} count=1 if=${TMP}.blob3.${arch} of=${TMP}.blob3.${arch}.vb0
  ${FUTILITY} vbutil_kernel \
    --repack ${TMP}.blob3.${arch}.vb1 \
    --vblockonly \
    --oldblob ${TMP}.blob1.${arch} \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin
  cmp ${TMP}.blob3.${arch}.vb0 ${TMP}.blob3.${arch}.vb1

  dd bs=${padding} count=1 if=${TMP}.blob4.${arch} of=${TMP}.blob4.${arch}.vb0
  ${FUTILITY} sign --debug \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin \
    --pad ${padding} \
    --vblockonly \
    ${TMP}.blob2.${arch} \
    ${TMP}.blob4.${arch}.vb1 \

  cmp ${TMP}.blob4.${arch}.vb0 ${TMP}.blob4.${arch}.vb1

  dd bs=${padding} skip=1 if=${TMP}.blob4.${arch} of=${TMP}.blob4.${arch}.kb1
  ${FUTILITY} verify --debug \
    --pad ${padding} \
    --publickey ${DEVKEYS}/kernel_subkey.vbpubk \
    --fv ${TMP}.blob4.${arch}.kb1 \
    ${TMP}.blob4.${arch}.vb1

  # Note: We specifically do not test repacking with a different --kloadaddr,
  # because the old way has a bug and does not update params->cmd_line_ptr to
  # point at the new on-disk location. Apparently (and not surprisingly), no
  # one has ever done that.
}

try_arch amd64
try_arch arm

# cleanup
rm -rf ${TMP}*
exit 0
