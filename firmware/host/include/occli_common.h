/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCCLI_COMM_H_
#define _OCCLI_COMM_H_

/* OC Inlucdes */
#include <stdbool.h>
#include <ocmw_helper.h>
#include <Framework.h>

#define HIT_FILE_BUFF_SIZE 50
#define OCCLI_STRING_SIZE 128
#define RES_STR_BUFF_SIZE 10000
#define OCMP_MAX_SIZE 10
#define OCCLI_CHAR_ARRAY_SIZE 30
#define OCMW_MAX_SUBSYSTEM 11
/* This timeout must be more than OCMW timeout */
#define OCCLI_TIMEOUT_PERIOD 12
#define FAILED -1
#define SUCCESS 0
#define OCCLI_SNPRINTF_MAX_LEN 200
#define OCCLI_HELP_MAX_SIZE 400

typedef struct {
    char option;
    int sizeNum;
    int totalStr;
} OCCLI_ARRAY_PARAM;

typedef struct {
    char subsystem[OCCLI_CHAR_ARRAY_SIZE];
    char component[OCCLI_CHAR_ARRAY_SIZE];
    char subcomponent[OCCLI_CHAR_ARRAY_SIZE];
    char msgtype[OCCLI_CHAR_ARRAY_SIZE];
    char parameter[OCCLI_CHAR_ARRAY_SIZE];
} strMsgFrame;

typedef struct {
    int8_t subsystem;
    int8_t component;
    int8_t msgtype;
    int16_t parameter;
} sMsgParam;

typedef struct {
    char *subsystem;
    char *component;
    char *msgtype;
    char *subcomponent;
    char *parameter;
} OCCLI_STRING_MSG;

typedef struct {
    int32_t totalNum;
    struct name {
        int32_t number;
        char name[OCCLI_CHAR_ARRAY_SIZE];
    } Info[OCCLI_CHAR_ARRAY_SIZE];
} subSystemInfo;

// Help Menu structure
typedef struct {
    char subSystem[OCMW_MAX_SUBSYSTEM_SIZE];
    char component[OCMW_HELP_FRAME_SIZE];
    char subComponent[OCMW_HELP_FRAME_SIZE];
    char msgType[OCMW_MAX_MSGTYPE_SIZE];
    char actionType[OCMW_MAX_ACTION_SIZE];
    char arguments[OCMW_HELP_FRAME_SIZE];
    char parameter[OCMW_HELP_FRAME_SIZE];
} helpMenu;

typedef struct {
    char subsystem[OCCLI_CHAR_ARRAY_SIZE];
    char component[OCCLI_CHAR_ARRAY_SIZE];
    char subComponent[OCCLI_CHAR_ARRAY_SIZE];
} commandFrame;

/*
 * Initialize the ocmw communication
 */
extern int32_t occli_init_comm(void);
/*
 * Deinitialize the ocmw communication
 */
extern int8_t occli_deinit_comm(void);
/*
 * @param cmd an input string (by pointer)
 * @param cmdlen an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t occli_send_cmd_to_ocmw(const char *cmd, int32_t cmdlen);
/*
 * @param resp an output value (by pointer)
 * @param resplen an output value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t occli_recv_cmd_resp_from_ocmw(char *resp, int32_t resplen);
/*
 * @param resp an output value (by pointer)
 * @param resplen an output value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t occli_recv_alertmsg_from_ocmw(char *resp, int32_t resplen);
/*
 * @param  root an input value (by pointer)
 * @param  msgFrame an input value (by pointer)
 * @param  ecSendBuf an input value (by pointer)
 * @param  actiontype an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_parse_msgframe(const Component *root, strMsgFrame *msgFrame,
                                   uint8_t actiontype,
                                   ocmwSchemaSendBuf *ecSendBuf);
/*
 * @param root an output value (by pointer)
 * @param dMsgFrameParam an output value (by pointer)
 * @param ecReceivedMsg an output value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern void ocmw_deserialization_msgframe(const Component *root,
                                          sMsgParam *dMsgFrameParam,
                                          OCMPMessageFrame *ecReceivedMsg);
/*
 * @param compBase an output value (by pointer)
 * @param msgFrame an output value (by pointer)
 * @param ecSendBuf an output value (by pointer)
 * @param actiontype an output value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_parse_command_msgframe(const Component *compBase,
                                           strMsgFrame *msgFrame,
                                           uint8_t actiontype,
                                           ocmwSchemaSendBuf *ecSendBuf,
                                           char *strTokenArray[]);
/*
 * @param compBase an output value (by pointer)
 * @param msgFrame an output value (by pointer)
 * @param ecSendBuf an output value (by pointer)
 * @param actiontype an output value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_parse_post_msgframe(const Component *compBase,
                                        strMsgFrame *msgFrame,
                                        uint8_t actiontype,
                                        ocmwSchemaSendBuf *ecSendBuf);
/*
 * @param root an output value (by pointer)
 * @param systemInfo an output value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_frame_subsystem_from_schema(const Component *root,
                                                subSystemInfo *systemInfo);

/*
 * @param root an output value (by pointer)
 * @param systemInfo an output value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_frame_postTable_from_schema(const Component *root);

/*
 * @param pointer to global memory
 *
 * @return  NONE
 */
extern void ocmw_free_global_pointer(void **ptr);
/*
 * @param root an output value (by pointer)
 * @param systemInfo an output value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern int8_t occli_printHelpMenu(const Component *root, char *cmd);

/*Display CLI window*/
extern void occli_print_opencelluar();

#endif /* _OCCLI_COMM_H_ */
