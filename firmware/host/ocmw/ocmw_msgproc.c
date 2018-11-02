/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include <logger.h>
#include "ocmw_core.h"
#include "ocmw_msgproc.h"
#include "ocmp_frame.h"

/**************************************************************************
 * Function Name    : ocmw_msgproc_send_msg
 * Description      : This Function configures the header
 * Input(s)         : strTokenArray, action, msgtype, paramstr
 * Output(s)        : paramvalue
 ***************************************************************************/
int ocmw_msgproc_send_msg(char * strTokenArray[], uint8_t action,
        int8_t msgtype, const int8_t* paramstr, void *paramvalue)
{
    int32_t ret = 0;
    uint8_t interface = 0;
#if defined(INTERFACE_STUB_EC) || defined(INTERFACE_ETHERNET)
    interface = OCMP_COMM_IFACE_ETHERNET;
#elif INTERFACE_USB
    interface = OCMP_COMM_IFACE_USB;
#else
    interface = OCMP_COMM_IFACE_UART;
#endif /* INTERFACE_ETHERNET */
    ret = ocmw_msg_packetize_and_send(&strTokenArray[0], action, msgtype,
            paramstr, interface, paramvalue);
    if (ret != 0) {
        logerr("ocmw_msg_packetize_and_send failed \n");
    }
    return ret;
}

