/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "util.h"

typedef int32_t (*SlipTxChar)(char ch);
typedef void (*CallbackHandler)(uint8_t* msg);
int ocps_slip_init(CallbackHandler cb_bsp_handler);
void ocps_slip_send_packet(uint8_t* buffer, int32_t packetLength);
