/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>

typedef void (*AppCallbackHandlers)(uint8_t* msg);
typedef void (*BspCallbackHandlers)(uint8_t* msg);

int ocps_bsp_comm_init();
int ocps_bsp_comm_deinit();
int32_t ocps_bsp_init(AppCallbackHandlers cb_app);
int32_t ocps_bsp_deinit();
void ocps_bsp_request_handler(uint8_t* requestMsg, int32_t msgLength);
void ocps_bsp_cb_handler(uint8_t* slipMsg);
