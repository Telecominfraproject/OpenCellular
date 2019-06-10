/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "ocps_bsp.h"
#include "ocps_uart.h"
#include "util.h"

int32_t uartfd[2];
int32_t slipfd[2];

AppCallbackHandlers cb_app_msg_handler;
BspCallbackHandlers cb_bsp_msg_handler;

/**************************************************************************
 * Function Name    : ocps_bsp_comm_init
 * Description      :
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int ocps_bsp_comm_init()
{
    int32_t rc = 0;
    /*UART to Slip layer communication.*/
    rc = pipe(slipfd);
    if (rc < 0){
        perror("UART pipe creation failed.\n");
        exit(1);
    }
    /*Slip layer to UART communication.*/
    rc = pipe(uartfd);
    if (rc < 0){
        perror("SLIP pipe creation failed.\n");
        exit(1);
    }
}

/**************************************************************************
 * Function Name    : ocps_bsp_comm_deinit
 * Description      :
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int ocps_bsp_comm_deinit()
{
    int32_t rc = 0;
    /*UART to Slip layer communication.*/
    rc = close(slipfd[0]);
    if (rc < 0){
        perror("UART pipe closing failed.\n");
        exit(1);
    }
    rc = close(slipfd[1]);
    if (rc < 0){
        perror("UART pipe closing failed.\n");
        exit(1);
    }

    /*Slip layer to UART communication.*/
    rc = close(uartfd[0]);
    if (rc < 0){
        perror("SLIP pipe closing failed.\n");
        exit(1);
    }
    rc = close(uartfd[1]);
    if (rc < 0){
        perror("SLIP pipe closing failed.\n");
        exit(1);
    }
}

/**************************************************************************
 * Function Name    : ocps_bsp_init
 * Description      :
 * Input(s)         : AppCallbackHandlers
 * Output(s)        :
 ***************************************************************************/
int32_t ocps_bsp_init(AppCallbackHandlers cb_app)
{

    int32_t rc = 0;
    /* Initialize Application call back.*/
    cb_app_msg_handler = cb_app;

    /* Initialize Application call back.*/
    cb_bsp_msg_handler = &ocps_bsp_cb_handler;

    /* Initialize BSP for OC-Power server*/
    rc = ocps_bsp_comm_init();
    if (rc != 0) {
        logerr("ocp_server_bsp_init() failed.");
        return FAILED;
    }

    /* Initialize SLIP module*/
    rc = ocps_slip_init(cb_bsp_msg_handler);
    if (rc != 0) {
        logerr("ocps_slip_init() failed.");
        return FAILED;
    }

    /* Initialize UART for Yapper MCU */
    rc = ocps_init_yapper_comm(&ocps_yapper_uart_msg_hndlr);
    if (rc != 0) {
        logerr("ocp_init_yapper_comm() failed.");
        return FAILED;
    }
}

/**************************************************************************
 * Function Name    : ocps_bsp_deinit
 * Description      :
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocps_bsp_deinit()
{
    ocps_bsp_comm_deinit();
    ocps_deinit_yapper_comm();
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocps_bsp_request_handler
 * Description      :
 * Input(s)         : request message, lenth of the request message.
 * Output(s)        :
 ***************************************************************************/
void ocps_bsp_request_handler(uint8_t* requestMsg, int32_t msgLength)
{
   logger("BSP::Received request message from OC-Power server app.\n");
   ocps_slip_send_packet(requestMsg, msgLength);
}

/**************************************************************************
 * Function Name    : ocps_bsp_cb_handler
 * Description      :
 * Input(s)         : slip decoded message.
 * Output(s)        :
 ***************************************************************************/
void ocps_bsp_cb_handler(uint8_t* slipMsg)
{
  /* call application layer callback*/
  logger("BSP::Sending a message to OC-Power server app.\n");
  (*cb_app_msg_handler)(slipMsg);
}
