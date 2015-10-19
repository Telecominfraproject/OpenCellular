#!/bin/bash
#
# Copyright (c) 2015, NVIDIA CORPORATION.  All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See file CREDITS for list of people who contributed to this
# project.
#
set -e
IMAGE_FILE=$1
KEY_FILE=$2
TARGET_IMAGE=$IMAGE_FILE
CONFIG_FILE=config.tmp

CBOOTIMAGE=../src/cbootimage
BCT_DUMP=../src/bct_dump
OBJCOPY=objcopy
OPENSSL=openssl
DD=dd
RM=rm
MV=mv
XXD=xxd
CUT=cut

echo "Get rid of all temporary files: *.sig, *.tosig, *.tmp *.mod"
$RM -f *.sig *.tosig *.tmp *.mod

echo "Get bl length "
BL_LENGTH=`$BCT_DUMP $IMAGE_FILE | grep "Bootloader\[0\].Length"\
 | awk -F ' ' '{print $4}' | awk -F ';' '{print $1}'`

echo "Extract bootloader to $IMAGE_FILE.bl.tosig, length $BL_LENGTH"
$DD bs=1 skip=32768 if=$IMAGE_FILE of=$IMAGE_FILE.bl.tosig count=$BL_LENGTH

echo "Calculate rsa signature for bootloader and save to $IMAGE_FILE.bl.sig"
$OPENSSL dgst -sha256 -sigopt rsa_padding_mode:pss -sigopt rsa_pss_saltlen:-1 \
 -sign $KEY_FILE -out $IMAGE_FILE.bl.sig $IMAGE_FILE.bl.tosig

echo "Update bootloader's rsa signature, aes hash and bct's aes hash"
echo "RsaPssSigBlFile = $IMAGE_FILE.bl.sig;" > $CONFIG_FILE
echo "RehashBl;" >> $CONFIG_FILE
$CBOOTIMAGE -s tegra210 -u $CONFIG_FILE $IMAGE_FILE $IMAGE_FILE.tmp

echo "Extract the part of bct which needs to be rsa signed"
$DD bs=1 if=$IMAGE_FILE.tmp of=$IMAGE_FILE.bct.tosig count=8944 skip=1296

echo "Calculate rsa signature for bct and save to $IMAGE_FILE.bct.sig"
$OPENSSL dgst -sha256 -sigopt rsa_padding_mode:pss -sigopt rsa_pss_saltlen:-1 \
 -sign $KEY_FILE -out $IMAGE_FILE.bct.sig $IMAGE_FILE.bct.tosig

echo "Create public key modulus from key file $KEY_FILE and save to $KEY_FILE.mod"
$OPENSSL rsa -in $KEY_FILE -noout -modulus -out $KEY_FILE.mod
# remove prefix
$CUT -d= -f2 < $KEY_FILE.mod > $KEY_FILE.mod.tmp

# convert from hexdecimal to binary
$XXD -r -p -l 256 $KEY_FILE.mod.tmp $KEY_FILE.mod.bin

echo "Update bct's rsa signature and modulus"
echo "RsaPssSigBctFile = $IMAGE_FILE.bct.sig;" > $CONFIG_FILE
echo "RsaKeyModulusFile = $KEY_FILE.mod.bin;" >> $CONFIG_FILE
$CBOOTIMAGE -s tegra210 -u $CONFIG_FILE $IMAGE_FILE.tmp $TARGET_IMAGE
