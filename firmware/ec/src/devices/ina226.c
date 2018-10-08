/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/devices/ina226.h"

#include "devices/i2c/threaded_int.h"
#include "inc/common/byteorder.h"
#include "inc/common/global_header.h"
#include "helpers/memory.h"

/*****************************************************************************
 *                          REGISTER DEFINITIONS
 *****************************************************************************/
#define INA_CONFIGURATION_REG 0x00
#define INA_SHUNTVOLTAGE_REG 0x01
#define INA_BUSVOLTAGE_REG 0x02
#define INA_POWER_REG 0x03
#define INA_CURRENT_REG 0x04
#define INA_CALIBRATION_REG 0x05
#define INA_MASKENABLE_REG 0x06
#define INA_ALERTLIMIT_REG 0x07
#define INA_MANUFACTUREID_REG 0xFE
#define INA_DIEID_REG 0xFF

/*INA226 Device Info */
#define INA226_MANFACTURE_ID 0x5449
#define INA226_DEVICE_ID 0x2260
#define INA226_DEV_VERSION 0x00

/* Configuration Register Bits */
#define INA_CFG_RESET (1 << 15)

/*
 * Conversion of current into Shunt Voltage Register contents and viceversa.
 *
 * First Calculate the Current Register Value from the given Current Value
 *  ui16rfINARegValue = ui16rfINACurrentLimit/(INA226_CURRENT_LSB);
 * Calculate Shunt Voltage Alert Limit Register Value
 *  ui16rfINARegValue = (ui16rfINARegValue * 2048)/INA226_CALIBRATION_REG_VALUE;
 */
#define CURRENT_TO_REG(x) \
    ((2048 * (x / INA226_CURRENT_LSB) / INA226_CAL_REG_VALUE))
#define REG_TO_CURRENT(y) \
    ((y * INA226_CURRENT_LSB * INA226_CAL_REG_VALUE) / 2048)

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS
 *****************************************************************************/
/* INA226 LSB Values */
#define INA226_VSHUNT_LSB 2.5 /* 2.5uV or 2500nV  (uV default) */
#define INA226_VBUS_LSB 1.25 /* 1.25mV or 1250uV (mV default) */
#define INA226_CURRENT_LSB 0.1 /* 0.100mA 0r 100uA (mA default) */
#define INA226_POWER_LSB 2.5 /* 2.5mW or 2500uW  (mW default) */

/* Configure the Configuration register with Number of Samples and Conversion
 * Time for Shunt and Bus Voltage.
 * Min(Default):0x4127; Max: 0x4FFF; Average: 0x476F
 */
#define INA226_CONFIG_REG_VALUE 0x476F

/* Configure Calibration register with shunt resistor value and current LSB.
 Current_LSB = Maximum Expected Current/2^15
 Current_LSB = 2A/2^15 = 0.00006103515625 = 61uA ~ 100uA(Maximum Expected Current = 2A)
 Calibration Register(CAL) = 0.00512/(Current_LSB*RSHUNT)
 CAL = 0.00512/(100uA*2mOhm) =  = 25600 = 0x6400.(RSHUNT = 2mohm)
 */
#define INA226_CAL_REG_VALUE 0x6400

#define INA226_MASKEN_REG_VALUE 0x8001

