/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef bsp_msgHeader

#define bsp_msgHeader 
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

typedef void (*OCPCallbackHandlers)(uint8_t* async_msg);

void ocp_msg_handler_init(OCPCallbackHandlers cb_ocp);
uint8_t* ocp_sync_msg_hanlder(uint8_t* reqMsg, int32_t msgLen);

#ifdef __cplusplus
}
#endif

#endif
