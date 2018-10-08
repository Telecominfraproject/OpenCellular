/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/**
 * This module allows us to expose a virtual UART port to allow us to forward
 * UART traffic to another UART for traffic monitoring and debugging
 *
 * It is currently very basic - it assumes debug UART & watched UART support
 * the exact same settings. Additionally, it only supports blocking read/write
 * and will only show incoming data that has been read by the subscriber (i.e.
 * unless the task has called UART_read, no data is pushed to the debug port)
 *
 * To use this driver, simply add a new entry to the UART_Config array:
 *
 * 1. Add a new entry to the UARTName enum
 *
 * 2. Create a new cfg/object instance & provide it the port # of the UART to
 *    monitor and the UART to forward to:
 *      UartMon_Object uart_mon_obj;
 *      const UartMon_Cfg uart_mon_cfg = {
 *          .uart_in_idx = OC_CONNECT1_UARTXR0,
 *          .uart_debug_idx = OC_CONNECT1_UART0,
 *      };
 *
 * 3. Add new entry to UART_Config array:
 *      [OC_CONNECT1_UARTMON] = {
 *          .fxnTablePtr = &UartMon_fxnTable,
 *          .object = &uart_mon_obj,
 *          .hwAttrs = &uart_mon_cfg,
 *      },
 *
 * 4. Open this new UART port and use it as usual (note: you can switch back to
 *    the regular UART at any time - this driver will only have an effect once
 *    it is open):
 *      UART_open(OC_CONNECT1_UARTMON, &uartParams);
 *
 */

#ifndef _UARTMON_H_
#define _UARTMON_H_

#include <ti/drivers/UART.h>

#include <stdbool.h>

/* UART function table pointer */
extern const UART_FxnTable UartMon_fxnTable;

typedef struct UartMon_Cfg {
    unsigned int uart_in_idx; /*!< The UART we're going to monitor */
    unsigned int uart_debug_idx; /*!< The UART we're going to forward to */
} UartMon_Cfg;

/* Private data for the driver instance */
typedef struct UartMon_Object {
    struct {
        bool opened : 1; /*!< Is there an open handle to the driver */
    } state;

    UART_Handle hUart_in; /*!< Handle to the monitored UART */
    UART_Handle hUart_debug; /*!< Handle to the forwarding UART */
} UartMon_Object;

#endif /* _UARTMON_H_ */
