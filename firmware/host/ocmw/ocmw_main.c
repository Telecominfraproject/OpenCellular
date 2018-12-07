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

/* OC includes */
#include <logger.h>
#include <ocmw_uart_comm.h>
#include <ocmw_core.h>
#include <ocmw_occli_comm.h>
#include <ocmw_eth_comm.h>

uint8_t mcuMsgBuf[OCMP_MSG_SIZE];

/**************************************************************************
 * Function Name    : main
 * Description      :
 * Input(s)         : argc, argv
 * Output(s)        :
 ***************************************************************************/
int32_t main(int32_t argc, char **argv)
{
    char cmdstr[CMD_STR_BUFF_SIZE] = { 0 };
    char response[RES_STR_BUFF_SIZE] = { 0 };
    int32_t ret = 0;

    /* Initialize logging */
    initlog("ocmw");

    ret = ocmw_init_occli_comm();
    if (ret != 0) {
        logerr("init_occli_comm() failed.");
        return FAILED;
    }

    /* Initialize UART for EC communication */
    ret = ocmw_init_ec_comm(&ocmw_ec_uart_msg_hndlr);
    if (ret != 0) {
        logerr("ocmw_init_ec_comm() failed.");
        return FAILED;
    }

#ifdef INTERFACE_ETHERNET
    ret = ocmw_init_eth_comm(OCMW_EC_DEV);
    if (ret != 0) {
        printf("ocmw_init_eth_comm() failed \n");
        return FAILED;
    }
#endif /* INTERFACE_ETHERNET */

#ifdef INTERFACE_STUB_EC
    ret = ocmw_init_eth_comm(OCMW_EC_STUB_DEV);
    if (ret != 0) {
        printf("init_eth_comm() failed \n");
        return FAILED;
    }
#endif /* INTERFACE_ETHERNET */

    ret = ocmw_init();
    if (ret != 0) {
        logerr("ocmw_init() failed.");
        return FAILED;
    }

    while (1) {
        memset(cmdstr, 0, sizeof(cmdstr));
        memset(response, 0, sizeof(response));

        ret = ocmw_recv_clicmd_from_occli(cmdstr, sizeof(cmdstr));
        if (ret == FAILED)
            continue; /* retry on error */

        ocmw_clicmd_handler(cmdstr, response);
        ocmw_send_clicmd_resp_to_occli(response, strlen(response));
    }

    ocmw_deinit_occli_comm();
    ocmw_deinit_occli_alert_comm();
    ocmw_deinit_ec_comm();
    deinitlog();
    return SUCCESS;
}
