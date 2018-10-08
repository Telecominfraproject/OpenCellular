/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/devices/se98a.h"

#include "inc/common/byteorder.h"
#include "devices/i2c/threaded_int.h"
#include "helpers/math.h"
#include "helpers/memory.h"

#include <ti/sysbios/knl/Task.h>
#include <math.h>

/*****************************************************************************
 *                          Register Definitions
 *****************************************************************************/
/* Register Addresses */
#define SE98A_REG_CAPS 0x00
#define SE98A_REG_CFG 0x01
#define SE98A_REG_HIGH_LIM 0x02
#define SE98A_REG_LOW_LIM 0x03
#define SE98A_REG_CRIT_LIM 0x04
#define SE98A_REG_TEMP 0x05
#define SE98A_REG_MFG_ID 0x06
#define SE98A_REG_DEV_ID 0x07

/* Temperature Sensor Info */
#define SE98A_MFG_ID 0x1131
#define SE98A_DEV_ID 0xA1

/* Configuration Bits */
#define SE98A_CFG_HEN_H (1 << 10) /* Hysteresis Enable High Bit */
#define SE98A_CFG_HEN_L (1 << 9) /* Hysteresis Enable Low Bit */
#define SE98A_CFG_SHMD (1 << 8) /* Shutdown Mode */

#define SE98A_CFG_CTLB (1 << 7) /* Critical Trip Lock Bit */
#define SE98A_CFG_AWLB (1 << 6) /* Alarm Window Lock Bit */
#define SE98A_CFG_CEVENT (1 << 5) /* (WO) Clear EVENT */
#define SE98A_CFG_ESTAT (1 << 4) /* (RO) EVENT Status */

#define SE98A_CFG_EOCTL (1 << 3) /* EVENT Output Control */
#define SE98A_CFG_CVO (1 << 2) /* Critical Event Only */
#define SE98A_CFG_EP (1 << 1) /* EVENT Polarity */
#define SE98A_CFG_EMD (1 << 0) /* EVENT Mode */

#define SE98A_CFG_HYS_0 (0x0 << 9)
#define SE98A_CFG_HYS_1P5 (0x1 << 9)
#define SE98A_CFG_HYS_3 (0x2 << 9)
#define SE98A_CFG_HYS_6 (0x3 << 9)

/* Default CFG plus interrupt mode (we don't support comparator mode) */
#define SE98A_CONFIG_DEFAULT (0x0000 | SE98A_CFG_EMD | SE98A_CFG_HYS_1P5)

/*****************************************************************************
 * Helper to read from a SE98A register
 *****************************************************************************/
static ReturnStatus se98a_reg_read(const SE98A_Dev *dev, uint8_t regAddress,
                                   uint16_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle tempHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!tempHandle) {
        LOGGER_ERROR("SE98A:ERROR:: Failed to get I2C Bus for Temperature "
                     "sensor 0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        status = i2c_reg_read(tempHandle, dev->cfg.dev.slave_addr, regAddress,
                              regValue, 2);
        *regValue = betoh16(*regValue);
    }
    return status;
}

/*****************************************************************************
 * Helper to write to a SE98A register
 *****************************************************************************/
static ReturnStatus se98a_reg_write(const SE98A_Dev *dev, uint8_t regAddress,
                                    uint16_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle tempHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!tempHandle) {
        LOGGER_ERROR("SE98A:ERROR:: Failed to get I2C Bus for Temperature "
                     "sensor 0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        regValue = htobe16(regValue);
        status = i2c_reg_write(tempHandle, dev->cfg.dev.slave_addr, regAddress,
                               regValue, 2);
    }
    return status;
}

/*****************************************************************************
 * Helper to read the device ID
 *****************************************************************************/
static ReturnStatus se98a_get_dev_id(const SE98A_Dev *dev, uint8_t *devID)
{
    uint16_t regValue;
    ReturnStatus status = se98a_reg_read(dev, SE98A_REG_DEV_ID, &regValue);
    if (status == RETURN_OK) {
        /* Strip off the revision - we don't care about it right now */
        *devID = HIBYTE(regValue);
    }
    return status;
}

/*****************************************************************************
 * Helper to read the manufacturer ID
 *****************************************************************************/
static ReturnStatus se98a_get_mfg_id(const SE98A_Dev *dev, uint16_t *mfgID)
{
    return se98a_reg_read(dev, SE98A_REG_MFG_ID, mfgID);
}

/*****************************************************************************
 * Helper to write to the configuration register
 *****************************************************************************/