/*****************************************************************************
 **    FUNCTION NAME   : read_ina_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from INA226 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus read_ina_reg(const INA226_Dev *dev, uint8_t regAddress,
                                 uint16_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle inaHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!inaHandle) {
        LOGGER_ERROR("INASENSOR:ERROR:: Failed to get I2C Bus for INA sensor "
                     "0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        status = i2c_reg_read(inaHandle, dev->cfg.dev.slave_addr, regAddress,
                              regValue, 2);
        *regValue = betoh16(*regValue);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : write_ina_reg
 **
 **    DESCRIPTION     : Write 16 bit value to INA226 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus write_ina_reg(const INA226_Dev *dev, uint8_t regAddress,
                                  uint16_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle inaHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!inaHandle) {
        LOGGER_ERROR("INASENSOR:ERROR:: Failed to get I2C Bus for INA sensor "
                     "0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        regValue = htobe16(regValue);
        status = i2c_reg_write(inaHandle, dev->cfg.dev.slave_addr, regAddress,
                               regValue, 2);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_dev_id
 **
 **    DESCRIPTION     : Read the device id of Current sensor.
 **
 **    ARGUMENTS       : i2c device and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus ina226_getDevId(INA226_Dev *dev, uint16_t *devID)
{
    return read_ina_reg(dev, INA_DIEID_REG, devID);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_mfg_id
 **
 **    DESCRIPTION     : Read the mfg id of Current sensor.
 **
 **    ARGUMENTS       : i2c device and out-pointer to manufacturing ID.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus ina226_getMfgId(INA226_Dev *dev, uint16_t *mfgID)
{
    return read_ina_reg(dev, INA_MANUFACTUREID_REG, mfgID);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_set_cfg_reg
 **
 **    DESCRIPTION     : Write the value to Current sensor configuration
 **                      register.
 **
 **    ARGUMENTS       : i2c device and new value of configuration register.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus _set_cfg_reg(INA226_Dev *dev, uint16_t regValue)
{
    return write_ina_reg(dev, INA_CONFIGURATION_REG, regValue);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_set_cal_reg
 **
 **    DESCRIPTION     : Write the value to Current sensor calibration register.
 **
 **    ARGUMENTS       : i2c device and new value of calibration register.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus _set_cal_reg(INA226_Dev *dev, uint16_t regValue)
{
    return write_ina_reg(dev, INA_CALIBRATION_REG, regValue);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_curr_limit
 **
 **    DESCRIPTION     : Read the value of Current sensor alert limit register.
 **
 **    ARGUMENTS       : i2c device and out-pointer to current limit.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ina226_readCurrentLim(INA226_Dev *dev, uint16_t *currLimit)
{
    uint16_t regValue = 0x0000;
    ReturnStatus status = read_ina_reg(dev, INA_ALERTLIMIT_REG, &regValue);
    if (status == RETURN_OK) {
        *currLimit = REG_TO_CURRENT(regValue);
        LOGGER_DEBUG("INASENSOR:INFO:: INA sensor 0x%x on bus 0x%x is "
                     "reporting current limit  of %d mA.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, *currLimit);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_set_curr_limit
 **
 **    DESCRIPTION     : Write the value to Current sensor alert limit register.
 **
 **    ARGUMENTS       : i2c device and new current limit.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ina226_setCurrentLim(INA226_Dev *dev, uint16_t currLimit)
{
    uint16_t regValue = CURRENT_TO_REG(currLimit);
    return write_ina_reg(dev, INA_ALERTLIMIT_REG, regValue);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_read_alert_reg
 **
 **    DESCRIPTION     : Read the value to Current sensor mask/enable register.
 **
 **    ARGUMENTS       : i2c device and out-pointer to enable and mask bits.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus _read_alert_reg(INA226_Dev *dev, uint16_t *regValue)
{
    return read_ina_reg(dev, INA_MASKENABLE_REG, regValue);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_enable_alert
 **
 **    DESCRIPTION     : Write the value to Current sensor mask/enable register.
 **
 **    ARGUMENTS       : i2c device and alert to be enabled.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus _enable_alert(INA226_Dev *dev, uint16_t regValue)
{
    return write_ina_reg(dev, INA_MASKENABLE_REG, regValue);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_bus_volt_value
 **
 **    DESCRIPTION     : Read the value of Current sensor bus voltage value.
 **
 **    ARGUMENTS       : i2c device and out-pointer to bus voltage value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ina226_readBusVoltage(INA226_Dev *dev, uint16_t *busVoltValue)
{
    uint16_t regValue;
    ReturnStatus status = read_ina_reg(dev, INA_BUSVOLTAGE_REG, &regValue);

    if (status == RETURN_OK) {
        *busVoltValue = regValue * INA226_VBUS_LSB;
        LOGGER_DEBUG("INASENSOR:INFO:: INA sensor 0x%x on bus 0x%x is "
                     "reporting bus voltage value  of %d mV.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, *busVoltValue);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_shunt_volt_value
 **
 **    DESCRIPTION     : Read the value of Current sensor shunt voltage value.
 **
 **    ARGUMENTS       : i2c device and out-pointer to shunt voltage.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ina226_readShuntVoltage(INA226_Dev *dev, uint16_t *shuntVoltValue)
{
    uint16_t regValue;
    ReturnStatus status = read_ina_reg(dev, INA_SHUNTVOLTAGE_REG, &regValue);

    if (status == RETURN_OK) {
        *shuntVoltValue = regValue * INA226_VSHUNT_LSB;
        LOGGER_DEBUG("INASENSOR:INFO:: INA sensor 0x%x on bus 0x%x is "
                     "reporting shunt voltage value  of %d uV.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus,
                     *shuntVoltValue);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_curr_value
 **
 **    DESCRIPTION     : Read the value of Current sensor current value.
 **
 **    ARGUMENTS       : i2c device and out-pointer to current value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ina226_readCurrent(INA226_Dev *dev, uint16_t *currValue)
{
    uint16_t regValue;
    ReturnStatus status = read_ina_reg(dev, INA_CURRENT_REG, &regValue);

    if (status == RETURN_OK) {
        *currValue = regValue * INA226_CURRENT_LSB;
        LOGGER_DEBUG("INASENSOR:INFO:: INA sensor 0x%x on bus 0x%x "
                     "is reporting current value  of %d mA.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, *currValue);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_power_value
 **
 **    DESCRIPTION     : Read the value of Current sensor power value.
 **
 **    ARGUMENTS       : i2c device and out-pointer to power value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ina226_readPower(INA226_Dev *dev, uint16_t *powValue)
{
    uint16_t regValue;
    ReturnStatus status = read_ina_reg(dev, INA_POWER_REG, &regValue);
    if (status == RETURN_OK) {
        *powValue = regValue * INA226_POWER_LSB;
        LOGGER_DEBUG("INASENSOR:INFO:: INA sensor 0x%x on bus 0x%x is "
                     "reporting power value  of %d mV.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, *powValue);
    }
    return status;
}

/*****************************************************************************
 * Internal IRQ handler - reads in triggered interrupts and dispatches CBs
 *****************************************************************************/
