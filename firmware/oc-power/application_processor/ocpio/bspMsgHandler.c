/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

#include "bspMsgHandler.h"
#include "ocps_bsp.h"
#include "ocp_serviceHeader.h"
#include "util.h"

//uint8_t rcvBuffer[2048];
static uint8_t* rcvBuffer;

static bool EXPECTING_SYNC_RESPONSE = false;
static bool RESPONSE_RECEIVED = false;
static bool ASYNC_MSG_RECEIVED = false;
#ifndef HOST_COMP
sem_t sem_sync;
sem_t sem_async;
#endif
pthread_t async_msg_thread;
OCPCallbackHandlers cb_ocp_server;

#define IF_SYNC_MSG(MSG)  (( (MSG[OCP_MSG_SRVC_INDEX])& SERVICE_ALERT )? false : true)
#define IF_ASYNC_MSG(MSG)  (( (MSG[OCP_MSG_SRVC_INDEX])& SERVICE_ALERT )? true : false)
#define MSG_LENGTH(MSG)    ((MSG[OCP_MSG_FSIZE_INDEX])*(MSG[OCP_MSG_FILDS_INDEX]))


bool valid_rx_msg(uint8_t* rxMsg)
{
    bool rc = true;
    /* Check for the service code */
    if((rxMsg[OCP_MSG_SRVC_INDEX] < SERVICE_TEST) && (rxMsg[OCP_MSG_SRVC_INDEX] > SERVICE_MAX)) {
            rc = false;
    }
    if(rxMsg[OCP_MSG_FILDS_INDEX]*rxMsg[OCP_MSG_FSIZE_INDEX] > MAX_LENGTH) {
        rc = false;
    } 
    return rc;    
}
/**************************************************************************
 * Function Name    : ocp_sync_msg_hanlder
 * Description      :
 * Input(s)         : Meassage to send a request to yapper.
 * Output(s)        :
 ***************************************************************************/
uint8_t* ocp_sync_msg_hanlder(uint8_t* reqMsg, int32_t msgLen)
{
    logger("MSGHANDLER::Sync::Sending a request message to OC-Power with %d.\n",msgLen);
    hexdisp(reqMsg, msgLen);
    ocps_bsp_request_handler(reqMsg, msgLen);
    logger("MSGHANDLER::Synch::Waiting for response..!\n");
    EXPECTING_SYNC_RESPONSE = true;
    // Wait for response. Timed wait is required.
#ifndef HOST_COMP
    sem_wait(&sem_sync);
#endif
    uint16_t msgl = MSG_LENGTH(rcvBuffer)+OCP_HEADER_SIZE;
    logger("MSGHANDLER::Sync::Received response message with length %d.\n",msgl);
    uint8_t* ocp_resp =(uint8_t*) malloc(msgl);
    if(ocp_resp) {
        memcpy(ocp_resp,rcvBuffer,msgl);
        hexdisp(ocp_resp,msgl);
    }
    /* Free the rx buffer*/
    if(rcvBuffer) {
        free(rcvBuffer);
        rcvBuffer = NULL;
    }
    RESPONSE_RECEIVED = false;
    return ocp_resp;
}

/**************************************************************************
 * Function Name    : ocp_async_msg_handler
 * Description      :
 * Input(s)         : Meassage to send a request to yapper.
 * Output(s)        :
 ***************************************************************************/
void *ocp_async_msg_handler()
{
    logger("MSGHANDLER::Starting async response collection thread.\n");
    while(true) {
        logger("MSGHANDLER::Async thread waiting state.\n");
#ifndef HOST_COMP
        sem_wait(&sem_async);
#endif
        logger("MSGHANDLER::Async thread recieved message.\n");
        uint8_t* async_msg = (uint8_t*) malloc(MSG_LENGTH(rcvBuffer)+OCP_HEADER_SIZE);
        if(async_msg) {
            memcpy(async_msg,rcvBuffer,MSG_LENGTH(rcvBuffer)+OCP_HEADER_SIZE);
            hexdisp(async_msg, (MSG_LENGTH(rcvBuffer)+OCP_HEADER_SIZE));
        }
        /* Free the rx buffer */
        if(rcvBuffer) {
            free(rcvBuffer);
            rcvBuffer = NULL;
        }
        /*Call another callback to server to send alerts.*/
        cb_ocp_server(rcvBuffer);
    }
    return;
}

/**************************************************************************
 * Function Name    : rx_msg_handler
 * Description      :
 * Input(s)         : AppCallbackHandler for the bsp.
 * Output(s)        :
 ***************************************************************************/
void rx_msg_handler(uint8_t *msg)
{
    logger("MSGHANDLER::Rx_MSG_HANDLER for the OC-Power messages.\n");
    if(valid_rx_msg(msg)) {
        if (IF_SYNC_MSG(msg)) {
            logger("MSGHANDLER::Received a sync message with size %d.\n",MSG_LENGTH(msg)+OCP_HEADER_SIZE);
            rcvBuffer = (uint8_t*) malloc(MSG_LENGTH(msg)+OCP_HEADER_SIZE);
            if(rcvBuffer) {
                memcpy(rcvBuffer,msg,MSG_LENGTH(msg)+OCP_HEADER_SIZE);
                if (EXPECTING_SYNC_RESPONSE) {
                   logger("MSGHANDLER::Released a sync semaphore.\n");
#ifndef HOST_COMP
                   sem_post(&sem_sync);
#endif
                }
                EXPECTING_SYNC_RESPONSE = false;
            }
        } else if (IF_ASYNC_MSG(msg)) {
            logger("MSGHANDLER::Received async response with size %d.\n",MSG_LENGTH(msg)+OCP_HEADER_SIZE);
            rcvBuffer = (uint8_t*) malloc(MSG_LENGTH(msg)+OCP_HEADER_SIZE);
            if(rcvBuffer) {
                memcpy(rcvBuffer,msg,MSG_LENGTH(msg)+OCP_HEADER_SIZE);
                logger("MSGHANDLER::Released a sync semaphore.\n");
#ifndef HOST_COMP
                sem_post(&sem_async);
#endif
            }
        }
    } 
}

/**************************************************************************
 * Function Name    : ocp_msg_handler_init
 * Description      : call bsp init and creates semaphores for synchronous and
                      asynchronous messages.
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
void ocp_msg_handler_init(OCPCallbackHandlers cb_ocp)
{
    /* Create semaphore for sync and async events*/
#ifndef HOST_COMP
    sem_init(&sem_sync, 0, 0);
    sem_init(&sem_async, 0, 0);
#endif
    /*Initialize the call back.*/ 
    cb_ocp_server = cb_ocp;
	
    /* Create a thread for async message handler*/
    pthread_create(&async_msg_thread,NULL, ocp_async_msg_handler,NULL);

    /*Initialize bsp lib with call back function.*/
    ocps_bsp_init(&rx_msg_handler);
    
    logger("MSGHANDLER::BSP initialization complete.\n");
}
