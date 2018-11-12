/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "uart.h"

#include "helpers/math.h"

#include <ti/sysbios/knl/Task.h>

void UART_flush(UART_Handle handle)
{
    int rxCount;
    while ((UART_control(handle, UART_CMD_GETRXCOUNT, &rxCount) ==
            UART_STATUS_SUCCESS) &&
           rxCount) {
        char buf[20];
        UART_read(handle, buf, MIN(rxCount, sizeof(buf)));
        Task_sleep(1); // Small delay so more data can come in
    }
}