static void _ina226_isr(void *context)
{
    INA226_Dev *dev = context;

    /* Read the alert mask register (will clear the alert bit if set) */
    /* TODO: this seems to be a strange bug in the sensor - sometimes it returns
     * 0xFF as the lo-byte. If this occurs, we need to re-read it.
     * NOTE: 0x1F is a perfectly legal value, but bits 5-9 are RFU and
     * normally zero */
    uint16_t alert_mask = 0xFFFF;
    while (LOBYTE(alert_mask) == 0xFF) {
        if (_read_alert_reg(dev, &alert_mask) != RETURN_OK) {
            LOGGER_DEBUG("INA226:ERROR:: INT mask read failed\n");
            return;
        }
    }

    if (!dev->obj.alert_cb) {
        return;
    }

    if (alert_mask & INA_MSK_AFF) {
        /* This alert was caused by a fault */

        /* Theory of operation: After reading the alert, we change the alert
         * mask to look at the complement alert. For example, after getting a
         * bus over-voltage alert, we switch the mask to tell us when it's
         * under-voltage and thus back in operating limits so that it's less
         * likely that we'll miss alerts on the shared line, although not
         * guaranteed :( */

        /* The device can only monitor one metric at a time, if multiple flags
         * are set, it monitors the highest order bit, so check the config
         * in order, from MSB to LSB */
        uint16_t value;
        uint16_t new_mask = alert_mask & (~INA_ALERT_EN_MASK);
        INA226_Event evt;
        uint16_t alert_lim;
        ina226_readCurrentLim(dev, &alert_lim);

        if (alert_mask & INA_MSK_SOL) {
            if (dev->obj.evt_to_monitor == INA226_EVT_COL ||
                dev->obj.evt_to_monitor == INA226_EVT_CUL) {
                if (ina226_readCurrent(dev, &value) != RETURN_OK) {
                    value = UINT16_MAX;
                }
                alert_lim -= INA_HYSTERESIS;
                evt = INA226_EVT_COL;
            } else {
                if (ina226_readShuntVoltage(dev, &value) != RETURN_OK) {
                    value = UINT16_MAX;
                }
                evt = INA226_EVT_SOL;
            }
            new_mask |= INA_MSK_SUL;
        } else if (alert_mask & INA_MSK_SUL) {
            if (dev->obj.evt_to_monitor == INA226_EVT_CUL ||
                dev->obj.evt_to_monitor == INA226_EVT_COL) {
                if (ina226_readCurrent(dev, &value) != RETURN_OK) {
                    value = UINT16_MAX;
                }
                alert_lim += INA_HYSTERESIS;
                evt = INA226_EVT_CUL;
            } else {
                if (ina226_readShuntVoltage(dev, &value) != RETURN_OK) {
                    value = UINT16_MAX;
                }
                evt = INA226_EVT_SUL;
            }
            new_mask |= INA_MSK_SOL;
        } else if (alert_mask & INA_MSK_BOL) {
            if (ina226_readBusVoltage(dev, &value) != RETURN_OK) {
                value = UINT16_MAX;
            }
            evt = INA226_EVT_BOL;
            new_mask |= INA_MSK_BUL;
        } else if (alert_mask & INA_MSK_BUL) {
            if (ina226_readBusVoltage(dev, &value) != RETURN_OK) {
                value = UINT16_MAX;
            }
            evt = INA226_EVT_BUL;
            new_mask |= INA_MSK_BOL;
        } else if (alert_mask & INA_MSK_POL) {
            if (ina226_readPower(dev, &value) != RETURN_OK) {
                value = UINT16_MAX;
            }
            evt = INA226_EVT_POL;
            /* TODO: there isn't a PUL alert, not sure what to do here. We
             * don't currently use this alert, but it would be nice to have a
             * complete driver */
            new_mask |= INA_MSK_POL;
        } else {
            LOGGER_ERROR("INA226:Unknown alert type\n");
            return;
        }

        /* Set a new limit in order to account for hysteresis */
        /* TODO: make this work for all alert types (this is a hack) */
        ina226_setCurrentLim(dev, alert_lim);

        /* Invert the alert type we're looking for */
        if (_enable_alert(dev, new_mask) != RETURN_OK) {
            /* TODO [HACK]: this sometimes reports failures at random times, so
             * this is a hacked together retry to keep things stable*/
            _enable_alert(dev, new_mask);
        }

        dev->obj.alert_cb(evt, value, dev->obj.cb_context);
    }
    /* TODO: Conversion ready not handled */
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus ina226_init(INA226_Dev *dev)
{
    ReturnStatus status;
    dev->obj = (INA226_Obj){};

    /* Perform a device reset to be safe */
    status = _set_cfg_reg(dev, INA_CFG_RESET);
    if (status != RETURN_OK) {
        return status;
    }

    /* Configure the Configuration register with number of samples and
     * conversion time for shunt and bus voltage */
    status = _set_cfg_reg(dev, INA226_CONFIG_REG_VALUE);
    if (status != RETURN_OK) {
        return status;
    }

    /* Configure the Calibration register with shunt resistor value and
     * current LSB */
    status = _set_cal_reg(dev, INA226_CAL_REG_VALUE);
    if (status != RETURN_OK) {
        return status;
    }

    /* Make sure we're talking to the right device */
    //    if (ina226_probe(dev) != POST_DEV_FOUND) {
    //        return RETURN_NOTOK;
    //    }

    if (dev->cfg.pin_alert) {
        const uint32_t pin_evt_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING;
        if (OcGpio_configure(dev->cfg.pin_alert, pin_evt_cfg) <
            OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }

        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(dev->cfg.pin_alert, _ina226_isr, (void *)dev);
    }
    return RETURN_OK;
}

/*****************************************************************************
 *****************************************************************************/
void ina226_setAlertHandler(INA226_Dev *dev, INA226_CallbackFn alert_cb,
                            void *cb_context)
{
    dev->obj.alert_cb = alert_cb;
    dev->obj.cb_context = cb_context;
}

/*****************************************************************************
 *****************************************************************************/
ReturnStatus ina226_enableAlert(INA226_Dev *dev, INA226_Event evt)
{
    /* TODO: perhaps caching the mask is better? If we have an active alert,
     * we'll inadvertently clear it here */
    /* TODO: this isn't thread safe, but does it need to be? */

    uint16_t alert_mask;
    ReturnStatus res = _read_alert_reg(dev, &alert_mask);
    if (res != RETURN_OK) {
        return res;
    }

    alert_mask &= (~INA_ALERT_EN_MASK); /* Wipe out previous alert EN bits */
    //alert_mask |= (INA_MSK_LEN); /* Enable latch mode (never miss an alert) */
    dev->obj.evt_to_monitor = evt;
    switch (evt) {
        case INA226_EVT_COL:
            alert_mask |= INA_MSK_SOL;
            break;
        case INA226_EVT_CUL:
            alert_mask |= INA_MSK_SUL;
            break;
        default:
            alert_mask |= evt;
    }
    return _enable_alert(dev, alert_mask);
}

/*****************************************************************************
 *****************************************************************************/
ePostCode ina226_probe(INA226_Dev *dev, POSTData *postData)
{
    uint16_t devId = 0x00;
    uint16_t manfId = 0x0000;
    if (ina226_getDevId(dev, &devId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (devId != INA226_DEVICE_ID) {
        return POST_DEV_ID_MISMATCH;
    }

    if (ina226_getMfgId(dev, &manfId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (manfId != INA226_MANFACTURE_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, dev->cfg.dev.bus, dev->cfg.dev.slave_addr,
                         manfId, devId);
    return POST_DEV_FOUND;
}
