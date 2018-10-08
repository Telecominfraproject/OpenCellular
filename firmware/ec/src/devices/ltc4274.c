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
#include "inc/devices/ltc4274.h"

#include "Board.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"
#include "inc/subsystem/power/power.h"
#include "devices/i2c/threaded_int.h"

#include <stdlib.h>
#include <ti/sysbios/knl/Task.h>

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/

/* Register map */
#define LTC4274_REG_INTERRUPT_STATUS 0x00
#define LTC4274_REG_INTERRUPT_MASK 0x01
#define LTC4274_REG_POWER_EVENT 0x02
#define LTC4274_REG_POWER_EVENT_COR 0x03
#define LTC4274_REG_DETECT_EVENT 0x04
#define LTC4274_REG_DETECT_EVENT_COR 0x05
#define LTC4274_REG_FAULT_EVENT 0x06
#define LTC4274_REG_FAULT_EVENT_COR 0x07
#define LTC4274_REG_START_EVENT 0x08
#define LTC4274_REG_START_EVENT_COR 0x09
#define LTC4274_REG_SUPPLY_EVENT 0x0A
#define LTC4274_REG_SUPPLY_EVENT_COR 0x0B
#define LTC4274_REG_STATUS 0x0C
#define LTC4274_REG_POWER_STATUS 0x10
#define LTC4274_REG_PNI_STATUS 0x11
#define LTC4274_REG_OPERATION_MODE 0x12
#define LTC4274_REG_ENABLE_DUSCONNECT SENSING 0X13
#define LTC4274_REG_DETECT_CLASS_ENABLE 0x14
#define LTC4274_REG_MIDSPAN 0x15
#define LTC4274_REG_MCONF 0x17
#define LTC4274_REG_DETPB 0x18
#define LTC4274_REG_PWRPB 0x19
#define LTC4274_REG_RSTPB 0x1A
#define LTC4274_REG_ID 0x1B
#define LTC4274_REG_TLIMIT 0x1E
#define LTC4274_REG_IP1LSB 0x30
#define LTC4274_REG_IP1MSB 0x31
#define LTC4274_REG_VP1LSB 0x32
#define LTC4274_REG_VP1MSB 0x33
#define LTC4274_REG_FIRMWARE 0x41
#define LTC4274_REG_WDOG 0x42
#define LTC4274_REG_DEVID 0x43
#define LTC4274_REG_HP_ENABLE 0x44
#define LTC4274_REG_HP_MODE 0x46
#define LTC4274_REG_CUT1 0x47
#define LTC4274_REG_LIM1 0x48
#define LTC4274_REG_IHP_STATUS 0x49

/* Interrupt bits */
#define LTC4274_SUPPLY_INTERRUPT 0x80
#define LTC4274_TSTART_INTERRUPT 0x40
#define LTC4274_TCUT_INTERRUPT 0x20
#define LTC4274_CLASSIFICATION_INTERRUPT 0x10
#define LTC4274_DETECT_INTERRUPT 0x08
#define LTC4274_DISCONNECT_INTERRUPT 0x04
#define LTC4274_POWERGOOD_INTERRUPT 0x02
#define LTC4274_PWERENABLE_INTERRUPT 0x01

/* Events and status control bits*/
#define LTC4274_PWR_GOOD_BIT 0x10
#define LTC4274_PWR_STATE_CHANGE_BIT 0x01
#define LTC4274_CLASSIFICATION_COMPLETE_BIT 0x10
#define LTC4274_DETECT_COMPLETE_BIT 0X01
#define LTC4274_DISCONNECT_TDIS_BIT 0x10
#define LTC4274_OVERCURRENT_TCUT_BIT 0x01
#define LTC4274_CURRENT_LIMIT_TOUT_BIT 0x10
#define LTC4274_START_OCURRENT_TOUT_BIT 0X01
#define LTC4274_PWR_GOOD_BIT 0x10
#define LTC4274_PWR_ENABLE_BIT 0x01
#define LTC4274_LTPOE_BIT 0x80
#define LTC4274_CLASS_BIT 0x70
#define LTC4274_DETECT_BIT 0X07
#define LTC4274_ENABLE_CLASSIFICATION_BIT 0X10
#define LTC4274_ENABLE_DETECTION_BIT 0x01
#define LTC4274_SUPPLY_EVENT_FAILURE 0xF0