static ReturnStatus se98a_set_config_reg(const SE98A_Dev *dev,
                                         uint16_t configValue)
{
    return se98a_reg_write(dev, SE98A_REG_CFG, configValue);
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus se98a_set_limit(SE98A_Dev *dev,
                             eTempSensor_ConfigParamsId limitToConfig,
                             int8_t tempLimitValue)
{
    uint8_t regAddress = 0x00;

    /* Get the limit to configure */
    switch (limitToConfig) {
        case CONF_TEMP_SE98A_LOW_LIMIT_REG:
            regAddress = SE98A_REG_LOW_LIM;
            break;
        case CONF_TEMP_SE98A_HIGH_LIMIT_REG:
            regAddress = SE98A_REG_HIGH_LIM;
            break;
        case CONF_TEMP_SE98A_CRITICAL_LIMIT_REG:
            regAddress = SE98A_REG_CRIT_LIM;
            break;
        default:
            return RETURN_NOTOK;
    }

    /*
     * [15..13] RFU
     * [12]     SIGN (2's complement)
     * [11..4]  Integer part (8 bits)
     * [3..2]   Fractional part (0.5, 0.25)
     * [1..0]   RFU
     */

    /* The device technically takes an int9, but is only rated from
     * -40 to +125, so we'll settle for an int8 */
    uint16_t regValue = ((int16_t)tempLimitValue & 0x00FF) << 4;

    /* Set the sign bit if negative */
    if (tempLimitValue < 0) {
        regValue |= 0x1000;
    }

    return se98a_reg_write(dev, regAddress, regValue);
}

/*****************************************************************************
 * Helper to convert a SE98A register value to a temperature
 *****************************************************************************/
static int8_t reg2temp(uint16_t reg)
{
    /* The limit regs have lower precision, so by making this function common,
     * we lose 0.125 precision... since we round to the nearest int, I'm not
     * worried */

    /* Calculate the Actual Temperature Value from fixed point register
     * (bottom 4 bits are fractional part - divide by 16)
     *
     * Register map REG_TEMP / REG_LIM
     * [15..13] Trip Status  / RFU
     * [12]     Sign         / Sign
     * [11..5]  8-bit Int    / 8-bit Int
     * [4..2]   Fraction     / Fraction
     * [1]      Fraction     / RFU
     * [0]      RFU
     */
    int16_t temperature = (reg & 0x0FFC);

    /* If negative, upper bits must be set (assume a 2's comp. system) */
    if (reg & 0x1000) {
        temperature |= 0xF000;
    }
    temperature = round(temperature / 16.0f);

    return CONSTRAIN(temperature, INT8_MIN, INT8_MAX);
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus se98a_read(SE98A_Dev *dev, int8_t *tempValue)
{
    /* The temperature value is 2's complement with the LSB equal to 0.0625.
     * The resolution is 0.125 (bit 0 is unused)
     */
    uint16_t regValue = 0x0000;
    ReturnStatus status = se98a_reg_read(dev, SE98A_REG_TEMP, &regValue);
    if (status == RETURN_OK) {
        *tempValue = reg2temp(regValue);
        LOGGER_DEBUG("TEMPSENSOR:INFO:: Temperature sensor 0x%x on bus "
                     "0x%x is reporting Temperature value of %d Celsius.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, *tempValue);
    }
    return status;
}

/*****************************************************************************
 * Helper to read the configuration register
 *****************************************************************************/
static ReturnStatus se98a_get_config_reg(const SE98A_Dev *dev,
                                         uint16_t *configValue)
{
    return se98a_reg_read(dev, SE98A_REG_CFG, configValue);
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus se98a_get_limit(SE98A_Dev *dev,
                             eTempSensor_ConfigParamsId limitToConfig,
                             int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint16_t regValue = 0x0000;
    uint8_t regAddress = 0x0000;

    /*getting the limit to configure */
    switch (limitToConfig) {
        case CONF_TEMP_SE98A_LOW_LIMIT_REG:
            regAddress = SE98A_REG_LOW_LIM;
            break;
        case CONF_TEMP_SE98A_HIGH_LIMIT_REG:
            regAddress = SE98A_REG_HIGH_LIM;
            break;
        case CONF_TEMP_SE98A_CRITICAL_LIMIT_REG:
            regAddress = SE98A_REG_CRIT_LIM;
            break;
        default:
            return RETURN_NOTOK;
    }

    status = se98a_reg_read(dev, regAddress, &regValue);
    if (status == RETURN_OK) {
        *tempLimitValue = reg2temp(regValue);
        LOGGER_DEBUG("TEMPSENSOR:INFO:: Temperature sensor 0x%x on bus "
                     "0x%x is having Limit configure to  0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus,
                     *tempLimitValue);
    }
    return status;
}

/*****************************************************************************
 * Internal IRQ handler - reads in triggered interrupts and dispatches CBs
 *****************************************************************************/
static void se98a_handle_irq(void *context)
{
    SE98A_Dev *dev = context;

    ReturnStatus res = RETURN_NOTOK;
    const IArg mutexKey = GateMutex_enter(dev->obj.mutex);
    {
        /* See if this event was from us (we can't just read the trip status
         * since those bits are sticky - they could be from an old event) */
        uint16_t config_reg;
        if ((se98a_get_config_reg(dev, &config_reg) == RETURN_OK) &&
            (config_reg & SE98A_CFG_ESTAT)) {
            /* Clear the event */
            config_reg |= SE98A_CFG_CEVENT;
            res = se98a_set_config_reg(dev, config_reg);
        }
    }
    GateMutex_leave(dev->obj.mutex, mutexKey);

    if (res != RETURN_OK) {
        return;
    }

    /* Read the temperature register which also contains event status */
    uint16_t regValue;
    if (se98a_reg_read(dev, SE98A_REG_TEMP, &regValue) != RETURN_OK) {
        /* Something really strange happened */
        return;
    }

    /* Grab the upper 3 bits which represent the event trip status */
    uint8_t trip_stat = (regValue >> 13) & 0x07;
    int8_t temperature = reg2temp(regValue);

    /* See if we have a callback assigned to handle alerts */
    if (!dev->obj.alert_cb) {
        return;
    }

    /* Since > CRIT implies above window, we only handle the highest priority
     * event to avoid duplicate events being sent */
    if (trip_stat & SE98A_EVT_ACT) {
        dev->obj.alert_cb(SE98A_EVT_ACT, temperature, dev->obj.cb_context);
    } else if (trip_stat & SE98A_EVT_AAW) {
        dev->obj.alert_cb(SE98A_EVT_AAW, temperature, dev->obj.cb_context);
    } else if (trip_stat & SE98A_EVT_BAW) {
        dev->obj.alert_cb(SE98A_EVT_BAW, temperature, dev->obj.cb_context);
    }
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus se98a_init(SE98A_Dev *dev)
{
    dev->obj = (SE98A_Obj){};

    dev->obj.mutex = GateMutex_create(NULL, NULL);
    if (!dev->obj.mutex) {
        return RETURN_NOTOK;
    }

    /* Make sure we're talking to the right device */
    //if (se98a_probe(dev) != POST_DEV_FOUND) {
    //    return RETURN_NOTOK;
    //}

    /* The only way to truly reset this device is to cycle power - we'll just
     * clear out the config register to be safe and clear any interrupts from
     * a previous life */
    if (se98a_set_config_reg(dev, SE98A_CONFIG_DEFAULT | SE98A_CFG_CEVENT) !=
        RETURN_OK) {
        return RETURN_NOTOK;
    }

    if (dev->cfg.pin_evt) {
        const uint32_t pin_evt_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING;
        if (OcGpio_configure(dev->cfg.pin_evt, pin_evt_cfg) < OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }

        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(dev->cfg.pin_evt, se98a_handle_irq, (void *)dev);
    }
    return RETURN_OK;
}

/*****************************************************************************
 *****************************************************************************/
void se98a_set_alert_handler(SE98A_Dev *dev, SE98A_CallbackFn alert_cb,
                             void *cb_context)
{
    dev->obj.alert_cb = alert_cb;
    dev->obj.cb_context = cb_context;
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus se98a_enable_alerts(SE98A_Dev *dev)
{
    /* Wait 125ms after setting alarm window before enabling event pin
     * see datasheet page 8 - 7.3.2.1 Alarm window */
    Task_sleep(125);

    ReturnStatus res = RETURN_NOTOK;
    const IArg mutexKey = GateMutex_enter(dev->obj.mutex);
    {
        uint16_t config_reg;
        if (se98a_get_config_reg(dev, &config_reg) == RETURN_OK) {
            config_reg |= SE98A_CFG_EOCTL;
            res = se98a_set_config_reg(dev, config_reg);
        }
    }
    GateMutex_leave(dev->obj.mutex, mutexKey);
    return res;
}

/*****************************************************************************
 *****************************************************************************/
ePostCode se98a_probe(SE98A_Dev *dev, POSTData *postData)
{
    uint8_t devId = 0x00;
    uint16_t manfId = 0x0000;
    if (se98a_get_dev_id(dev, &devId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (devId != SE98A_DEV_ID) {
        return POST_DEV_ID_MISMATCH;
    }

    if (se98a_get_mfg_id(dev, &manfId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (manfId != SE98A_MFG_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, dev->cfg.dev.bus, dev->cfg.dev.slave_addr,
                         manfId, devId);
    return POST_DEV_FOUND;
}
