#!/bin/bash -eux

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Refer to the Google Chrome OS Main Processor Firmware Specification for what
# the pieces are.
# This script generates different firmware binaries with different
# configurations.

# Syntax: ./firmware_boot.sh <Firmware name without .fd extension>.
# Usage of the script.
usage()
{
  cat <<EOF
  $0 firmware_name
  firmware_name - name of the firmware.
EOF
}

if [ $# != 1 ]; then
  usage
  exit 0
fi

base=$1
input=${base}.fd

if [ ! -f $input ]; then
  echo "$input file does not exists."
  exit 0
fi

# First, run dump_fmap $input | ./x to compute these values:

# dev-mode BIOS is in firmware A
rw_a_offset=$(dump_fmap -p ${input} | grep 'RW_SECTION_A' | cut -d' ' -f2)
rw_a_size=$(dump_fmap -p ${input} | grep 'RW_SECTION_A' | cut -d' ' -f3)

# normal-mode BIOS is in firmware B
rw_b_offset=$(dump_fmap -p ${input} | grep 'RW_SECTION_B' | cut -d' ' -f2)
rw_b_size=$(dump_fmap -p ${input} | grep 'RW_SECTION_B' | cut -d' ' -f3)

# Extract the RW BIOS chunks
dd if=${input} of=dev.bin bs=1 skip=${rw_a_offset} count=${rw_a_size}
dd if=${input} of=nor.bin bs=1 skip=${rw_b_offset} count=${rw_b_size}

# Garble one to make it fail the signature. I know that we reserve 64K at the
# start of the section for the signature and headers, so we'll make a random
# payload and put the normal header on the front.
dd if=/dev/urandom of=bad.bin bs=1 count=${rw_b_size}
dd if=nor.bin of=bad.bin conv=notrunc bs=1 count=65536

# A:Normal B:Normal
output=${base}-NN.fd
cp ${input} ${output}
dd if=nor.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=nor.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Dev    B:Dev
output=${base}-DD.fd
cp ${input} ${output}
dd if=dev.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=dev.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Normal B:Dev
output=${base}-ND.fd
cp ${input} ${output}
dd if=nor.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=dev.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Dev    B:Normal
output=${base}-DN.fd
cp ${input} ${output}
dd if=dev.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=nor.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Normal B:Bad
output=${base}-NB.fd
cp ${input} ${output}
dd if=nor.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=bad.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Bad    B:Normal
output=${base}-BN.fd
cp ${input} ${output}
dd if=bad.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=nor.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Dev    B:Bad
output=${base}-DB.fd
cp ${input} ${output}
dd if=dev.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=bad.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}

# A:Bad    B:Dev
output=${base}-BD.fd
cp ${input} ${output}
dd if=bad.bin of=${output} conv=notrunc bs=1 seek=${rw_a_offset}
dd if=dev.bin of=${output} conv=notrunc bs=1 seek=${rw_b_offset}
