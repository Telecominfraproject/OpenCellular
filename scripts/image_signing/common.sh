#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Determine script directory
SCRIPT_DIR=$(dirname $0)
PROG=$(basename $0)
GPT=cgpt

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
  dd if=$image of=$output_file bs=512 skip=$offset count=$size
}
  
