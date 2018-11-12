/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef TEST_MODULE_H_
#define TEST_MODULE_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/Framework.h"
#include "drivers/OcGpio.h"
#include "helpers/attribute.h"

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* G510 config */
/* TODO: should move to a separate file */
typedef struct G510_Cfg {
    unsigned int uart;
    OcGpio_Pin pin_enable;
    OcGpio_Pin pin_pwr_en;
    OcGpio_Pin pin_sim_present;
} G510_Cfg;

/* Subsystem config */
typedef struct TestMod_Cfg {
    G510_Cfg g510_cfg;
    OcGpio_Pin pin_ant_sw;
} TestMod_Cfg;

/*
 * Test Module Components. This is the part of the OCMPMsg in componentID field.
 */

/* TODO: this should be in gsm.h */
typedef enum OperatorStat {
    OP_STAT_UNKNOWN = 0x00,
    OP_STAT_AVAILABLE = 0x01,
    OP_STAT_CURRENT = 0x02,
    OP_STAT_FORBIDDEN = 0x03,
} OperatorStat;

typedef struct TestModule_opInfo {
    uint16_t opCode;
    OperatorStat stat;
} TestModule_opInfo;

typedef enum TestModule_errorSource {
    TESTMOD_ERR_INTERNAL = 0,
    TESTMOD_ERR_CMS = 1,
    TESTMOD_ERR_CME = 2
} TestModule_errorSource;

typedef struct TestModule_lastError {
    TestModule_errorSource src;
    uint16_t code;
} TestModule_lastError;

typedef union PACKED {
    uint64_t imei;
    uint64_t imsi;
    char mfg[10];
    char model[5];
    uint8_t rssi;
    uint8_t ber;
    uint8_t regStat;
    TestModule_opInfo opInfo;
    uint32_t cid;
    uint8_t bsic;
    TestModule_lastError lastErr;
} TestMod_2G_Status_Data;

typedef struct PACKED {
    char number[16];
    char msg[20];
} TestModule_sms;

extern const Driver Testmod_G510;

/* Command callbacks */
bool TestMod_cmdEnable(void *driver, void *params);
bool TestMod_cmdDisable(void *driver, void *params);
bool TestMod_cmdDisconnect(void *driver, void *params);
bool TestMod_cmdConnect(void *driver, void *params);
bool TestMod_cmdSendSms(void *driver, void *params);
bool TestMod_cmdDial(void *driver, void *params);
bool TestMod_cmdAnswer(void *driver, void *params);
bool TestMod_cmdHangup(void *driver, void *params);
bool TestMod_cmdReset(void *driver, void *params);

#endif /* TEST_MODULE_H_ */
