#!/bin/sh -e
#
# Description
#   This script records the md5 hashes of host build objects
#
BIN_FILES=$(find $1 -executable -type f)
if [ "$2" = "-host" ]; then
  MD5_FILE="hmd5sums.txt"
fi

if [ "$2" = "-ec" ]; then
  MD5_FILE="ecmd5sums.txt"
fi

md5sum $BIN_FILES > $MD5_FILE
