/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef LTC4274_H_
#define LTC4274_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"

#include <ti/sysbios/gates/GateMutex.h>

/* PSE Configuration */
#define LTC4274_INTERRUPT_MASK 0x00
#define LTC4274_OPERATING_MODE_SET 0x03
#define LTC4274_DETCET_CLASS_ENABLE 0x11
#define LTC4274_MISC_CONF 0xD1

/* PSE operating modes */
#define LTC4274_SHUTDOWN_MODE 0x00
#define LTC4274_MANUAL_MODE 0x01
#define LTC4274_SEMIAUTO_MODE 0x02
#define LTC4274_AUTO_MODE 0x03

#define LTC4274_INTERRUPT_ENABLE 0x80
#define LTC4274_DETECT_ENABLE 0x40
#define LTC4274_FAST_IV 0x20
#define LTC4274_MSD_MASK 0x01

#define LTC4274_HP_ENABLE 0x11

/* POE Device Info */
#define LTC4274_DEV_ID 0x0C
#define LTC4274_ADDRESS 0x2F
#define LTC4274_LTEPOE_90W 0x0E

#define LTC4274_DEVID(x) (x >> 3)
#define LTC4274_PWRGD(x) ((x & 0x10) >> 4)
#define LTC4374_CLASS(x) \
    ((x & 0xF0) >> 4) /*if MSB is set it specifies LTEPOE++ device*/
#define LTC4374_DETECT(x) ((x & 0x07))
#define LTC4274_DETECTION_COMPLETE(x) (x & 0x01)
#define LTC4274_CLASSIFICATION_COMPLETE(x) (x & 0x10)

typedef enum LTC4274_Event {
    LTC4274_EVT_SUPPLY = 1 << 7,
    LTC4274_EVT_TSTART = 1 << 6,
    LTC4274_EVT_TCUT = 1 << 5,
    LTC4274_EVT_CLASS = 1 << 4,
    LTC4274_EVT_DETECTION = 1 << 3,
    LTC4274_EVT_DISCONNECT = 1 << 2,
    LTC4274_EVT_POWERGOOD = 1 << 1,
    LTC4274_EVT_POWER_ENABLE = 1 << 0,
    LTC4274_EVT_NONE = 0,
} LTC4274_Event; // From LTC4274 Datasheet, Interrupts table

typedef enum { LTC4274_POWERGOOD = 0, LTC4274_POWERGOOD_NOTOK } ePSEPowerState;

typedef enum {
    LTC4274_DETECT_UNKOWN = 0,
    LTC4274_SHORT_CIRCUIT,
    LTC4274_CPD_HIGH,
    LTC4274_RSIG_LOW,
    LTC4274_SIGNATURE_GOOD,
    LTC4274_RSIG_TOO_HIGH,
    LTC4274_OPEN_CIRCUIT,
    LTC4274_DETECT_ERROR
} ePSEDetection;

typedef enum {
    LTC4274_CLASSTYPE_UNKOWN = 0x01,
    LTC4274_CLASSTYPE_1,
    LTC4274_CLASSTYPE_2,
    LTC4274_CLASSTYPE_3,
    LTC4274_CLASSTYPE_4,
    LTC4274_CLASSTYPE_RESERVED,
    LTC4274_CLASSTYPE_0,
    LTC4274_OVERCURRENT,
    LTC4274_LTEPOE_TYPE_52_7W = 0x09,
    LTC4274_LTEPOE_TYPE_70W = 0x0a,
    LTC4274_LTEPOE_TYPE_90W = 0x0b,
    LTC4274_LTEPOE_TYPE_38_7W = 0xe,
    LTC4274_LTEPOE_RESERVED,
    LTC4274_CLASS_ERROR
} ePSEClassType;

typedef enum { LTC4274_STATE_OK = 0, LTC4274_STATE_NOTOK } ePSEState;

typedef enum {
    LTC4274_NO_ACTIVE_ALERT = 0x00,
    LTC4274_POWER_ENABLE_ALERT = 0x01,
    LTC4274_POWERGOOD_ALERT = 0x02,
    LTC4274_DISCONNECT_ALERT = 0x04,
    LTC4274_DETECTION_ALERT = 0x08,
    LTC4274_CLASS_ALERT = 0x10,
    LTC4274_TCUT_ALERT = 0x20,
    LTC4274_TSTART_ALERT = 0x40,
    LTC4274_SUPPLY_ALERT = 0x80
} ePSEAlert;

