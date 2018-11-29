/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCMW_IPC_COMM_H_
#define _OCMW_IPC_COMM_H_

#include <ocmw_helper.h>

#define PARAMSTR_NUMBER_LEN 12
#define TESTMOD_MAX_LEN 16
#define RES_STR_BUFF_SIZE 100000
#define TEMP_STR_BUFF_SIZE 100
#define FINAL_STR_BUFF_SIZE 100
#define ALERT_STR_BUFF_SIZE 200
#define CMD_STR_BUFF_SIZE 100
#define OCMW_MAX_IMEI_SIZE 15
#define OCMW_MAX_MSG_SIZE 20

typedef struct {
    int8_t pin;
    int8_t value;
} debugGPIOData;

typedef struct {
    uint16_t regAddress;
    uint16_t regValue;
} debugMDIOData;

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t slaveAddress;
    uint8_t numOfBytes;
    uint8_t regAddress;
    uint16_t regValue;
} debugI2CData;

typedef enum {
    SET_STR,
    GET_STR,
    RESET_STR,
    ENABLE_STR,
    DISABLE_STR,
    ACTIVE_STR,
    ECHO_STR,
    DISCONNECT_STR,
    CONNECT_STR,
    SEND_STR,
    DIAL_STR,
    ANSWER_STR,
    HANGUP_STR,
    ELOOPBK_STR,
    DLOOPBK_STR,
    EPKTGEN_STR,
    DPKTGEN_STR,
    ALERTLOG_STR,
    MAX_STR
} ocmw_token_t;

typedef enum {
    HCI_STR,
    DEBUG_STR,
    RESULT_STR,
    ENABLE_SET_STR,
    GETSETMAX
} ocmw_setGet;

/*
 * Initialize the ocmw cli communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_init_occli_comm(void);

/*
 * Deinitialize the ocmw cli communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_deinit_occli_comm(void);
/*
 * @param cmd an input string (by pointer)
 * @param cmdlen an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_recv_clicmd_from_occli(char *cmdstr, int32_t cmdlen);
/*
 * @param resp an input value (by pointer)
 * @param resplen an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_send_clicmd_resp_to_occli(const char *resp,
                                              int32_t resplen);
/*
 * @param cmdstr an input value (by pointer)
 * @param response an output value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern int ocmw_clicmd_handler(const char *cmdstr, char *response);
/*
 * Initialize the ocmw alert communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t init_occli_alert_comm();
/*
 * Deinitialize the ocmw alert communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_deinit_occli_alert_comm(void);
/*
 * @param buf an input value (by pointer)
 * @param buflen an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_send_alert_to_occli(const char *buf, int32_t buflen);

extern char ocmw_retrieve_post_results_count(ocwarePostResults *psData);

extern char ocmw_retrieve_post_results(ocwarePostResults *psData);

extern char ocmw_retrieve_reply_code_desc(ocwarePostReplyCode *replyCode);
#endif /* _OCMW_IPC_COMM_H_ */
