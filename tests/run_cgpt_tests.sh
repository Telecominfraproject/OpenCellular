#!/bin/bash -eu

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Run tests for CGPT.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

GPT=$(readlink -f "$1")
[ -x "$GPT" ] || error "Can't execute $GPT"

# Run tests in a dedicated directory for easy cleanup or debugging.
DIR="${TEST_DIR}/cgpt_test_dir"
[ -d "$DIR" ] || mkdir -p "$DIR"
warning "testing $GPT in $DIR"
cd "$DIR"

echo "Create an empty file to use as the device..."
NUM_SECTORS=1000
DEV=fake_dev.bin
dd if=/dev/zero of=${DEV} conv=notrunc bs=512 count=${NUM_SECTORS} 2>/dev/null


echo "Create a bunch of partitions, using the real GUID types..."
DATA_START=100
DATA_SIZE=20
DATA_LABEL="data stuff"
DATA_GUID='ebd0a0a2-b9e5-4433-87c0-68b6b72699c7'
DATA_NUM=1

KERN_START=200
KERN_SIZE=30
KERN_LABEL="kernel stuff"
KERN_GUID='fe3a2a5d-4f32-41a7-b725-accc3285a309'
KERN_NUM=2

ROOTFS_START=300
ROOTFS_SIZE=40
ROOTFS_LABEL="rootfs stuff"
ROOTFS_GUID='3cb8e202-3b7e-47dd-8a3c-7ff2a13cfcec'
ROOTFS_NUM=3

ESP_START=400
ESP_SIZE=50
ESP_LABEL="ESP stuff"
ESP_GUID='c12a7328-f81f-11d2-ba4b-00a0c93ec93b'
ESP_NUM=4

FUTURE_START=500
FUTURE_SIZE=60
FUTURE_LABEL="future stuff"
FUTURE_GUID='2e0a753d-9e48-43b0-8337-b15192cb1b5e'
FUTURE_NUM=5

RANDOM_START=600
RANDOM_SIZE=70
RANDOM_LABEL="random stuff"
RANDOM_GUID='2364a860-bf63-42fb-a83d-9ad3e057fcf5'
RANDOM_NUM=6

$GPT create ${DEV}

$GPT add -b ${DATA_START} -s ${DATA_SIZE} -t ${DATA_GUID} \
  -l "${DATA_LABEL}" ${DEV}
$GPT add -b ${KERN_START} -s ${KERN_SIZE} -t ${KERN_GUID} \
  -l "${KERN_LABEL}" ${DEV}
$GPT add -b ${ROOTFS_START} -s ${ROOTFS_SIZE} -t ${ROOTFS_GUID} \
  -l "${ROOTFS_LABEL}" ${DEV}
$GPT add -b ${ESP_START} -s ${ESP_SIZE} -t ${ESP_GUID} \
  -l "${ESP_LABEL}" ${DEV}
$GPT add -b ${FUTURE_START} -s ${FUTURE_SIZE} -t ${FUTURE_GUID} \
  -l "${FUTURE_LABEL}" ${DEV}
$GPT add -b ${RANDOM_START} -s ${RANDOM_SIZE} -t ${RANDOM_GUID} \
  -l "${RANDOM_LABEL}" ${DEV}


echo "Extract the start and size of given partitions..."

X=$($GPT show -b -i $DATA_NUM ${DEV})
Y=$($GPT show -s -i $DATA_NUM ${DEV})
[ "$X $Y" = "$DATA_START $DATA_SIZE" ] || error "fail at line $LINENO"

X=$($GPT show -b -i $KERN_NUM ${DEV})
Y=$($GPT show -s -i $KERN_NUM ${DEV})
[ "$X $Y" = "$KERN_START $KERN_SIZE" ] || error "fail at line $LINENO"

X=$($GPT show -b -i $ROOTFS_NUM ${DEV})
Y=$($GPT show -s -i $ROOTFS_NUM ${DEV})
[ "$X $Y" = "$ROOTFS_START $ROOTFS_SIZE" ] || error "fail at line $LINENO"

X=$($GPT show -b -i $ESP_NUM ${DEV})
Y=$($GPT show -s -i $ESP_NUM ${DEV})
[ "$X $Y" = "$ESP_START $ESP_SIZE" ] || error "fail at line $LINENO"

X=$($GPT show -b -i $FUTURE_NUM ${DEV})
Y=$($GPT show -s -i $FUTURE_NUM ${DEV})
[ "$X $Y" = "$FUTURE_START $FUTURE_SIZE" ] || error "fail at line $LINENO"

X=$($GPT show -b -i $RANDOM_NUM ${DEV})
Y=$($GPT show -s -i $RANDOM_NUM ${DEV})
[ "$X $Y" = "$RANDOM_START $RANDOM_SIZE" ] || error "fail at line $LINENO"


echo "Set the boot partition.."
$GPT boot -i ${KERN_NUM} ${DEV} >/dev/null

echo "Check the PMBR's idea of the boot partition..."
X=$($GPT boot ${DEV})
Y=$($GPT show -u -i $KERN_NUM $DEV)
[ "$X" = "$Y" ] || error "fail at line $LINENO"

echo "Done."

happy "All tests passed."
