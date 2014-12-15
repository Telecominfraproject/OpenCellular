#!/bin/bash -eu

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Run tests for cgpt utility.

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

CGPT=$(readlink -f "$1")
[ -x "$CGPT" ] || error "Can't execute $CGPT"

MTD="${@:2}"

# Run tests in a dedicated directory for easy cleanup or debugging.
DIR="${TEST_DIR}/cgpt_test_dir"
[ -d "$DIR" ] || mkdir -p "$DIR"
warning "testing $CGPT in $DIR"
cd "$DIR"

assert_fail() {
  set +e
  "$@" 2>/dev/null
  if [ $? == 0 ]; then
    error "$*" " should have failed but did not"
  fi
  set -e
}

# Test failure on non existing file.
assert_fail ${CGPT} show $MTD blah_404_haha

echo "Create an empty file to use as the device..."
NUM_SECTORS=1000
DEV=fake_dev.bin
rm -f ${DEV}
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

$CGPT create $MTD ${DEV}

$CGPT add $MTD -b ${DATA_START} -s ${DATA_SIZE} -t ${DATA_GUID} \
  -l "${DATA_LABEL}" ${DEV}
$CGPT add $MTD -b ${KERN_START} -s ${KERN_SIZE} -t ${KERN_GUID} \
  -l "${KERN_LABEL}" ${DEV}
$CGPT add $MTD -b ${ROOTFS_START} -s ${ROOTFS_SIZE} -t ${ROOTFS_GUID} \
  -l "${ROOTFS_LABEL}" ${DEV}
$CGPT add $MTD -b ${ESP_START} -s ${ESP_SIZE} -t ${ESP_GUID} \
  -l "${ESP_LABEL}" ${DEV}
$CGPT add $MTD -b ${FUTURE_START} -s ${FUTURE_SIZE} -t ${FUTURE_GUID} \
  -l "${FUTURE_LABEL}" ${DEV}
$CGPT add $MTD -b ${RANDOM_START} -s ${RANDOM_SIZE} -t ${RANDOM_GUID} \
  -l "${RANDOM_LABEL}" ${DEV}


echo "Extract the start and size of given partitions..."

X=$($CGPT show $MTD -b -i $DATA_NUM ${DEV})
Y=$($CGPT show $MTD -s -i $DATA_NUM ${DEV})
[ "$X $Y" = "$DATA_START $DATA_SIZE" ] || error

X=$($CGPT show $MTD -b -i $KERN_NUM ${DEV})
Y=$($CGPT show $MTD -s -i $KERN_NUM ${DEV})
[ "$X $Y" = "$KERN_START $KERN_SIZE" ] || error

X=$($CGPT show $MTD -b -i $ROOTFS_NUM ${DEV})
Y=$($CGPT show $MTD -s -i $ROOTFS_NUM ${DEV})
[ "$X $Y" = "$ROOTFS_START $ROOTFS_SIZE" ] || error

X=$($CGPT show $MTD -b -i $ESP_NUM ${DEV})
Y=$($CGPT show $MTD -s -i $ESP_NUM ${DEV})
[ "$X $Y" = "$ESP_START $ESP_SIZE" ] || error

X=$($CGPT show $MTD -b -i $FUTURE_NUM ${DEV})
Y=$($CGPT show $MTD -s -i $FUTURE_NUM ${DEV})
[ "$X $Y" = "$FUTURE_START $FUTURE_SIZE" ] || error

X=$($CGPT show $MTD -b -i $RANDOM_NUM ${DEV})
Y=$($CGPT show $MTD -s -i $RANDOM_NUM ${DEV})
[ "$X $Y" = "$RANDOM_START $RANDOM_SIZE" ] || error


echo "Change the beginning..."
DATA_START=$((DATA_START + 10))
$CGPT add $MTD -i 1 -b ${DATA_START} ${DEV} || error
X=$($CGPT show $MTD -b -i 1 ${DEV})
[ "$X" = "$DATA_START" ] || error

echo "Change the size..."
DATA_SIZE=$((DATA_SIZE + 10))
$CGPT add $MTD -i 1 -s ${DATA_SIZE} ${DEV} || error
X=$($CGPT show $MTD -s -i 1 ${DEV})
[ "$X" = "$DATA_SIZE" ] || error

