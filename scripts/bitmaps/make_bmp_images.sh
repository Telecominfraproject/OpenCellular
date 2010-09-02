#!/bin/bash -e
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This adds text to our non-labeled recovery images.
#
# The source images should be 1366x800, with the expectation they'll be cropped
# to 1366x768 or 1280x800, have 2 lines of text overlayed at the bottom, and
# then be resized to 800x600 so that the BIOS can then display them stretched
# to the full screen size.
#

# Require one arg
if [ $# -ne "1" ]; then
  echo "Usage: $(basename $0) URL" 1>&2
  exit 1
fi
url=$1


# Image parameters
geom_orig='1366x800'
geom_crop_a='1366x768'
geom_crop_b='1280x800'
geom_final='800x600!'
font="Helvetica-Narrow"
pointsize=30


# Temporary files
tmpdir=$(mktemp -d /tmp/tmp.XXXXXXXXX)
trap "rm -rf $tmpdir" EXIT
img_orig="${tmpdir}/img_orig.bmp"
img_crop_a="${tmpdir}/img_crop_a.bmp"
img_crop_b="${tmpdir}/img_crop_b.bmp"
img_txt_a="${tmpdir}/img_txt_a.bmp"
img_txt_b="${tmpdir}/img_txt_b.bmp"
label_file="${tmpdir}/label.txt"

# Output directories
thisdir=$(readlink -e $(dirname $0))
outdir_a=${thisdir}/out_${geom_crop_a}
[ -d "$outdir_a" ] || mkdir -p "$outdir_a"
outdir_b=${thisdir}/out_${geom_crop_b}
[ -d "$outdir_b" ] || mkdir -p "$outdir_b"


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
  dst_dir_a="${outdir_a}/${root}"
  [ -d "$dst_dir_a" ] || mkdir -p "$dst_dir_a"
  dst_dir_b="${outdir_b}/${root}"
  [ -d "$dst_dir_b" ] || mkdir -p "$dst_dir_b"
  dst_img_a="${dst_dir_a}/${root}.bmp"
  dst_img_b="${dst_dir_b}/${root}.bmp"
  echo "processing $root ..."

  # First, make sure we start with the right-size original
  bg=$(find_background_color "$src_img")
  convert "$src_img" -background "$bg" \
    -gravity center -extent $geom_orig "$img_orig"

  # Now crop that to the two target sizes
  convert "$img_orig" -gravity Center \
    -crop "$geom_crop_a"+0+0 +repage "$img_crop_a"
  convert "$img_orig" -gravity Center \
    -crop "$geom_crop_b"+0+0 +repage "$img_crop_b"

  # Add the labels in
  if [ -r "$txt_file" ]; then
    # Replace all '$URL' in the URL in the text file with the real url
    perl -p \
      -e 'BEGIN {$/ = undef; $url = shift; }' \
      -e 's/\s+$/\n/gs; s/\$URL/$url/gs;' \
      "$url" "$txt_file" > "$label_file"
    # Render it
    convert "$img_crop_a" -fill white \
      -font "$font" -pointsize "$pointsize" -interline-spacing 5 \
      -gravity south -annotate '+0+0' '@'"$label_file" "$img_txt_a"
    convert "$img_crop_b" -fill white \
      -font "$font" -pointsize "$pointsize" -interline-spacing 5 \
      -gravity south -annotate '+0+0' '@'"$label_file" "$img_txt_b"
  else
    mv "$img_crop_a" "$img_txt_a"
    mv "$img_crop_b" "$img_txt_b"
  fi

  # Now scale the result to the final size
  convert "$img_txt_a" -scale "$geom_final" -alpha off "$dst_img_a"
  convert "$img_txt_b" -scale "$geom_final" -alpha off "$dst_img_b"
}


# Do it.
for file in originals/*.gif; do
  process_one_file "$file"
done

# Zip up the bitmaps
nicename=${url// /_}
(cd "$outdir_a" && zip -qr "${thisdir}/out_${nicename}__${geom_crop_a}.zip" *)
(cd "$outdir_b" && zip -qr "${thisdir}/out_${nicename}__${geom_crop_b}.zip" *)