typedef void (*LTC4274_CallbackFn)(LTC4274_Event evt, void *context);

typedef struct LTC4274_Cfg {
    I2C_Dev i2c_dev;
    OcGpio_Pin *pin_evt;
    OcGpio_Pin reset_pin;
} LTC4274_Cfg;

typedef struct LTC4274_Obj {
    LTC4274_CallbackFn alert_cb;
    void *cb_context;
    GateMutex_Handle mutex;
} LTC4274_Obj;

typedef struct LTC4274_Dev {
    const LTC4274_Cfg cfg;
    LTC4274_Obj obj;
} LTC4274_Dev;

ReturnStatus ltc4274_set_cfg_operation_mode(const I2C_Dev *i2c_dev,
                                            uint8_t operatingMode);
ReturnStatus ltc4274_get_operation_mode(const I2C_Dev *i2c_dev,
                                        uint8_t *operatingMode);
ReturnStatus ltc4274_set_cfg_detect_enable(const I2C_Dev *i2c_dev,
                                           uint8_t detectEnable);
ReturnStatus ltc4274_get_detect_enable(const I2C_Dev *i2c_dev,
                                       uint8_t *detectVal);
ReturnStatus ltc4274_set_interrupt_mask(const I2C_Dev *i2c_dev,
                                        uint8_t interruptMask);
ReturnStatus ltc4274_get_interrupt_mask(const I2C_Dev *i2c_dev,
                                        uint8_t *intrMask);
ReturnStatus ltc4274_cfg_interrupt_enable(const I2C_Dev *i2c_dev, bool enable);
ReturnStatus ltc4274_get_interrupt_enable(const I2C_Dev *i2c_dev,
                                          uint8_t *interruptEnable);
ReturnStatus ltc4274_set_cfg_pshp_feature(const I2C_Dev *i2c_dev,
                                          uint8_t hpEnable);
ReturnStatus ltc4274_get_pshp_feature(const I2C_Dev *i2c_dev,
                                      uint8_t *hpEnable);
ReturnStatus ltc4274_get_detection_status(const I2C_Dev *i2c_dev,
                                          ePSEDetection *pseDetect);
ReturnStatus ltc4274_get_class_status(const I2C_Dev *i2c_dev,
                                      ePSEClassType *pseClass);
ReturnStatus ltc4274_get_powergood_status(const I2C_Dev *i2c_dev,
                                          uint8_t *psePwrGood);
void ltc4274_set_alert_handler(LTC4274_Dev *dev, LTC4274_CallbackFn alert_cb,
                               void *cb_context);
ReturnStatus ltc4274_clear_interrupt(const I2C_Dev *i2c_dev, uint8_t *pwrEvent,
                                     uint8_t *overCurrent, uint8_t *supply);
ReturnStatus ltc4274_get_interrupt_status(const I2C_Dev *i2c_dev, uint8_t *val);
ReturnStatus ltc4274_debug_write(const I2C_Dev *i2c_dev, uint8_t reg_address,
                                 uint8_t value);
ReturnStatus ltc4274_debug_read(const I2C_Dev *i2c_dev, uint8_t reg_address,
                                uint8_t *value);
void ltc4274_enable(LTC4274_Dev *dev, uint8_t enableVal);
ReturnStatus ltc4274_get_devid(const I2C_Dev *i2c_dev, uint8_t *devID);
ReturnStatus ltc4274_detect(const I2C_Dev *i2c_dev, uint8_t *detect,
                            uint8_t *val);
void ltc4274_config(LTC4274_Dev *dev);
ePostCode ltc4274_probe(const LTC4274_Dev *i2c_dev, POSTData *postData);
void ltc4274_init(LTC4274_Dev *dev);
void ltc4274_initPSEStateInfo(void);
void ltc4274_update_stateInfo(const I2C_Dev *i2c_dev);
ReturnStatus ltc4274_reset(LTC4274_Dev *dev);

#endif /* LTC4274_H_ */
