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
#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
/* Mask/Enable Register Bits */
#define INA_ALERT_EN_MASK 0xF800 /* Upper 5 bits are the enable bits */
#define INA_MSK_SOL (1 << 15) /* Shunt over-voltage */
#define INA_MSK_SUL (1 << 14) /* Shunt under-voltage */
#define INA_MSK_BOL (1 << 13) /* Bus over-voltage */
#define INA_MSK_BUL (1 << 12) /* Bus under-voltage */
#define INA_MSK_POL (1 << 11) /* Power over limit */
#define INA_MSK_CNVR \
    (1 << 10) /* Conversion ready - enable alert when
                                 * CVRF is set (ready for next conversion) */

#define INA_MSK_AFF \
    (1 << 4) /* Alert Function Flag (caused by alert)
                                 * In latch mode, cleared on mask read */
#define INA_MSK_CVRF \
    (1 << 3) /* Conversion Ready Flag, cleared when
                                 * writing to cfg reg or mask read */
#define INA_MSK_OVF (1 << 2) /* Math Overflow Flag (data may be invalid) */
#define INA_MSK_APOL (1 << 1) /* Alert Polarity (1 = invert, active high) */
#define INA_MSK_LEN \
    (1 << 0) /* Alert Latch Enable
                                 * 1 Latch (alert only cleared by read to msk)
                                 * 0 Transparent (auto-clear on fault clear) */

#define INA_HYSTERESIS \
    30 /* 30mA TODO: need to make more robust, maybe percentage based */

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
typedef enum INA226_Event {
    INA226_EVT_SOL = INA_MSK_SOL, /* Shunt over-voltage */
    INA226_EVT_SUL = INA_MSK_SUL, /* Shunt under-voltage */
    INA226_EVT_BOL = INA_MSK_BOL, /* Bus over-voltage */
    INA226_EVT_BUL = INA_MSK_BUL, /* Bus under-voltage */
    INA226_EVT_POL = INA_MSK_POL, /* Power over limit */

    /* Meta Events */
    INA226_EVT_COL, /* Current over limit - based on SOL */
    INA226_EVT_CUL, /* Current under limit - based on SUL */
} INA226_Event;

typedef void (*INA226_CallbackFn)(INA226_Event evt, uint16_t value,
                                  void *context);

typedef struct INA226_Cfg {
    I2C_Dev dev;
    OcGpio_Pin *pin_alert;
} INA226_Cfg;

typedef struct INA226_Obj {
    INA226_CallbackFn alert_cb;
    void *cb_context;
    INA226_Event evt_to_monitor;
} INA226_Obj;

typedef struct INA226_Dev {
    const INA226_Cfg cfg;
    INA226_Obj obj;
} INA226_Dev;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus ina226_readCurrentLim(INA226_Dev *dev, uint16_t *currLimit);
ReturnStatus ina226_setCurrentLim(INA226_Dev *dev, uint16_t currLimit);
ReturnStatus ina226_readBusVoltage(INA226_Dev *dev, uint16_t *busVoltValue);
ReturnStatus ina226_readShuntVoltage(INA226_Dev *dev, uint16_t *shuntVoltValue);
ReturnStatus ina226_readCurrent(INA226_Dev *dev, uint16_t *currValue);
ReturnStatus ina226_readPower(INA226_Dev *dev, uint16_t *powValue);
ReturnStatus ina226_init(INA226_Dev *dev);
void ina226_setAlertHandler(INA226_Dev *dev, INA226_CallbackFn alert_cb,
                            void *cb_context);
ReturnStatus ina226_enableAlert(INA226_Dev *dev, INA226_Event evt);
ePostCode ina226_probe(INA226_Dev *dev, POSTData *postData);
#endif /* INA226_H_ */