/* PSE Configuration */
#define LTC4274_INTERRUPT_SET 0x1E
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

typedef struct {
    ePSEDetection detectStatus;
    ePSEClassType classStatus;
    ePSEPowerState powerGoodStatus;
} tPower_PSEStatus_Data;

typedef struct {
    tPower_PSEStatus_Data pseStatus;
    ePSEAlert psealert;
} tPower_PSEStatus_Info;

static tPower_PSEStatus_Info PSEStatus_Info;

/******************************************************************************
 * @fn         ltc4274_raw_write
 *
 * @brief      Write to PSE register
 *
 * @args       I2C device, Slave address, register address and value to be written.
 *
 * @return     ReturnStatus
 */
ReturnStatus ltc4274_write(const I2C_Dev *i2c_dev, uint8_t regAddress,
                           uint8_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle pseHandle = i2c_get_handle(i2c_dev->bus);
    if (!pseHandle) {
        LOGGER_ERROR("LTC4274:ERROR:: Failed to get I2C Bus for PSE"
                     "0x%x on bus 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        status = i2c_reg_write(pseHandle, i2c_dev->slave_addr, regAddress,
                               regValue, 1);
    }
    return status;
}

/******************************************************************************
 * @fn         ltc4274_read
 *
 * @brief      read two bytesfrom PSE register.
 *
 * @args       I2C device, Slave address, register address and value read.
 *
 * @return     ReturnStatus
 */