echo "Change the type..."
$CGPT add $MTD -i 1 -t reserved ${DEV} || error
X=$($CGPT show $MTD -t -i 1 ${DEV} | tr 'A-Z' 'a-z')
[ "$X" = "$FUTURE_GUID" ] || error
# arbitrary value
$CGPT add $MTD -i 1 -t 610a563a-a55c-4ae0-ab07-86e5bb9db67f ${DEV} || error
X=$($CGPT show $MTD -t -i 1 ${DEV})
[ "$X" = "610A563A-A55C-4AE0-AB07-86E5BB9DB67F" ] || error

$CGPT add $MTD -i 1 -t data ${DEV} || error
X=$($CGPT show $MTD -t -i 1 ${DEV} | tr 'A-Z' 'a-z')
[ "$X" = "$DATA_GUID" ] || error


echo "Set the boot partition.."
$CGPT boot $MTD -i ${KERN_NUM} ${DEV} >/dev/null

echo "Check the PMBR's idea of the boot partition..."
X=$($CGPT boot $MTD ${DEV})
Y=$($CGPT show $MTD -u -i $KERN_NUM $DEV)
[ "$X" = "$Y" ] || error

echo "Test the cgpt prioritize command..."

# Input: sequence of priorities
# Output: ${DEV} has kernel partitions with the given priorities
make_pri() {
  local idx=0
  $CGPT create $MTD ${DEV}
  for pri in "$@"; do
    idx=$((idx+1))
    $CGPT add $MTD -t kernel -l "kern$idx" -b $((100 + 2 * $idx)) -s 1 -P $pri ${DEV}
  done
}

# Output: returns string containing priorities of all kernels
get_pri() {
  echo $(
  for idx in $($CGPT find $MTD -t kernel ${DEV} | sed -e s@${DEV}@@); do
    $CGPT show $MTD -i $idx -P ${DEV}
  done
  )
}

# Input: list of priorities
# Operation: expects ${DEV} to contain those kernel priorities
assert_pri() {
  local expected="$*"
  local actual=$(get_pri)
  [ "$actual" = "$expected" ] || \
    error 1 "expected priority \"$expected\", actual priority \"$actual\""
}

# no kernels at all. This should do nothing.
$CGPT create $MTD ${DEV}
$CGPT add $MTD -t rootfs -b 100 -s 1 ${DEV}
$CGPT prioritize $MTD ${DEV}
assert_pri ""

# common install/upgrade sequence
make_pri   2 0 0
$CGPT prioritize $MTD -i 1 ${DEV}
assert_pri 1 0 0
$CGPT prioritize $MTD -i 2 ${DEV}
assert_pri 1 2 0
$CGPT prioritize $MTD -i 1 ${DEV}
assert_pri 2 1 0
$CGPT prioritize $MTD -i 2 ${DEV}
assert_pri 1 2 0
# lots of kernels, all same starting priority, should go to priority 1
make_pri   8 8 8 8 8 8 8 8 8 8 8 0 0 8
$CGPT prioritize $MTD ${DEV}
assert_pri 1 1 1 1 1 1 1 1 1 1 1 0 0 1

# now raise them all up again
$CGPT prioritize $MTD -P 4 ${DEV}
assert_pri 4 4 4 4 4 4 4 4 4 4 4 0 0 4

# set one of them higher, should leave the rest alone
$CGPT prioritize $MTD -P 5 -i 3 ${DEV}
assert_pri 4 4 5 4 4 4 4 4 4 4 4 0 0 4

# set one of them lower, should bring the rest down
$CGPT prioritize $MTD -P 3 -i 4 ${DEV}
assert_pri 1 1 2 3 1 1 1 1 1 1 1 0 0 1

# raise a group by including the friends of one partition
$CGPT prioritize $MTD -P 6 -i 1 -f ${DEV}
assert_pri 6 6 4 5 6 6 6 6 6 6 6 0 0 6

# resurrect one, should not affect the others
make_pri   0 0 0 0 0 0 0 0 0 0 0 0 0 0
$CGPT prioritize $MTD -i 2 ${DEV}
assert_pri 0 1 0 0 0 0 0 0 0 0 0 0 0 0

# resurrect one and all its friends
make_pri   0 0 0 0 0 0 0 0 1 2 0 0 0 0
$CGPT prioritize $MTD -P 5 -i 2 -f ${DEV}
assert_pri 5 5 5 5 5 5 5 5 3 4 5 5 5 5

# no options should maintain the same order
$CGPT prioritize $MTD ${DEV}
assert_pri 3 3 3 3 3 3 3 3 1 2 3 3 3 3

