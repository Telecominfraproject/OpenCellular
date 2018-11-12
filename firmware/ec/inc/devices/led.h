/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef INA226_H_
#define INA226_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/Framework.h"
#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"
#include "inc/devices/sx1509.h"
#include "inc/subsystem/hci/hci_led.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define LED_OFF 0xFF

/* ClkX = fOSC/(2^(RegMisc[6:4]-1); 0x50-125kHz, 0x40-250KHz, 0x30-500KHz,
 * 0x20-1MHz, 0x10-2MHz; Fading - Linear */
#define REG_MISC_VALUE 0x24

/* 4:0 => ON Time of IO[X]; If 0 : TOnX = Infinite;
 * 1 - 15 : TOnX = 64 * RegTOnX * (255/ClkX);
 * 16 - 31 : TOnX = 512 * RegTOnX * (255/ClkX) */
#define REG_T_ON_VALUE 0x10

/* 7:3 - OFF Time of IO[X]; If 0 : TOffX = Infinite;
 * 1 - 15 : TOffX = 64 * RegOffX[7:3] * (255/ClkX);
 * 16 - 31 : TOffX = 512 * RegOffX[ 7:3] * (255/ClkX) */
/* 2:0 - OFF Intensity of IO[X] = >Linear mode : IOffX = 4 x RegOff[2:0] */
#define REG_OFF_VALUE 0x80

#define HCI_LED_TOTAL_NOS 14

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* LED Test Params */
typedef enum { HCI_LED_OFF = 0, HCI_LED_RED, HCI_LED_GREEN } ledTestParam;

typedef enum {
    HCI_LED_1 = 0,
    HCI_LED_2,
    HCI_LED_3,
    HCI_LED_4,
    HCI_LED_5,
    HCI_LED_6,
    HCI_LED_7,
    HCI_LED_8,
    HCI_LED_9,
    HCI_LED_10,
    HCI_LED_11,
    HCI_LED_12,
    HCI_LED_13,
    HCI_LED_14,
    HCI_LED_MAX
} hciLedNo;

typedef struct {
    hciLedNo ledNumber;
    HciLed_DriverId ioexpDev;
    sx1509RegType ledReg;
    uint16_t ledGreen;
    uint16_t ledRed;
    uint16_t ledOff;
} hciLedData;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus hci_led_turnon_green(const HciLedCfg *driver);
ReturnStatus hci_led_turnon_red(const HciLedCfg *driver);
ReturnStatus hci_led_turnoff_all(const HciLedCfg *driver);
ReturnStatus hci_led_system_boot(const HciLedCfg *driver);
ReturnStatus led_init(const HciLedCfg *driver);
ePostCode led_probe(const HciLedCfg *driver, POSTData *postData);

#endif /* INA226_H_ */
