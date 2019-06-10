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

typedef void
        (*handle_msg_from_yapper_t)(const unsigned char* msgstr, int32_t msgsize);

void ocps_yapper_uart_msg_hndlr(const unsigned char* msgstr, int32_t msgsize);
int32_t ocps_init_yapper_comm(handle_msg_from_yapper_t msghndlr);
int32_t ocps_deinit_yapper_comm(void);
int32_t ocps_send_uart_msg_to_yapper(const uint8_t* msgstr, int32_t msgsize);
