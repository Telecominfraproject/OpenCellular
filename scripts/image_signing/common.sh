#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Determine script directory
SCRIPT_DIR=$(dirname $0)
PROG=$(basename $0)
GPT=cgpt

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

# Mount a partition from an image into a local directory
# Args: IMAGE PARTNUM MOUNTDIRECTORY
mount_image_partition() {
  local image=$1
  local partnum=$2
  local mount_dir=$3
  local offset=$(partoffset "$image" "$partnum")
  sudo mount -o loop,offset=$((offset * 512)) "$image" "$mount_dir"
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
      sudo umount -d $i 2>/dev/null
      rm -rf $i
    fi
  done
  set -e
  rm -rf $TEMP_DIR_LIST $TEMP_FILE_LIST
}

trap "cleanup_temps_and_mounts" EXIT