ReturnStatus ltc4274_read(const I2C_Dev *i2c_dev, uint8_t regAddress,
                          uint8_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle pseHandle = i2c_get_handle(i2c_dev->bus);
    if (!pseHandle) {
        LOGGER_ERROR("LTC4274:ERROR:: Failed to get I2C Bus for PSE"
                     "0x%x on bus 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        /* TODO: refactor i2c_reg_read to not require uint16 */
        uint16_t value;
        status = i2c_reg_read(pseHandle, i2c_dev->slave_addr, regAddress,
                              &value, 1);
        *regValue = (uint8_t)value;
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_set_cfg_operation_mode
 *
 * @brief       Configure PSE operational mode.
 *
 * @args         I2c device struct, Address and operatingMode
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_set_cfg_operation_mode(const I2C_Dev *i2c_dev,
                                            uint8_t operatingMode)
{
    ReturnStatus status = RETURN_OK;
    status = ltc4274_write(i2c_dev, LTC4274_REG_OPERATION_MODE, operatingMode);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: Write failed to the Operation mode register of PSE.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_operation_mode
 *
 * @brief       read PSE operational mode.
 *
 * @args         I2c device struct, Address and operatingMode
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_operation_mode(const I2C_Dev *i2c_dev,
                                        uint8_t *operatingMode)
{
    ReturnStatus status = RETURN_OK;
    status = ltc4274_read(i2c_dev, LTC4274_REG_OPERATION_MODE, operatingMode);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: Read failed from the Operation mode register of PSE.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_set_cfg_detect_enable
 *
 * @brief       Configure PSE detect and classification enable.
 *
 * @args         I2c device struct, Address and detectEnable
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_set_cfg_detect_enable(const I2C_Dev *i2c_dev,
                                           uint8_t detectEnable)
{
    ReturnStatus status = RETURN_OK;

    //Enable detect and classfication of PD
    status = ltc4274_write(i2c_dev, LTC4274_REG_DETECT_CLASS_ENABLE,
                           detectEnable);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE detect enable setting failed.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_detect_enable
 *
 * @brief       Read PSE detect and classification enable configuration.
 *
 * @args         I2c device struct, Address and detectEnable
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_detect_enable(const I2C_Dev *i2c_dev,
                                       uint8_t *detectVal)
{
    ReturnStatus status = RETURN_OK;
    //Enable detect and classfication of PD
    uint8_t val = 0;
    status = ltc4274_read(i2c_dev, LTC4274_REG_DETECT_CLASS_ENABLE, &val);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE detect enable config read failed.\n");
    }
    *detectVal = (val & 0x07);
    return status;
}

/******************************************************************************
 * @fn          ltc4274_set_interrupt_mask
 *
 * @brief       Configure Interrupts for PSE device.
 *
 * @args         I2c device struct,Address and intrMask
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_set_interrupt_mask(const I2C_Dev *i2c_dev,
                                        uint8_t interruptMask)
{
    ReturnStatus status = RETURN_OK;
    /* Enable interrupts for power event,power good, supply related faults or
     * over currrent*/
    status = ltc4274_write(i2c_dev, LTC4274_REG_INTERRUPT_MASK, interruptMask);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE interrupt mask setting failed.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_interrupt_mask
 *
 * @brief       Read Interrupts for PSE device configuration.
 *
 * @args         I2c device struct, Address and intrMask
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_interrupt_mask(const I2C_Dev *i2c_dev,
                                        uint8_t *intrMask)
{
    ReturnStatus status = RETURN_OK;
    /* Enable interrupts for power event,power good, supply related faults or
     * over currrent*/
    status = ltc4274_read(i2c_dev, LTC4274_REG_INTERRUPT_MASK, intrMask);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE interrupt mask config read failed.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_cfg_interrupt_enable
 *
 * @brief       Configure Interrupts for PSE device.
 *
 * @args         I2c device struct, Address and interruptBit
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_cfg_interrupt_enable(const I2C_Dev *i2c_dev, bool enable)
{
    ReturnStatus status = RETURN_OK;
    uint8_t interruptEnable = 0x00;
    if (enable) {
        interruptEnable = LTC4274_INTERRUPT_ENABLE;
    }
    /*Enable interrupts from Misc register*/
    status = ltc4274_write(i2c_dev, LTC4274_REG_MCONF, interruptEnable);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE interrupt enable failed.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_interrupt_enable
 *
 * @brief       Reads Interrupts enable config for PSE device.
 *
 * @args         I2c device struct, Address and interruptBit
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_interrupt_enable(const I2C_Dev *i2c_dev,
                                          uint8_t *interruptEnable)
{
    ReturnStatus status = RETURN_OK;
    /*Enable interrupts from Misc register*/
    status = ltc4274_read(i2c_dev, LTC4274_REG_MCONF, interruptEnable);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE interrupt enable config read failed.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_set_cfg_pshp_feature
 *
 * @brief       Configure high power feature for LTEPOE++.
 *
 * @args         I2c device struct ,Address and hpEnable
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_set_cfg_pshp_feature(const I2C_Dev *i2c_dev,
                                          uint8_t hpEnable)
{
    ReturnStatus status = RETURN_OK;
    /*Enbale high power feature */
    status = ltc4274_write(i2c_dev, LTC4274_REG_HP_ENABLE, hpEnable);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE high power mode setting failed.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_pshp_feature
 *
 * @brief       read high power feature config for LTEPOE++.
 *
 * @args         I2c device struct, Address and hpEnable
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_pshp_feature(const I2C_Dev *i2c_dev, uint8_t *hpEnable)
{
    ReturnStatus status = RETURN_OK;
    /*Enbale high power feature */
    status = ltc4274_read(i2c_dev, LTC4274_REG_HP_ENABLE, hpEnable);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE high power mode config read failed.\n");
    }
    return status;
}
/******************************************************************************
 * @fn          _get_pse_class_enum
 *
 * @brief      get PSE classs.
 *
 * @args       value and pointer to class type
 *
 * @return     None
 */
static void _get_pse_class_enum(uint8_t val, ePSEClassType *pseClass)
{
    switch (val) {
        case LTC4274_CLASSTYPE_UNKOWN: {
            *pseClass = LTC4274_CLASSTYPE_UNKOWN;
        } break;
        case LTC4274_CLASSTYPE_1: {
            *pseClass = LTC4274_CLASSTYPE_1;
        } break;
        case LTC4274_CLASSTYPE_2: {
            *pseClass = LTC4274_CLASSTYPE_2;
        } break;
        case LTC4274_CLASSTYPE_3: {
            *pseClass = LTC4274_CLASSTYPE_3;
        } break;
        case LTC4274_CLASSTYPE_4: {
            *pseClass = LTC4274_CLASSTYPE_4;
        } break;
        case LTC4274_CLASSTYPE_RESERVED: {
            *pseClass = LTC4274_CLASSTYPE_RESERVED;
        } break;
        case LTC4274_CLASSTYPE_0: {
            *pseClass = LTC4274_CLASSTYPE_0;
        } break;
        case LTC4274_OVERCURRENT: {
            *pseClass = LTC4274_OVERCURRENT;
        } break;
        case LTC4274_LTEPOE_TYPE_52_7W: {
            *pseClass = LTC4274_LTEPOE_TYPE_52_7W;
        } break;
        case LTC4274_LTEPOE_TYPE_70W: {
            *pseClass = LTC4274_LTEPOE_TYPE_70W;
        } break;
        case LTC4274_LTEPOE_TYPE_90W: {
            *pseClass = LTC4274_LTEPOE_TYPE_90W;
        } break;
        case LTC4274_LTEPOE_TYPE_38_7W: {
            *pseClass = LTC4274_LTEPOE_TYPE_38_7W;
        } break;
        default: {
            *pseClass = LTC4274_LTEPOE_RESERVED;
        }
    }
}

/******************************************************************************
 * @fn          _get_pse_detect_enum
 *
 * @brief      get PSE detect.
 *
 * @args       value and pointer to detection status
 *
 * @return     None
 */
static void _get_pse_detect_enum(uint8_t val, ePSEDetection *pseDetect)
{
    switch (val) {
        case LTC4274_DETECT_UNKOWN: {
            *pseDetect = LTC4274_DETECT_UNKOWN;
        } break;
        case LTC4274_SHORT_CIRCUIT: {
            *pseDetect = LTC4274_SHORT_CIRCUIT;
        } break;
        case LTC4274_CPD_HIGH: {
            *pseDetect = LTC4274_CPD_HIGH;
        } break;
        case LTC4274_RSIG_LOW: {
            *pseDetect = LTC4274_RSIG_LOW;
        } break;
        case LTC4274_SIGNATURE_GOOD: {
            *pseDetect = LTC4274_SIGNATURE_GOOD;
        } break;
        case LTC4274_RSIG_TOO_HIGH: {
            *pseDetect = LTC4274_RSIG_TOO_HIGH;
        } break;
        case LTC4274_OPEN_CIRCUIT: {
            *pseDetect = LTC4274_OPEN_CIRCUIT;
        } break;
        default: {
            *pseDetect = LTC4274_DETECT_ERROR;
        }
    }
}
/******************************************************************************
 * @fn          ltc4274_get_detection_status
 *
 * @brief       Read PSE class
 *
 * @args         I2c device struct and pointer to class
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_detection_status(const I2C_Dev *i2c_dev,
                                          ePSEDetection *pseDetect)
{
    ReturnStatus status = RETURN_OK;
    uint8_t val = 0;
    status = ltc4274_read(i2c_dev, LTC4274_REG_DETECT_EVENT, &val);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE detection status read failed.\n");
    }
    if (!(LTC4274_DETECTION_COMPLETE(val))) {
        *pseDetect = LTC4274_DETECT_ERROR;
    } else {
        status = ltc4274_read(i2c_dev, LTC4274_REG_STATUS, &val);
        if (status != RETURN_OK) {
            LOGGER("LTC4274:ERROR:: PSE detection code read failed.\n");
        }
        val = LTC4374_DETECT(val);
        _get_pse_detect_enum(val, pseDetect);
    }

    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_class_status
 *
 * @brief       Read PSE class
 *
 * @args         I2c device struct and pointer to class
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_class_status(const I2C_Dev *i2c_dev,
                                      ePSEClassType *pseClass)
{
    ReturnStatus status = RETURN_OK;
    uint8_t val = 0;
    status = ltc4274_read(i2c_dev, LTC4274_REG_DETECT_EVENT, &val);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE class status read failed.\n");
    }
    if (!(LTC4274_CLASSIFICATION_COMPLETE(val))) {
        *pseClass = LTC4274_CLASS_ERROR;
    } else {
        status = ltc4274_read(i2c_dev, LTC4274_REG_STATUS, &val);
        if (status != RETURN_OK) {
            LOGGER("LTC4274:ERROR:: PSE class code read failed.\n");
        }
        val = LTC4374_CLASS(val);
        _get_pse_class_enum(val, pseClass);
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_powergood_status
 *
 * @brief       Read PSE power good
 *
 * @args         I2c device struct, Address and detectEnable
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_powergood_status(const I2C_Dev *i2c_dev,
                                          uint8_t *psePwrGood)
{
    ReturnStatus status = RETURN_OK;
    status = ltc4274_read(i2c_dev, LTC4274_REG_POWER_STATUS, psePwrGood);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: PSE power good read failed.\n");
    }
    if (LTC4274_PWRGD(*psePwrGood)) {
        *psePwrGood = LTC4274_POWERGOOD;
    } else {
        *psePwrGood = LTC4274_POWERGOOD_NOTOK;
    }
    return status;
}

/*****************************************************************************
 * Internal IRQ handler - reads in triggered interrupts and dispatches CBs
 *****************************************************************************/
static void ltc4274_handle_irq(void *context)
{
    LTC4274_Dev *dev = context;
    uint8_t alertVal;
    if (ltc4274_get_interrupt_status(&dev->cfg.i2c_dev, &alertVal) !=
        RETURN_OK) {
        /* Something really strange happened */
        return;
    }
    /* See if we have a callback assigned to handle alerts */
    if (!dev->obj.alert_cb) {
        return;
    }
    switch (alertVal) {
        case LTC4274_EVT_SUPPLY: {
            dev->obj.alert_cb(LTC4274_EVT_SUPPLY, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_TSTART: {
            dev->obj.alert_cb(LTC4274_EVT_TSTART, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_TCUT: {
            dev->obj.alert_cb(LTC4274_EVT_TCUT, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_CLASS: {
            dev->obj.alert_cb(LTC4274_EVT_CLASS, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_DETECTION: {
            dev->obj.alert_cb(LTC4274_EVT_DETECTION, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_DISCONNECT: {
            dev->obj.alert_cb(LTC4274_EVT_DISCONNECT, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_POWERGOOD: {
            dev->obj.alert_cb(LTC4274_EVT_POWERGOOD, dev->obj.cb_context);
        } break;
        case LTC4274_EVT_POWER_ENABLE: {
            dev->obj.alert_cb(LTC4274_EVT_POWER_ENABLE, dev->obj.cb_context);
        } break;
        default: {
            dev->obj.alert_cb(LTC4274_EVT_NONE, dev->obj.cb_context);
        }
    }
}
/*****************************************************************************
 *****************************************************************************/
void ltc4274_set_alert_handler(LTC4274_Dev *dev, LTC4274_CallbackFn alert_cb,
                               void *cb_context)
{
    dev->obj.alert_cb = alert_cb;
    dev->obj.cb_context = cb_context;
}

/******************************************************************************
 * @fn          ltc4274_clear_interrupt
 *
 * @brief       Read powerGood info and power event info.
 *
 * @args        A I2c device struct and address
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_clear_interrupt(const I2C_Dev *i2c_dev, uint8_t *pwrEvent,
                                     uint8_t *overCurrent, uint8_t *supply)
{
    ReturnStatus status = RETURN_OK;
    status = ltc4274_read(i2c_dev, LTC4274_REG_POWER_EVENT_COR, pwrEvent);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: Reading power good for PSE failed.\n");
    }

    /*Bit 4 for power good and bit 0 for power event*/
    LOGGER("PSELTC4274::INFO:: PSE power Good Info and Power ecent info is read with 0x%x.\n",
           *pwrEvent);

    /* if it is due to over current*/
    status = ltc4274_read(i2c_dev, LTC4274_REG_START_EVENT_COR, overCurrent);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR::Reading power good for PSE failed.\n");
    }

    LOGGER("PSELTC4274::INFO:: PSE power Good Info and Power ecent info is read with 0x%x.\n",
           *overCurrent);

    /* if its due to supply */
    status = ltc4274_read(i2c_dev, LTC4274_REG_SUPPLY_EVENT_COR, supply);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR::Reading power good for PSE failed.\n");
    }

    LOGGER("PSELTC4274::INFO:: PSE power Good Info and Power ecent info is read with 0x%x.\n",
           *supply);

    return status;
}

/******************************************************************************
 * @fn          ltc4274_get_interrupt_status
 *
 * @brief       Read active PSE interrupt info.
 *
 * @args        I2c device struct and Address
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_interrupt_status(const I2C_Dev *i2c_dev, uint8_t *val)
{
    ReturnStatus status = RETURN_OK;
    uint8_t interruptVal = 0;
    status = ltc4274_read(i2c_dev, LTC4274_REG_INTERRUPT_STATUS, &interruptVal);
    if (status != RETURN_OK) {
        LOGGER("PSELTC4274: ERROR:Reading power good for PSE failed.\n");
    }
    LOGGER("PSELTC4274::INFO: PSE interrupt state is 0x%x.\n", interruptVal);
    *val = interruptVal;
    return status;
}

/*********************************************************************************
 * @fn          ltc4274_debug_write
 *
 * @brief       debug PSE device.
 *
 * @args         I2c device struct, Address and value
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_debug_write(const I2C_Dev *i2c_dev, uint8_t reg_address,
                                 uint8_t value)
{
    ReturnStatus status = RETURN_OK;
    /*Enbale high power feature */
    status = ltc4274_write(i2c_dev, reg_address, value);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: Debug write command for reg 0x%x failed.\n",
               reg_address);
    }
    return status;
}

/*********************************************************************************
 * @fn          ltc4274_debug_read
 *
 * @brief       debug PSE device.
 *
 * @args         I2c device struct, Address and pointer to value
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_debug_read(const I2C_Dev *i2c_dev, uint8_t reg_address,
                                uint8_t *value)
{
    ReturnStatus status = RETURN_OK;
    /*Enbale high power feature */
    status = ltc4274_read(i2c_dev, reg_address, value);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: Debug read command for reg 0x%x failed.\n",
               reg_address);
    }
    return status;
}

/******************************************************************************
 * @fn          ltc4274_enable
 *
 * @brief       Enable .
 *WR
 * @args        On/off
 *
 * @return      void
 */
void ltc4274_enable(LTC4274_Dev *dev, uint8_t enableVal)
{
    if (enableVal) {
        OcGpio_write(&dev->cfg.reset_pin, false);
    } else {
        OcGpio_write(&dev->cfg.reset_pin, true);
    }
}

/******************************************************************************
 * @fn          ltc4274_get_devid
 *
 * @brief       Read PSE device id.
 *
 * @args         I2c device struct and Pointer to device Id.
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_get_devid(const I2C_Dev *i2c_dev, uint8_t *devID)
{
    ReturnStatus ret = RETURN_OK;
    ret = ltc4274_read(i2c_dev, LTC4274_REG_ID, devID);
    if (ret != RETURN_OK) {
        LOGGER("LTC4274:ERROR:: Reading Device Id for PSE failed.\n");
    }
    *devID = LTC4274_DEVID(*devID);
    return ret;
}

/*********************************************************************************
 * @fn          ltc4274_detect
 *
 * @brief       Reads the detectioin and classification register.
 *
 * @args         I2c device struct and Addrress
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_detect(const I2C_Dev *i2c_dev, uint8_t *detect,
                            uint8_t *val)
{
    ReturnStatus status = RETURN_OK;
    *val = 0;
    status = ltc4274_read(i2c_dev, LTC4274_REG_DETECT_EVENT, detect);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR::Reading PSE port detection failed.\n");
        return status;
    }
    LOGGER("PSELTC4274::Reading PSE port detection done.\n");

    status = ltc4274_read(i2c_dev, LTC4274_REG_STATUS, val);
    if (status != RETURN_OK) {
        LOGGER("LTC4274:ERROR::Reading PSE port classificatin failed.\n");
    }
    LOGGER("PSELTC4274::Reading PSE port classification is 0x%x.\n", *val);

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ltc4274_config
 **
 **    DESCRIPTION     : configure gpio and bring it out of reset .
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ePostCode
 **
 *****************************************************************************/
void ltc4274_config(LTC4274_Dev *dev)
{
    OcGpio_configure(&dev->cfg.reset_pin,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    //Enable PSE device.
    ltc4274_enable(dev, true);
}

/*****************************************************************************
 **    FUNCTION NAME   : ltc4274_probe
 **
 **    DESCRIPTION     : reset PSE device.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ePostCode
 **
 *****************************************************************************/
ePostCode ltc4274_probe(const LTC4274_Dev *dev, POSTData *postData)
{
    ePostCode postcode = POST_DEV_MISSING;
    uint8_t devId = 0x00;
    ReturnStatus status = RETURN_NOTOK;
    status = ltc4274_get_devid(&dev->cfg.i2c_dev, &devId);
    if (status != RETURN_OK) {
        postcode = POST_DEV_MISSING;
    } else if (devId == LTC4274_DEV_ID) {
        postcode = POST_DEV_FOUND;
    } else {
        postcode = POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, dev->cfg.i2c_dev.bus,
                         dev->cfg.i2c_dev.slave_addr, 0xFF, devId);
    return postcode;
}

/*****************************************************************************
 **    FUNCTION NAME   : ltc4274_reset
 **
 **    DESCRIPTION     : reset PSE device.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus ltc4274_reset(LTC4274_Dev *dev)
{
    ReturnStatus status = RETURN_OK;

    OcGpio_write(&dev->cfg.reset_pin, true);
    Task_sleep(100);
    OcGpio_write(&dev->cfg.reset_pin, false);

    return status;
}

/******************************************************************************
 * @fn          ltc4274_default_cfg
 *
 * @brief       configure PSE device.
 *
 * @args         I2c device struct and PSE device configurations.
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4274_default_cfg(const I2C_Dev *i2c_dev, uint8_t operatingMode,
                                 uint8_t detectEnable, uint8_t intrMask,
                                 uint8_t interruptEnable, uint8_t hpEnable)
{
    ReturnStatus ret = RETURN_OK;
    ret = ltc4274_set_cfg_operation_mode(i2c_dev, operatingMode);
    if (ret != RETURN_OK) {
        LOGGER("LTC4274::ERROR: PSE operational mode setting mode failed.\n");
        return ret;
    }

    ret = ltc4274_set_cfg_detect_enable(i2c_dev, detectEnable);
    if (ret != RETURN_OK) {
        LOGGER("LTC4274::ERROR: PSE detection and classification enable failed.\n");
        return ret;
    }

    ret = ltc4274_set_interrupt_mask(i2c_dev, intrMask);
    if (ret != RETURN_OK) {
        LOGGER("LTC4274::ERROR:PSE interrupts mask enable failed.\n");
        return ret;
    }

    ret = ltc4274_cfg_interrupt_enable(i2c_dev, interruptEnable);
    if (ret != RETURN_OK) {
        LOGGER("LTC4274::ERROR: PSE interrupt enable failed.\n");
        return ret;
    }

    ret = ltc4274_set_cfg_pshp_feature(i2c_dev, hpEnable);
    if (ret != RETURN_OK) {
        LOGGER("LTC4274::ERROR: PSE configured for LTEPOE++.\n");
        return ret;
    }
    return ret;
}

/*****************************************************************************
 **    FUNCTION NAME   : ltc4274_init
 **
 **    DESCRIPTION     : PSE Status is intialized.
 **
 **    ARGUMENTS       : None.
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
void ltc4274_init(LTC4274_Dev *dev)
{
    dev->obj = (LTC4274_Obj){};

    dev->obj.mutex = GateMutex_create(NULL, NULL);
    if (!dev->obj.mutex) {
        return;
    }

    ltc4274_initPSEStateInfo();

    if (dev->cfg.pin_evt) {
        const uint32_t pin_evt_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING;
        if (OcGpio_configure(dev->cfg.pin_evt, pin_evt_cfg) < OCGPIO_SUCCESS) {
            return;
        }

        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(dev->cfg.pin_evt, ltc4274_handle_irq, (void *)dev);
    }
}
/*****************************************************************************
 **    FUNCTION NAME   : ltc4274_initPSEStateInfo
 **
 **    DESCRIPTION     : PSE Status is intialized.
 **
 **    ARGUMENTS       : None.
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
void ltc4274_initPSEStateInfo(void)
{
    PSEStatus_Info.pseStatus.detectStatus = LTC4274_DETECT_UNKOWN;
    PSEStatus_Info.pseStatus.classStatus = LTC4274_CLASSTYPE_UNKOWN;
    PSEStatus_Info.pseStatus.powerGoodStatus = LTC4274_POWERGOOD_NOTOK;
    PSEStatus_Info.psealert = LTC4274_DISCONNECT_ALERT;
}

/*****************************************************************************
 **    FUNCTION NAME   : ltc4274_update_state
 **
 **    DESCRIPTION     : PSE Status to update.
 **
 **    ARGUMENTS       : I2c struct
 **
 **    RETURN TYPE     : void
 **
 *****************************************************************************/
void ltc4274_update_stateInfo(const I2C_Dev *i2c_dev)
{
    ReturnStatus status = RETURN_OK;
    status = ltc4274_get_powergood_status(
            i2c_dev, &PSEStatus_Info.pseStatus.powerGoodStatus);
    if (status != RETURN_OK) {
        LOGGER("PDLTC4275::ERROR: Power good signal read failed.\n");
        PSEStatus_Info.pseStatus.detectStatus = LTC4274_DETECT_UNKOWN;
        PSEStatus_Info.pseStatus.classStatus = LTC4274_CLASSTYPE_UNKOWN;
        PSEStatus_Info.psealert = LTC4274_DISCONNECT_ALERT;
        return;
    }
    if (PSEStatus_Info.pseStatus.powerGoodStatus == LTC4274_POWERGOOD) {
        status = ltc4274_get_detection_status(
                i2c_dev, &PSEStatus_Info.pseStatus.detectStatus);
        if (status != RETURN_OK) {
            LOGGER("PDLTC4275::ERROR: Reading PSE detection failed.\n");
            PSEStatus_Info.pseStatus.detectStatus = LTC4274_DETECT_UNKOWN;
            PSEStatus_Info.pseStatus.classStatus = LTC4274_CLASSTYPE_UNKOWN;
            PSEStatus_Info.psealert = LTC4274_DISCONNECT_ALERT;
            return;
        }
        status = ltc4274_get_class_status(
                i2c_dev, &PSEStatus_Info.pseStatus.classStatus);
        if (status != RETURN_OK) {
            LOGGER("PDLTC4275::ERROR: Reading PSE classification failed.\n");
            PSEStatus_Info.pseStatus.detectStatus = LTC4274_DETECT_UNKOWN;
            PSEStatus_Info.pseStatus.classStatus = LTC4274_CLASSTYPE_UNKOWN;
            PSEStatus_Info.psealert = LTC4274_DISCONNECT_ALERT;
            return;
        }
        status =
                ltc4274_get_interrupt_status(i2c_dev, &PSEStatus_Info.psealert);
        if (status != RETURN_OK) {
            LOGGER("PDLTC4275::ERROR: Reading PSE detection failed.\n");
            PSEStatus_Info.pseStatus.detectStatus = LTC4274_DETECT_UNKOWN;
            PSEStatus_Info.pseStatus.classStatus = LTC4274_CLASSTYPE_UNKOWN;
            PSEStatus_Info.psealert = LTC4274_DISCONNECT_ALERT;
            return;
        }
    }
}
