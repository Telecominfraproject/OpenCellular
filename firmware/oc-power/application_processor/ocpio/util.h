/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

#define ECTTY "/dev/ttyUSB0"
#define MAX_LENGTH 5120 

#define logerr printf
#define logger printf

#define SUCCESS 0
#define FAILED  -1

#define int32_t int
#define uint8_t unsigned char
