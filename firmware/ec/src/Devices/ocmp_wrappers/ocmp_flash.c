/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*
*/

#include "common/inc/global/Framework.h"
#include "devices/i2c/threaded_int.h"
#include "helpers/memory.h"
#include "inc/devices/AT45DB.h"
#include "inc/common/bigbrother.h"
#include "inc/common/byteorder.h"
#include "inc/common/global_header.h"
#include "inc/utils/ocmp_util.h"

#define FRAME_SIZE	 		64
#define LAST_MSG_FLAG		0
#define PAYLOAD_SIZE 		47
#define NEXT_MSG_FLAG		1
#define NEXT_MSG_FLAG_POS	17

extern void fileRead(const char *path, UChar *buf, uint32_t size);
uint32_t fileSize(const char *path);

static bool _read_flash(void *driver, void *buf)
{
	char fileName[] = "logs";
	uint32_t numOfMsg = 0;
	uint32_t file_size = 0;
	uint8_t *logFile;

    OCMPMessageFrame *tMsg = (OCMPMessageFrame *) OCMP_mallocFrame(PAYLOAD_SIZE);
	file_size = fileSize(fileName);
	logFile = (uint8_t*)malloc(file_size);
	numOfMsg = file_size/FRAME_SIZE;
	fileRead(fileName,logFile, file_size);

    while(numOfMsg)
    {
    	logFile[NEXT_MSG_FLAG_POS] = NEXT_MSG_FLAG;
    	if(numOfMsg == 1){
    		logFile[NEXT_MSG_FLAG_POS] = LAST_MSG_FLAG;
    	}
    	memcpy(tMsg, logFile, FRAME_SIZE);
        Util_enqueueMsg(bigBrotherTxMsgQueue, semBigBrotherMsg,
                        (uint8_t*)tMsg);
        logFile +=FRAME_SIZE;
    	numOfMsg--;
    	free(tMsg);
    }
    free(logFile);
    return true;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    return POST_DEV_CFG_DONE;
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    return at45db_probe(driver,postData);
}

const Driver_fxnTable AT45DB641E_fxnTable = {
    /* Message handlers */
		.cb_probe = _probe,
		.cb_init = _init,
		.cb_flash_rw = _read_flash,
};
