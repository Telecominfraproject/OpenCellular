#!/bin/bash -e
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script searches the directory tree passed in as the parameter for
# bmp_*.fv files, figures out the FWID strings from files' names, verifies the
# FWIDs' integrity (by recalculating the CRC included in the FWID string) and
# then rebuilds the bitmaps with the appropriate text and target specific
# geometry.
#


# Given a string "<prefix>_<el1>_<el2>_.._<eln>_<suffix>" print string
# '<el1> <el2> .. <eln>', i.e. <prefix>_ and _<suffix> dropped and underscores
# replaced with spaces.
get_elements() {
  echo $1 | awk 'BEGIN {FS="_"}; {
    x = 2;
    do {
      printf "%s ", $x;
      x += 1
    } while (x < (NF - 1))
    printf "%s", $(NF-1);
  }'
}

# Concatenate input parameters into a space separated string, calculate the
# string's CRC32 and print the last four hex digits of the crc.
signer() {
   python -c "import sys,zlib;
me=' '.join(sys.argv[1:]);
print ('%04u'%(zlib.crc32(me)&0xffffffffL))[-4:]" $1
}


if [ "$#" != "1" -o ! -d "$1" ]; then
   echo "One parameter is required, the path to the chromeos release tree" >&2
   exit 1
fi

tree=$(readlink -f $1)
cd $(dirname "$0")
for f in $(find "${tree}" -type f -name 'bmp_*_[0-9]*.fv'); do
  filename=$(basename "$f")
  elements="$(get_elements $filename)"
  signature=$(signer "${elements}")

  # Rebuild file name to verify CRC.
  comp_name=bmp_${elements// /_}_${signature}.fv
  if [ "${filename}" !=  "${comp_name}" ]; then
    echo "skipping ${filename} (crc mismatch with ${comp_name})"
    continue
  fi
  echo "Processing ${filename}"
  case "${elements}" in
    (*ACER*) geometry='1366x768'
      ;;
    (*MARIO*) geometry='1280x800'
      ;;
    (*) echo "skipping ${filename}, unknown target geometry"
      echo
      continue
    ;;
  esac
  ./make_bmp_images.sh "${elements} ${signature}" "${geometry}" "x86"
  echo
done
