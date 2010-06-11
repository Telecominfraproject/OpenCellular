#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Determine script directory.
if [[ $0 == '/'* ]]; 
then
  SCRIPT_DIR="`dirname $0`"
elif [[ $0 == './'* ]];
then
  SCRIPT_DIR="`pwd`"
else
  SCRIPT_DIR="`pwd`"/"`dirname $0`"
fi

UTIL_DIR=`dirname ${SCRIPT_DIR}`/utility
TEST_DIR=${SCRIPT_DIR}
TESTKEY_DIR=${SCRIPT_DIR}/testkeys
TESTCASE_DIR=${SCRIPT_DIR}/testcases

# Color output encodings.
COL_RED='\E[31;1m'
COL_GREEN='\E[32;1m'
COL_YELLOW='\E[33;1m'
COL_BLUE='\E[34;1m'
COL_STOP='\E[0;m'

hash_algos=( sha1 sha256 sha512 )
key_lengths=( 1024 2048 4096 8192 ) 

function happy {
  echo -e "${COL_GREEN}$*${COL_STOP}" 1>&2
}

function warning {
  echo -e "${COL_YELLOW}WARNING: $*${COL_STOP}" 1>&2
}

function error {
  echo -e "${COL_RED}ERROR: $*${COL_STOP}" 1>&2
  exit 1
}

function check_test_keys {
  [ -d ${TESTKEY_DIR} ] || \
    error "You must run gen_test_keys.sh to generate test keys first."
}

