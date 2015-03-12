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
dd if=/dev/urandom bs=1M count=16 of=${TMP}.kern_partition

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
    --signpubkey ${DEVKEYS}/recovery_key.vbpubk > ${TMP}.verify1

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

  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob2.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/recovery_key.vbpubk > ${TMP}.verify2

  # they should be identical
  cmp ${TMP}.blob1.${arch} ${TMP}.blob2.${arch}
  diff ${TMP}.verify1 ${TMP}.verify2

  echo -n "2 " 1>&3

  # repack it the old way
  ${FUTILITY} vbutil_kernel --debug \
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
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk > ${TMP}.verify3

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

  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob4.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk > ${TMP}.verify4

  # they should be identical
  cmp ${TMP}.blob3.${arch} ${TMP}.blob4.${arch}
  diff ${TMP}.verify3 ${TMP}.verify4

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

  ${FUTILITY} vbutil_kernel --verify ${TMP}.blob5.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk > ${TMP}.verify5

  # they should be identical
  cmp ${TMP}.blob3.${arch} ${TMP}.blob5.${arch}
  diff ${TMP}.verify3 ${TMP}.verify5

  # and now just the vblocks...
  echo -n "4 " 1>&3

  # pack the old way
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

  # compare this new vblock with the one from the full pack
  dd bs=${padding} count=1 if=${TMP}.blob1.${arch} of=${TMP}.blob1.${arch}.vb0
  cmp ${TMP}.blob1.${arch}.vb0 ${TMP}.blob1.${arch}.vb1

  # pack the new way
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

  # compare this new vblock with the one from the full pack
  dd bs=${padding} count=1 if=${TMP}.blob2.${arch} of=${TMP}.blob2.${arch}.vb0
  cmp ${TMP}.blob2.${arch}.vb0 ${TMP}.blob2.${arch}.vb1

  echo -n "5 " 1>&3

  # now repack the old way, again emitting just the vblock
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

  # compare the full repacked vblock with the new repacked vblock
  dd bs=${padding} count=1 if=${TMP}.blob3.${arch} of=${TMP}.blob3.${arch}.vb0
  cmp ${TMP}.blob3.${arch}.vb0 ${TMP}.blob3.${arch}.vb1

  # extract just the kernel blob
  dd bs=${padding} skip=1 if=${TMP}.blob3.${arch} of=${TMP}.blob3.${arch}.kb0
  # and verify it using the new vblock (no way to do that with vbutil_kernel)
  ${FUTILITY} verify --debug \
    --pad ${padding} \
    --publickey ${DEVKEYS}/kernel_subkey.vbpubk \
    --fv ${TMP}.blob3.${arch}.kb0 \
    ${TMP}.blob3.${arch}.vb1 > ${TMP}.verify3v

  # repack the new way
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

  # compare the full repacked vblock with the new repacked vblock
  dd bs=${padding} count=1 if=${TMP}.blob4.${arch} of=${TMP}.blob4.${arch}.vb0
  cmp ${TMP}.blob4.${arch}.vb0 ${TMP}.blob4.${arch}.vb1

  # extract just the kernel blob
  dd bs=${padding} skip=1 if=${TMP}.blob4.${arch} of=${TMP}.blob4.${arch}.kb0
  # and verify it using the new vblock (no way to do that with vbutil_kernel)
  ${FUTILITY} verify --debug \
    --pad ${padding} \
    --publickey ${DEVKEYS}/kernel_subkey.vbpubk \
    --fv ${TMP}.blob4.${arch}.kb0 \
    ${TMP}.blob4.${arch}.vb1 > ${TMP}.verify4v


  echo -n "6 " 1>&3

  # Now lets repack some kernel partitions, not just blobs.
  cp ${TMP}.kern_partition ${TMP}.part1.${arch}
  dd if=${TMP}.blob1.${arch} of=${TMP}.part1.${arch} conv=notrunc

  # Make sure the partitions verify
  ${FUTILITY} vbutil_kernel --verify ${TMP}.part1.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/recovery_key.vbpubk > ${TMP}.verify6

  # The partition should verify the same way as the blob
  diff ${TMP}.verify1 ${TMP}.verify6

  # repack it the old way
  ${FUTILITY} vbutil_kernel --debug \
    --repack ${TMP}.part6.${arch} \
    --oldblob ${TMP}.part1.${arch} \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin

  # verify the old way
  ${FUTILITY} vbutil_kernel --verify ${TMP}.part6.${arch} \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk > ${TMP}.verify6.old

  # this "partition" should actually be the same as the old-way blob
  cmp ${TMP}.blob3.${arch} ${TMP}.part6.${arch}

  # repack it the new way, in-place
  cp ${TMP}.part1.${arch} ${TMP}.part6.${arch}.new1
  ${FUTILITY} sign --debug \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin \
    ${TMP}.part6.${arch}.new1

  ${FUTILITY} vbutil_kernel --verify ${TMP}.part6.${arch}.new1 \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk > ${TMP}.verify6.new1

  # The verification should be indentical
  diff ${TMP}.verify6.old ${TMP}.verify6.new1
  # But the content should only match up to the size of the kernel blob, since
  # we're modifying an entire partition in-place.
  blobsize=$(stat -c '%s' ${TMP}.part6.${arch})
  cmp -n ${blobsize} ${TMP}.part6.${arch} ${TMP}.part6.${arch}.new1
  # The rest of the partition should be unchanged.
  cmp -i ${blobsize} ${TMP}.part1.${arch} ${TMP}.part6.${arch}.new1

  # repack it the new way, from input to output
  cp ${TMP}.part1.${arch} ${TMP}.part1.${arch}.in
  ${FUTILITY} sign --debug \
    --signprivate ${DEVKEYS}/kernel_data_key.vbprivk \
    --keyblock ${DEVKEYS}/kernel.keyblock \
    --version 2 \
    --pad ${padding} \
    --config ${TMP}.config2.txt \
    --bootloader ${TMP}.bootloader2.bin \
    ${TMP}.part1.${arch}.in \
    ${TMP}.part6.${arch}.new2

  ${FUTILITY} vbutil_kernel --verify ${TMP}.part6.${arch}.new2 \
    --pad ${padding} \
    --signpubkey ${DEVKEYS}/kernel_subkey.vbpubk > ${TMP}.verify6.new2

  # The input file should not have changed (just being sure).
  cmp ${TMP}.part1.${arch} ${TMP}.part1.${arch}.in
  # The verification should be indentical
  diff ${TMP}.verify6.old ${TMP}.verify6.new2
  # And creating a new output file should only emit a blob's worth
  cmp ${TMP}.part6.${arch} ${TMP}.part6.${arch}.new2

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
