#!/bin/bash -e
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This adds text to our non-labeled recovery images.
#
# The source images should be 1366x800, with the expectation they'll be cropped
# to <screen geometry>, which is 1366x768 or 1280x800, have 2 lines of text
# overlayed at the bottom, and then be resized to 800x600 if on x86, otherwise
# same as <screen geometry>. On x86, resizing to 800x600 because the BIOS can
# then display them stretched to the full screen size.
#


# Require three args
if [ $# -ne "3" -o \( "$3" != "x86" -a "$3" != "arm" \) ]; then
  echo "Usage: $(basename $0) <HWID> <screen geometry> <x86/arm>" 1>&2
  exit 1
fi
HWID=$1
geom_crop=$2
geom_final='800x600!'
flag_final=
# If arm, make the final geometry as screen size and use 8bpp rle format.
if [ $3 = "arm" ]; then
  geom_final=$2
  flag_final="-colors 256 -compress rle"
fi

nicename=${HWID// /_}

# Default URL
URL='http://google.com/chromeos/recovery'

# Image parameters
geom_orig='1366x800'
bluecolor='#9ccaec'
bluefont="Helvetica-Narrow"
bluepointsize=30
whitefont="Helvetica-Narrow"
whitepointsize=48


# Temporary files
tmpdir=$(mktemp -d /tmp/tmp.bmp.XXXXXX)
trap "rm -rf $tmpdir" EXIT
img_orig="${tmpdir}/img_orig.bmp"
img_crop="${tmpdir}/img_crop.bmp"
img_txt="${tmpdir}/img_txt.bmp"
label_file="${tmpdir}/label.txt"
label_img="${tmpdir}/label.bmp"

# Output directories
thisdir=$(readlink -e $(dirname $0))
outdir="out_${nicename}"
[ -d "$outdir" ] || mkdir -p "$outdir"

function find_background_color {
  src_img=$1
  convert "$src_img" -crop '1x1+10+10!' txt:- | \
    perl -n -e 'print "$1" if m/(#[0-9a-f]+)/i;'
}

function process_one_file {
  src_img=$1

  # Figure out the filenames to use
  txt_file=${src_img%*.gif}.txt
  root=$(basename "$src_img")
  root=${root%*.*}
  # one more layer of heirarchy to match BIOS source tree
  dst_dir="${outdir}/${root}"
  [ -d "$dst_dir" ] || mkdir -p "$dst_dir"
  dst_img="${dst_dir}/${root}.bmp"
  echo "processing $root ..."

  # First, make sure we start with the right-size original
  bg=$(find_background_color "$src_img")
  convert "$src_img" -background "$bg" \
    -gravity center -extent $geom_orig "$img_orig"

  # Now crop that to the two target sizes
  convert "$img_orig" -gravity Center \
    -crop "$geom_crop"+0+0 +repage "$img_crop"

  # Add the labels in
  if [ -r "$txt_file" ]; then
    # The only way to change font and color in multiline text is to split each
    # line into a separate image and then composite them together. Ugh.
    # First, split each input line into a separate file.
    "${thisdir}/makelines.sh" -u "$URL" -m "$HWID" -d "$tmpdir" "$txt_file"
    # Convert each line file into an image file.
    for txtfile in ${tmpdir}/linetxt_*; do
      case "$txtfile" in
        *.txt)
          convert \
            -background "$bg" -fill "$bluecolor" \
            -font "$bluefont" -pointsize "$bluepointsize" \
            -bordercolor "$bg" -border 0x1 \
            label:'@'"$txtfile" "${txtfile%.*}".bmp
          ;;
        *.TXT)
          convert \
            -background "$bg" -fill "white" \
            -font "$whitefont" -pointsize "$whitepointsize" \
            -bordercolor "$bg" -border 0x10 \
            label:'@'"$txtfile" "${txtfile%.*}".bmp
          ;;
      esac
    done
    # Now bash them all together to make one image.
    convert -background "$bg" -gravity center ${tmpdir}/linetxt_*.bmp \
      label:'\n\n\n\n' -append "$label_img"
    # Finally, layer the label image on top of the original.
    composite "$label_img" -gravity south "$img_crop" "$img_txt"
  else
    mv "$img_crop" "$img_txt"
  fi

  # Now scale the result to the final size
  convert "$img_txt" -scale "$geom_final" -alpha off $flag_final "$dst_img"
}


# Do it.
for file in ${thisdir}/originals/*.gif; do
  process_one_file "$file"
done

# Zip up the bitmaps
(cd "$outdir" && zip -qr "${geom_crop}.zip" *)
