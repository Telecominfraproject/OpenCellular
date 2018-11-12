/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef SE98A_H_
#define SE98A_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "common/inc/global/Framework.h"
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"

#include <ti/sysbios/gates/GateMutex.h>

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
typedef enum SE98A_Event {
    SE98A_EVT_ACT = 1 << 2, /* Above critical temp */
    SE98A_EVT_AAW = 1 << 1, /* Above alarm window */
    SE98A_EVT_BAW = 1 << 0, /* Below alarm window */
} SE98A_Event;

typedef enum {
    CONF_TEMP_SE98A_LOW_LIMIT_REG = 1,
    CONF_TEMP_SE98A_HIGH_LIMIT_REG,
    CONF_TEMP_SE98A_CRITICAL_LIMIT_REG
} eTempSensor_ConfigParamsId;

typedef void (*SE98A_CallbackFn)(SE98A_Event evt, int8_t temperature,
                                 void *context);

typedef struct SE98A_Cfg {
    I2C_Dev dev;
    OcGpio_Pin *pin_evt;
} SE98A_Cfg;

typedef struct SE98A_Obj {
    SE98A_CallbackFn alert_cb;
    void *cb_context;
    GateMutex_Handle mutex;
} SE98A_Obj;

typedef struct SE98A_Dev {
    const SE98A_Cfg cfg;
    SE98A_Obj obj;
} SE98A_Dev;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
/*! Initializes an SE98A device
 * @param dev Pointer to the device struct containing hw config and object data
 * @return RETURN_OK on success, error code on failure
 */
ReturnStatus se98a_init(SE98A_Dev *dev);

/*! Registers a callback function to process alerts from the sensor
 * @param dev Device struct pointer
 * @param alert_db Function to call when alert is triggered
 * @param cb_context Pointer to pass to callback (optional)
 */
void se98a_set_alert_handler(SE98A_Dev *dev, SE98A_CallbackFn alert_cb,
                             void *cb_context);

/*! Enables the alert output pin on the SE98A
 * @param dev Device struct pointer
 * @return RETURN_OK on success, error code on failure
 */
ReturnStatus se98a_enable_alerts(SE98A_Dev *dev);

/* TODO: this is for legacy support for how subsystems currently perform
 * POST (probe device, then init) - I propose that this should all be
 * in a single function */
/*! Tests the SE98A device and verifies that the driver supports it
 * @param dev Device struct pointer, Post data struct
 * @return POST_DEV_FOUND on success, error code on failure
 */
ePostCode se98a_probe(SE98A_Dev *dev, POSTData *postData);

/*! Sets one of the 3 alert thresholds on the device
 * @param dev Device struct pointer
 * @param limitToConfig Which of the 3 limits to configure
 * @param tempLimitValue The temperature to set the threshold to
 * @return RETURN_OK on success, error code on failure
 */
ReturnStatus se98a_set_limit(SE98A_Dev *dev,
                             eTempSensor_ConfigParamsId limitToConfig,
                             int8_t tempLimitValue);

/*! Reads one of the 3 alert thresholds on the device
 * @param dev Device struct pointer
 * @param limitToConfig Which of the 3 limits to read
 * @param tempLimitValue Outparam containing the returned threshold
 * @return RETURN_OK on success, error code on failure
 */
ReturnStatus se98a_get_limit(SE98A_Dev *dev,
                             eTempSensor_ConfigParamsId limitToConfig,
                             int8_t *tempLimitValue);

/*! Reads the current temperature from the sensor
 * @param dev Device struct pointer
 * @param tempValue Outval containing the measured temperature
 * @return RETURN_OK on success, error code on failure
 */
ReturnStatus se98a_read(SE98A_Dev *dev, int8_t *tempValue);

#endif /* SE98A_H_ */