# squish all the ranks
make_pri   1 1 2 2 3 3 4 4 5 5 0 6 7 7
$CGPT prioritize $MTD -P 6 ${DEV}
assert_pri 1 1 1 1 2 2 3 3 4 4 0 5 6 6

# squish the ranks by not leaving room
make_pri   1 1 2 2 3 3 4 4 5 5 0 6 7 7
$CGPT prioritize $MTD -P 7 -i 3 ${DEV}
assert_pri 1 1 7 1 2 2 3 3 4 4 0 5 6 6

# squish the ranks while bringing the friends along
make_pri   1 1 2 2 3 3 4 4 5 5 0 6 7 7
$CGPT prioritize $MTD -P 6 -i 3 -f ${DEV}
assert_pri 1 1 6 6 1 1 2 2 3 3 0 4 5 5

# squish them pretty hard
make_pri   1 1 2 2 3 3 4 4 5 5 0 6 7 7
$CGPT prioritize $MTD -P 2 ${DEV}
assert_pri 1 1 1 1 1 1 1 1 1 1 0 1 2 2

# squish them really really hard (nobody gets reduced to zero, though)
make_pri   1 1 2 2 3 3 4 4 5 5 0 6 7 7
$CGPT prioritize $MTD -P 1 -i 3 ${DEV}
assert_pri 1 1 1 1 1 1 1 1 1 1 0 1 1 1

make_pri   15 15 14 14 13 13 12 12 11 11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0
$CGPT prioritize $MTD -i 3 ${DEV}
assert_pri 14 14 15 13 12 12 11 11 10 10  9  9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 1 1 0
$CGPT prioritize $MTD -i 5 ${DEV}
assert_pri 13 13 14 12 15 11 10 10  9  9  8  8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 1 1 1 1 0
# but if I bring friends I don't have to squish
$CGPT prioritize $MTD -i 1 -f ${DEV}
assert_pri 15 15 13 12 14 11 10 10  9  9  8  8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 1 1 1 1 0

# Now make sure that we don't need write access if we're just looking.
echo "Test read vs read-write access..."
chmod 0444 ${DEV}

# These should fail
$CGPT create $MTD -z ${DEV} 2>/dev/null && error
$CGPT add $MTD -i 2 -P 3 ${DEV} 2>/dev/null && error
$CGPT repair $MTD ${DEV} 2>/dev/null && error
$CGPT prioritize $MTD -i 3 ${DEV} 2>/dev/null && error

# Most 'boot' usage should fail too.
$CGPT boot $MTD -p ${DEV} 2>/dev/null && error
dd if=/dev/zero of=fake_mbr.bin bs=100 count=1 2>/dev/null
$CGPT boot $MTD -b fake_mbr.bin ${DEV} 2>/dev/null && error
$CGPT boot $MTD -i 2 ${DEV} 2>/dev/null && error

$CGPT boot $MTD ${DEV} >/dev/null
$CGPT show $MTD ${DEV} >/dev/null
$CGPT find $MTD -t kernel ${DEV} >/dev/null

# Enable write access again to test boundary in off device storage
chmod 600 ${DEV}
# GPT too small
dd if=/dev/zero of=${DEV} bs=5632 count=1
assert_fail $CGPT create -D 1024 ${DEV}
# GPT is just right for 16 entries (512 + 512 + 16 * 128) * 2 = 6144
dd if=/dev/zero of=${DEV} bs=6144 count=1
$CGPT create -D 1024 ${DEV}
# Create a small 8K file to simulate Flash NOR section
dd if=/dev/zero of=${DEV} bs=8K count=1
# Drive size is not multiple of 512
assert_fail $CGPT create -D 511 ${DEV}
assert_fail $CGPT create -D 513 ${DEV}
MTD="-D 1024"
# Create a GPT table for a device of 1024 bytes (2 sectors)
$CGPT create $MTD ${DEV}
# Make sure number of entries is reasonable for 8KiB GPT
X=$($CGPT show -D 1024 -d ${DEV} | grep -c "Number of entries: 24")
[ "$X" = "2" ] || error
# This fails because header verification is off due to different drive size
assert_fail $CGPT show ${DEV}
# But this passes because we pass in correct drive size
$CGPT show $MTD ${DEV}
# This fails because beginning sector is over the size of the device
assert_fail $CGPT add $MTD -b 2 -s 1 -t data ${DEV}
# This fails because partition size is over the size of the device
assert_fail $CGPT add $MTD -b 0 -s 3 -t data ${DEV}


echo "Done."

happy "All tests passed."
