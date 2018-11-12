/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "UartMon.h"

static void UartMon_close(UART_Handle handle)
{
    UartMon_Object *obj = handle->object;

    if (!obj->state.opened) {
        return;
    }

    UART_close(obj->hUart_in);
    UART_close(obj->hUart_debug);
    obj->state.opened = false;
}

static int UartMon_control(UART_Handle handle, unsigned int cmd, void *arg)
{
    UartMon_Object *obj = handle->object;
    return UART_control(obj->hUart_in, cmd, arg);
}

static void UartMon_init(UART_Handle handle)
{
    UartMon_Object *obj = handle->object;
    obj->state.opened = false;
}

static UART_Handle UartMon_open(UART_Handle handle, UART_Params *params)
{
    UartMon_Object *obj = handle->object;
    const UartMon_Cfg *cfg = handle->hwAttrs;
    if (obj->state.opened) {
        return NULL;
    }

    /* This is a driver limitation - we only support blocking mode for now */
    if (params->readDataMode != UART_MODE_BLOCKING ||
        params->writeDataMode != UART_MODE_BLOCKING) {
        return NULL;
    }

    obj->hUart_in = UART_open(cfg->uart_in_idx, params);
    if (!obj->hUart_in) {
        return NULL;
    }

    obj->hUart_debug = UART_open(cfg->uart_debug_idx, params);
    if (!obj->hUart_debug) {
        UART_close(obj->hUart_in);
        return NULL;
    }

    return handle;
}

static int UartMon_read(UART_Handle handle, void *buffer, size_t size)
{
    UartMon_Object *obj = handle->object;

    int bytes_read = UART_read(obj->hUart_in, buffer, size);
    if (bytes_read > 0) {
        UART_write(obj->hUart_debug, buffer, bytes_read);
    }
    return bytes_read;
}

static int UartMon_readPolling(UART_Handle handle, void *buffer, size_t size)
{
    UartMon_Object *obj = handle->object;

    int bytes_read = UART_readPolling(obj->hUart_in, buffer, size);
    if (bytes_read > 0) {
        /* We should forward with polling functionality too, since this function
         * may be called while interrupts are disabled
         */
        UART_writePolling(obj->hUart_debug, buffer, bytes_read);
    }
    return bytes_read;
}

static void UartMon_readCancel(UART_Handle handle)
{
    return;
}

static int UartMon_write(UART_Handle handle, const void *buffer, size_t size)
{
    UartMon_Object *obj = handle->object;

    int bytes_written = UART_write(obj->hUart_in, buffer, size);

    if (bytes_written > 0) {
        UART_write(obj->hUart_debug, buffer, bytes_written);
    }
    return bytes_written;
}

static int UartMon_writePolling(UART_Handle handle, const void *buffer,
                                size_t size)
{
    UartMon_Object *obj = handle->object;

    int bytes_written = UART_writePolling(obj->hUart_in, buffer, size);

    if (bytes_written > 0) {
        /* We should forward with polling functionality too, since this function
         * may be called while interrupts are disabled
         */
        UART_writePolling(obj->hUart_debug, buffer, bytes_written);
    }
    return bytes_written;
}

static void UartMon_writeCancel(UART_Handle handle)
{
    return;
}

const UART_FxnTable UartMon_fxnTable = {
    UartMon_close,      UartMon_control, UartMon_init,
    UartMon_open,       UartMon_read,    UartMon_readPolling,
    UartMon_readCancel, UartMon_write,   UartMon_writePolling,
    UartMon_writeCancel
};
