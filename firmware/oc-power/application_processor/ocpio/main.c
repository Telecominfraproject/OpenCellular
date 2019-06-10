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

#include "bspMsgHandler.h"

void ocp_async_msg_receiver(uint8_t* msg)
{
/*copy data*/

}


uint8_t* sendConfigReq() 
{
 uint8_t* req = malloc(sizeof(char)*6);
 if (req) {
    req[0] = 0x01;
    req[1] = 0x01;
    req[2] = 0x01;
    req[3] = 0x01;
    req[4] = 0;
    req[5] = 0;
 }
 return req;
}

uint8_t* sendTelemetryReq() 
{
 uint8_t* req = malloc(sizeof(char)*13);
 if (req) {
    req[0] = 0x03;
    req[1] = 0x01;
    req[2] = 0x08;
    req[3] = 0x01;
    req[4] = 0;
    req[5] = 0;
    req[6] = 0;
    req[7] = 0;
    req[8] = 0;
    req[9] = 0;
    req[10] = 0;
    req[11] = 0;
    req[12] = 2;
 }
 return req;
}


/**************************************************************************
 * Function Name    : main
 * Description      :
 * Input(s)         : main
 * Output(s)        :
 ***************************************************************************/
int32_t main()
{

  ocp_msg_handler_init(&ocp_async_msg_receiver);

  printf("Starting Request tx.\n");	
  uint8_t* reqMsg;
  uint8_t* respMsg;
  uint8_t i = 0;
  while(true) {
    if(i%2) {
  	    reqMsg = sendConfigReq();
        respMsg = (uint8_t*)ocp_sync_msg_hanlder(reqMsg,6);
        if(respMsg) {
	          printf("Response is %s\n",respMsg);
              free(respMsg);
              free(reqMsg);
        }
    } else {
  	    reqMsg = sendTelemetryReq();
        respMsg = (uint8_t*)ocp_sync_msg_hanlder(reqMsg,13);
        if(respMsg) {
	          printf("Response is %s\n",respMsg);
              free(respMsg);
              free(reqMsg);
        }
    }
    i++; 
    printf("Running %d times.\n",i);
   usleep(10); 
  }
  return 0;
}
