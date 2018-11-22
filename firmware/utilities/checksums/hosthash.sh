#!/bin/sh -e
#
# Description
#   This script records the md5 hashes of host build objects
#
BIN_FILES=$(find $1 -executable -type f)
HOST_MD5_FILE="hmd5sums.txt"
md5sum $BIN_FILES > $HOST_MD5_FILE
