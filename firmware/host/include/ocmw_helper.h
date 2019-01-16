/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCMW_HELPER_H_
#define _OCMW_HELPER_H_

/* stdlib includes */
#include <unistd.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

/* OC includes */
#include <ocmp_frame.h>
#include <ocmw_uart_comm.h>

#define Buf_PARAM_STRUCT_MAX_SIZE   16
#define MAX_PARM_COUNT          (OCMP_MSG_SIZE - (sizeof(OCMPMessage)\
                    - sizeof(void *) + sizeof (OCMPHeader)))

//#define DEBUG_SUBSYSTEM_NBR         1
//#define DEBUG_I2C                   2
//#define DEBUG_MDIO                  8

#define PARAM_STR_BUFF_SIZE         100
#define PARAM_TYPE_BUFF_SIZE        32

#define OCMW_MAX_SUBSYSTEM_SIZE     16
#define OCMW_MAX_ACTION_SIZE        16
#define OCMW_MAX_MSGTYPE_SIZE       16
#define OCMW_COMMAND_BUFF_SIZE      20
#define OCMW_POST_DESC_SIZE         24
#define OCMW_HELP_FRAME_SIZE        40
#define OCMW_POST_DEVICE_SIZE       40
#define EEPROM_STATUS_MAX_SIZE      21
#define EEPROM_SDR_STATUS_SIZE      19
#define EEPROM_CONFIG_MAX_SIZE      14
#define EEPROM_CONFIG_SIZE          18
#define OCMW_MAX_POST_CODE_SIZE     100
#define TEMP_STR_BUFF_SIZE          100


typedef enum {
    VOID    = 0,
    CHAR    = 1,
    UCHAR   = 1,
    SHORT   = 2,
    USHORT  = 2,
    INT     = 4,
    UINT    = 4,
    FLOAT   = 4
} DATA_TYPE;

typedef enum {
    FAILED = -1,
    SUCCESS,
    INVALID_POINTER,
} ocmw_db_ret;

typedef struct {
    int32_t paramindex;
    int32_t paramval;
} bufParam;

typedef struct {
    int8_t msgType;
    int8_t componentId;
    int8_t subsystem;
    int8_t actionType;
    int16_t paramInfo;
    int32_t numOfele;
    int8_t pbuf[MAX_PARM_COUNT];
    int32_t paramSizebuf[MAX_PARM_COUNT];
} ocmwSendRecvBuf;

typedef struct {
    int8_t subsystem;
    int8_t componentId;
    int8_t msgType;
    int8_t actionType;
    int16_t paramId;
    int8_t paramPos;
    int8_t paramSize;
    char commandType[OCMW_COMMAND_BUFF_SIZE];
    char cmdStr[MAX_PARM_COUNT];
} ocmwSchemaSendBuf;

typedef struct __attribute__((packed, aligned(1))){
    uint8_t devsn;                              /* device serial Number */
    uint8_t subsystem;
    char subsysName[OCMW_MAX_SUBSYSTEM_SIZE];   /* Subsystem Name */
    char deviceName[OCMW_POST_DEVICE_SIZE];     /* Device Name */
    uint8_t status;                         /* device status */
}ocwarePostResultData;

typedef struct {
    unsigned int count;                                         /* Device Status count */
    ocwarePostResultData results[OCMW_MAX_POST_CODE_SIZE];    /* Post result structure */
} ocwarePostResults;

typedef struct {
    uint8_t msgtype;                    /* Post Message tyep */
    uint8_t replycode;                  /* Reply type */
    char desc[OCMW_POST_DESC_SIZE];     /* Device description */
} ocwarePostReplyCode;

/*
 * @param sem an input value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
int32_t ocmw_sem_wait_nointr(sem_t *sem);
/*
 * @param sem an input value (by pointer)
 * @param timeout an input value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
int32_t ocmw_sem_timedwait_nointr(sem_t *sem, const struct timespec *timeout);
/*
 * @param paramindex an input value (by value)
 * @param paramSizebuf an input value (by pointer)
 * @param dataSize an output value (by pointer)
 * @param pos an output value (by pointer)
 *
 */
void ocmw_dataparsing_from_db(int32_t paramIndex, int32_t *paramSizebuf,
        int32_t *dataSize, int32_t *pos);
/*
 * @param input an input buffer (by pointer)
 * @param bufParamStruct an output buffer (by pointer)
 *
 */
void ocmw_dataparsing_from_ec(ocmwSendRecvBuf *input,
        bufParam * bufParamStruct);
/*
 * @param uartInputBuf an input buffer (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
int32_t ocmw_fill_inputstruct(ocmwSendRecvBuf *uartInputBuf);
/*
 * @param ecInputData an input data (by value)
 *
 * @return true if function succeeds, false otherwise
 */
int8_t ocmw_parse_eepromdata_from_ec (ocmwSendRecvBuf ecInputData);
/*
 * @param ecInputData an input data (by value)
 *
 * @return true if function succeeds, false otherwise
 */
int32_t ocmw_parse_obc_from_ec(ocmwSendRecvBuf ecInputData);
/*
 * @param ecInputData an input data (by value)
 *
 * @return true if function succeeds, false otherwise
 */
int32_t ocmw_parse_testingmodule_from_ec(ocmwSendRecvBuf ecInputData);
/*
 * @param msgaction an input value (by value)
 * @param msgtype an input value (by value)
 * @param paramstr an input string (by pointer)
 * @param paramvalue an input value (by pointer)
 *
 */
//int ocmw_msgproc_send_msg(int8_t msgaction, int8_t msgtype,
    //                    const int8_t* paramstr, void* paramvalue);

#endif /* _OCMW_HELPER_H_ */
