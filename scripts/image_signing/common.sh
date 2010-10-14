#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Determine script directory
SCRIPT_DIR=$(dirname $0)
PROG=$(basename $0)
GPT=cgpt

# The tag when the rootfs is changed.
TAG_NEEDS_TO_BE_SIGNED="/root/.need_to_be_signed"

# Load shflags
if [ -f /usr/lib/shflags ]; then
  . /usr/lib/shflags
else
  . "${SCRIPT_DIR}/lib/shflags/shflags"
fi

# List of Temporary files and mount points.
TEMP_FILE_LIST=$(mktemp)
TEMP_DIR_LIST=$(mktemp)

# Read GPT table to find the starting location of a specific partition.
# Args: DEVICE PARTNUM
# Returns: offset (in sectors) of partition PARTNUM
partoffset() {
  sudo $GPT show -b -i $2 $1
}

# Read GPT table to find the size of a specific partition.
# Args: DEVICE PARTNUM
# Returns: size (in sectors) of partition PARTNUM
partsize() {
  sudo $GPT show -s -i $2 $1
}

# Tags a file system as "needs to be resigned".
# Args: MOUNTDIRECTORY
tag_as_needs_to_be_resigned() {
  local mount_dir="$1"
  sudo touch "$mount_dir/$TAG_NEEDS_TO_BE_SIGNED"
}

# Determines if the target file system has the tag for resign
# Args: MOUNTDIRECTORY
# Returns: $FLAGS_TRUE if the tag is there, otherwise $FLAGS_FALSE
has_needs_to_be_resigned_tag() {
  local mount_dir="$1"
  if [ -f "$mount_dir/$TAG_NEEDS_TO_BE_SIGNED" ]; then
    return ${FLAGS_TRUE}
  else
    return ${FLAGS_FALSE}
  fi
}

# Determines if the target file system is a Chrome OS root fs
# Args: MOUNTDIRECTORY
# Returns: $FLAGS_TRUE if MOUNTDIRECTORY looks like root fs,
#          otherwise $FLAGS_FALSE
is_rootfs_partition() {
  local mount_dir="$1"
  if [ -f "$mount_dir/$(dirname "$TAG_NEEDS_TO_BE_SIGNED")" ]; then
    return ${FLAGS_TRUE}
  else
    return ${FLAGS_FALSE}
  fi
}

# Mount a partition read-only from an image into a local directory
# Args: IMAGE PARTNUM MOUNTDIRECTORY
mount_image_partition_ro() {
  local image=$1
  local partnum=$2
  local mount_dir=$3
  local offset=$(partoffset "$image" "$partnum")
  sudo mount -o loop,ro,offset=$((offset * 512)) "$image" "$mount_dir"
}

# Mount a partition from an image into a local directory
# Args: IMAGE PARTNUM MOUNTDIRECTORY
mount_image_partition() {
  local image=$1
  local partnum=$2
  local mount_dir=$3
  local offset=$(partoffset "$image" "$partnum")
  sudo mount -o loop,offset=$((offset * 512)) "$image" "$mount_dir"
  if is_rootfs_partition "$mount_dir"; then
    tag_as_needs_to_be_resigned "$mount_dir"
  fi
}

# Extract a partition to a file
# Args: IMAGE PARTNUM OUTPUTFILE
extract_image_partition() {
  local image=$1
  local partnum=$2
  local output_file=$3
  local offset=$(partoffset "$image" "$partnum")
  local size=$(partsize "$image" "$partnum")
  dd if=$image of=$output_file bs=512 skip=$offset count=$size conv=notrunc >/dev/null 2>&1
}

# Replace a partition in an image from file
# Args: IMAGE PARTNUM INPUTFILE
replace_image_partition() {
  local image=$1
  local partnum=$2
  local input_file=$3
  local offset=$(partoffset "$image" "$partnum")
  local size=$(partsize "$image" "$partnum")
  dd if=$input_file of=$image bs=512 seek=$offset count=$size conv=notrunc
}

# Create a new temporary file and return its name.
# File is automatically cleaned when cleanup_temps_and_mounts() is called.
make_temp_file() {
  local tempfile=$(mktemp)
  echo "$tempfile" >> $TEMP_FILE_LIST
  echo $tempfile
}

# Create a new temporary directory and return its name.
# Directory is automatically deleted and any filesystem mounted on it unmounted
# when cleanup_temps_and_mounts() is called.
make_temp_dir() {
  local tempdir=$(mktemp -d)
  echo "$tempdir" >> $TEMP_DIR_LIST
  echo $tempdir
}

cleanup_temps_and_mounts() {
  for i in "$(cat $TEMP_FILE_LIST)"; do
    rm -f $i
  done
  set +e  # umount may fail for unmounted directories
  for i in "$(cat $TEMP_DIR_LIST)"; do
    if [ -n "$i" ]; then
      if has_needs_to_be_resigned_tag "$i"; then
        echo "Warning: image may be modified. Please resign image."
      fi
      sudo umount -d $i 2>/dev/null
      rm -rf $i
    fi
  done
  set -e
  rm -rf $TEMP_DIR_LIST $TEMP_FILE_LIST
}

trap "cleanup_temps_and_mounts" EXIT

