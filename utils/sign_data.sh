#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ $# -ne 3 ]
then
  echo "Usage: `basename $0` <algorithm> <key file> <input file>"
  exit -1
fi

./signature_digest $1 $3 | openssl rsautl -sign -pkcs -inkey $2 
