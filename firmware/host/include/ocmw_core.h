/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCMW_CORE_H_
#define _OCMW_CORE_H_

/* OC includes */
#include <ocmw_helper.h>

sem_t semecMsgParser;
sem_t semCliReturn;
sem_t semCommandPost;

/* This timeout must be less than CLI timeout */
#define OCMW_SEM_WAIT_TIME          10
#define OCMW_BUSY_WAIT_INDEX        0
#define OCMW_TIMED_WAIT_INDEX       1
#define PARAM_STR_MAX_BUFF_SIZE     100

extern int32_t ocmw_init();
/*
 * @param actionType an input enum value (by value)
 * @param msgType an input enum value (by value)
 * @param paramStr an input string (by pointer)
 * @param interface an input enum value (by value)
 * @param paramVal an input value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_msg_packetize_and_send(char * argv[], uint8_t actionType,
        uint8_t msgType, const int8_t * paramStr, uint8_t interface,
        void* paramVal);
/*
 * Message parser module
 *
 */
extern void ocmw_ec_msgparser(void);
/*
 * Thread to parse the data coming from EC to AP through uart
 * @param pthreadData an input value (by pointer)
 */
extern void * ocmw_thread_uartmsgparser(void *pthreadData);

/*
 * Thread to parse the data coming from EC to AP through ethernet
 * @param pthreadData an input value (by pointer)
 */
extern void * ocmw_thread_ethmsgparser(void *pthreadData);
/*
 * @param ecMsgFrame an input structure (by value)
 * @param interface an input enum value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_send_msg(OCMPMessageFrame ecMsgFrame, uint8_t interface);
/*
 * @param semId an input value (by pointer)
 * @param semWaitType an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_sem_wait(sem_t *semId, int32_t semWaitType);
#endif /* _OCMW_CORE_H_ */
