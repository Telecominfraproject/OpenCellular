/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
// Standard Header files
//*****************************************************************************
#include "inc/devices/eth_sw.h"

#include "inc/common/i2cbus.h"
#include "inc/devices/KSZ9567_registers.h"
#include "inc/common/global_header.h"
#include "inc/devices/mdio_bb.h"
#include "registry/SSRegistry.h"
#include "inc/common/byteorder.h"

#define GLOBAL_CHIP_ID_REG1 0X0001
#define GLOBAL_CHIP_ID_REG2 0X0002
#define GLOBAL_CHIP_ID_REG3 0X0003
#define PHY_ID_HIGH_REG 0X1104
#define PHY_ID_LOW_REG 0X1106

#define CLEAR_BIT(x, y) (y = (~x) & y)
#define SET_BIT(x, y) (y = x | y)
static bool s_eth_sw_linkup = false;

/*****************************************************************************
 **    FUNCTION NAME   : read_ksz9567_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from KSZ9567 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus read_ksz9567_reg(const Eth_cfg *dev, uint16_t regAddress,
                                     uint32_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle kszHandle = i2c_get_handle(dev->eth_sw_cfg->dev_cfg.bus);
    if (!kszHandle) {
        LOGGER_ERROR(
            "KSZSWITCH:ERROR:: Failed to get I2C Bus for KSZ9567 Switch "
            "0x%x on bus 0x%x.\n",
            dev->eth_sw_cfg->dev_cfg.slave_addr, dev->eth_sw_cfg->dev_cfg.bus);
    } else {
        status = i2c_reg_read_16bit_address(kszHandle,
                                            dev->eth_sw_cfg->dev_cfg.slave_addr,
                                            regAddress, regValue, 2);
        *regValue = betoh16(*regValue);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : write_ksz9567_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from KSZ9567 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus write_ksz9567_reg(const Eth_cfg *dev, uint16_t regAddress,
                                      uint32_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle kszHandle = i2c_get_handle(dev->eth_sw_cfg->dev_cfg.bus);
    if (!kszHandle) {
        LOGGER_ERROR(
            "KSZSWITCH:ERROR:: Failed to get I2C Bus for KSZ9567 Switch "
            "0x%x on bus 0x%x.\n",
            dev->eth_sw_cfg->dev_cfg.slave_addr, dev->eth_sw_cfg->dev_cfg.bus);
    } else {
        status = i2c_reg_write_16bit_address(
            kszHandle, dev->eth_sw_cfg->dev_cfg.slave_addr, regAddress,
            regValue, 2);
    }
    return status;
}

void eth_sw_configure(Eth_cfg *ethCfg)
{
    uint8_t link_up;
    uint16_t read_val = 0;
    if (!s_eth_sw_linkup) {
        OcGpio_configure(&ethCfg->eth_sw_cfg->pin_ec_ethsw_reset,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
        SysCtlDelay(16000000); // 400ms delay
    }
}

ePostCode eth_sw_probe(Eth_cfg *dev, POSTData *postData)
{
    ePostCode eth_sw_found = POST_DEV_MISSING;
    uint16_t switch_cid = 0;
    uint32_t devId = 0;
    /*Switch idenifier*/
    uint32_t Value1 = 0;
    uint32_t Value2 = 0;
    if (read_ksz9567_reg(dev, GLOBAL_CHIP_ID_REG1, &Value1) == RETURN_OK) {
        switch_cid = Value1;
        if (switch_cid == ETH_SW_PRODUCT_ID) {
            /* Phy Identifier */
            read_ksz9567_reg(dev, PHY_ID_HIGH_REG, &Value1);
            read_ksz9567_reg(dev, PHY_ID_LOW_REG, &Value2);
            devId = (Value1 << 16) | Value2;
            if (devId == PHY_IDENTIFIER) {
                eth_sw_found = POST_DEV_FOUND;
            }
        }
    }
    post_update_POSTData(postData, 0xFF, 0xFF, 0xFF, devId);
    LOGGER_DEBUG("ETHSW::INFO:: Ethernet switch  %s.\n",
                 ((eth_sw_found == POST_DEV_FOUND) ? "found" : "not found"));
    return eth_sw_found;
}

void eth_enable_interrupt(Eth_cfg *dev)
{
    if (write_ksz9567_reg(dev, GLOBAL_PORT_INT_MASK_REG, PORT_INT_EN) ==
        RETURN_OK)
        ;
}

uint8_t get_interrupt_port(Eth_cfg *dev)
{
    uint32_t value = 0;
    uint8_t port = 0;
    if (read_ksz9567_reg(dev, GLOBAL_PORT_INT_STATUS_REG, value) == RETURN_OK) {
        if (value & 0x7F) {
            while (!((value >> port) & 1)) {
                port++;
            }
        }
    }
    return port;
}

uint16_t get_interrupt_status(Eth_cfg *dev, uint8_t port)
{
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PORT_INT_CONTROL_REG | (port << 12);
    if (read_ksz9567_reg(dev, write_reg, &value) == RETURN_OK)
        return value;
}
/*****************************************************************************
 * Internal IRQ handler - reads in triggered interrupts and dispatches CBs
 *****************************************************************************/
static void _ethernet_sw_isr(void *context)
{
    Eth_cfg *ethCfg = context;
    uint8_t port = 0;
    uint8_t int_port = 0;
    uint32_t write_reg = 0;
    uint16_t interrupt_status = 0;
    uint32_t reg_value = 0;
    uint8_t value = 0;
    if (!ethCfg->eth_sw_cfg->eth_switch.obj.alert_cb) {
        return;
    }
    /* Confirm the interrupt*/
    int_port = get_interrupt_port(ethCfg);
    LOGGER_DEBUG(
        "ETHSW::INFO:: Ethernet switch context report interrupt from 0x%x.\n",
        int_port);
    uint16_t i = 0;
    Eth_Sw_Events eth_Evt;
    if (interrupt_status = get_interrupt_status(ethCfg, int_port)) {
        for (i = 0; (1 << i) != 0x40; i++) {
            switch (interrupt_status & (1 << i)) {
                case LINK_UP_INT_STATUS: {
                    uint8_t link_status = 0;
                    write_reg = PHY_BASIC_STATUS_REG | int_port << 12;
                    read_ksz9567_reg(ethCfg, write_reg, &reg_value);
                    link_status = (reg_value & LINK_UP_STATUS) ? 1 : 0;
                    if (link_status == 1) {
                        eth_Evt = ETH_EVT_LINKUP;
                        value = link_status;
                    }
                } break;
                case LINK_DOWN_INT_STATUS: {
                    uint8_t link_status = 0;
                    write_reg = PHY_BASIC_STATUS_REG | int_port << 12;
                    read_ksz9567_reg(ethCfg, write_reg, &reg_value);
                    link_status = (reg_value & LINK_UP_STATUS) ? 1 : 0;
                    if (link_status == 0) {
                        eth_Evt = ETH_EVT_LINKDOWN;
                        value = link_status;
                    }
                } break;
                case REMOTE_FAULT_INT_STATUS: {
                    uint8_t rem_fault_status = 0;
                    write_reg = PORT_INT_CONTROL_REG | int_port << 12;
                    read_ksz9567_reg(ethCfg, write_reg, &reg_value);
                    rem_fault_status =
                        (reg_value & PORT_REM_FAULT_STATUS) ? 1 : 0;
                    if (rem_fault_status == 1) {
                        eth_Evt = ETH_EVT_REMOTEFAULT;
                        value = rem_fault_status;
                    }
                } break;
                case JABBER_INT_STATUS: {
                    uint8_t jabber_status = 0;
                    write_reg = PHY_BASIC_STATUS_REG | int_port << 12;
                    read_ksz9567_reg(ethCfg, write_reg, &reg_value);
                    jabber_status = (reg_value & PORT_JABBER_STATUS) ? 1 : 0;
                    if (jabber_status == 1) {
                        eth_Evt = ETH_EVT_JABBER;
                        value = jabber_status;
                    }
                } break;
                case RECEIVE_ERROR_INT_STATUS: {
                    eth_Evt = ETH_EVT_RECEIVE_ERROR;
                    value = 1;
                } break;
                case PR_DET_FAULT_INT_STATUS: {
                    uint8_t para_fault_status = 0;
                    write_reg = PHY_AUTO_NEG_EXP_STATUS_REG | int_port << 12;
                    read_ksz9567_reg(ethCfg, write_reg, &reg_value);
                    para_fault_status =
                        (reg_value & PORT_PARA_FALT_STATUS) ? 1 : 0;
                    if (para_fault_status == 1) {
                        eth_Evt = ETH_EVT_PARALLELFAULT;
                        value = para_fault_status;
                    }
                } break;
                default: {
                    LOGGER_ERROR("ETHSW:Unknown event type\n");
                    return;
                }
            }
            ethCfg->eth_sw_cfg->eth_switch.obj.alert_cb(
                eth_Evt, value, ethCfg->eth_sw_cfg->eth_switch.obj.cb_context);
        }
    }
}

/*****************************************************************************
 *****************************************************************************/
void eth_sw_setAlertHandler(Eth_cfg *ethCfg, Eth_Sw_CallbackFn alert_cb,
                            void *cb_context)
{
    ethCfg->eth_sw_cfg->eth_switch.obj.alert_cb = alert_cb;
    ethCfg->eth_sw_cfg->eth_switch.obj.cb_context = cb_context;
}

ePostCode eth_sw_init(Eth_cfg *dev)
{
    ePostCode ret = POST_DEV_CFG_DONE;

    return ret;
}

ReturnStatus eth_sw_get_status_speed(Eth_cfg *dev, uint8_t port,
                                     port_speed *speed)
{
    ReturnStatus status = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PORT_STATUS_REG | port << 12;
    status = read_ksz9567_reg(dev, write_reg, &value);
    value = ((value >> 8) & 0xFF) & SPEED_STATUS;
    *speed = value ? ((value < 0x10) ? SPEED_100M : SPEED_1000M) : SPEED_10M;
    return status;
}

ReturnStatus eth_sw_get_status_duplex(Eth_cfg *dev, uint8_t port,
                                      port_duplex *duplex)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PORT_STATUS_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    value = ((value >> 8) & 0xFF) & DUPLEX_STATUS;
    *duplex = value ? FULL_DUPLEX : HALF_DUPLEX;
    return ret;
}

ReturnStatus eth_sw_get_status_auto_neg(Eth_cfg *dev, uint8_t port,
                                        uint8_t *autoneg_on)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    *autoneg_on = (value & AUTO_NEGO_STATUS) ? 1 : 0;
    return ret;
}

ReturnStatus eth_sw_get_status_power_down_mode(Eth_cfg *dev, uint8_t port,
                                               uint8_t *power_down_en)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    *power_down_en = (value & PWR_DWN_STATUS) ? 1 : 0;
    return ret;
}

ReturnStatus eth_sw_get_status_auto_neg_complete(Eth_cfg *dev, uint8_t port,
                                                 uint8_t *autoneg_complete)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_STATUS_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    *autoneg_complete = (value & AUTO_NEGO_COM_STATUS) ? 1 : 0;
    return ret;
}

ReturnStatus eth_sw_get_status_link_up(Eth_cfg *dev, uint8_t port,
                                       uint8_t *link_up)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_STATUS_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    *link_up = (value & LINK_UP_STATUS) ? 1 : 0;
    return ret;
}

/* restart_autoneg - This function re-initiated autonegotiation */
ReturnStatus restart_autoneg(Eth_cfg *dev, uint8_t port)
{
    ReturnStatus ret = RETURN_OK;

    return ret;
}

ReturnStatus eth_sw_set_config_speed(Eth_cfg *dev, uint8_t port,
                                     port_speed speed)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t value = 0;
    uint16_t write_reg = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;

    read_ksz9567_reg(dev, write_reg, &value);
    value = value & 0xEFFF;
    write_ksz9567_reg(dev, write_reg, value);

    switch (speed) {
        case SPEED_10M:
            read_ksz9567_reg(dev, write_reg, &value);
            value = value & 0xDFBF;
            write_ksz9567_reg(dev, write_reg, value);
            break;
        case SPEED_100M:
            read_ksz9567_reg(dev, write_reg, &value);
            value = (value | 0x0040) & (0x2FFF);
            write_ksz9567_reg(dev, write_reg, value);
            break;
        case SPEED_1000M:
            read_ksz9567_reg(dev, write_reg, &value);
            value = value | 0x2000;
            write_ksz9567_reg(dev, write_reg, value);
            break;
        case SPEED_AUTONEG:
            restart_autoneg(dev, port);
            break;
        default:
            DEBUG("Invalid Ethernet speed set option");
            return RETURN_NOTOK;
    }
    return ret;
}

ReturnStatus eth_sw_set_config_duplex(Eth_cfg *dev, uint8_t port,
                                      port_duplex duplex)
{
    ReturnStatus ret = RETURN_OK;
    int write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    read_ksz9567_reg(dev, write_reg, &value);
    value = value & 0xEFFF;
    write_ksz9567_reg(dev, write_reg, value);
    switch (duplex) {
        case HALF_DUPLEX:
            read_ksz9567_reg(dev, write_reg, &value);
            value = value & 0xFEFF;
            write_ksz9567_reg(dev, write_reg, value);
            break;
        case FULL_DUPLEX:
            read_ksz9567_reg(dev, write_reg, &value);
            value = value | 0x0100;
            write_ksz9567_reg(dev, write_reg, value);
            break;
        case DUPLEX_AUTONEG:
            restart_autoneg(dev, port);
            break;
        default:
            DEBUG("Invalid Ethernet speed set option");
            return RETURN_NOTOK;
    }
    return ret;
}

ReturnStatus eth_sw_set_config_power_down(Eth_cfg *dev, uint8_t port,
                                          uint8_t power_down)
{
    ReturnStatus ret = RETURN_OK;
    int write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    if (read_ksz9567_reg(dev, write_reg, &value) == RETURN_OK) {
        value = value | PWR_DWN_SET;
        ret = write_ksz9567_reg(dev, write_reg, value);
    }
    return ret;
}

ReturnStatus eth_sw_set_config_soft_reset(Eth_cfg *dev)
{
    ReturnStatus ret = RETURN_NOTOK;
    // OcGpio_configure(&dev->eth_sw_cfg->pin_ec_ethsw_reset,
    // OCGPIO_CFG_OUTPUT); OcGpio_write(&dev->eth_sw_cfg->pin_ec_ethsw_reset,
    // false); SysCtlDelay(16000000); //400ms delay
    // OcGpio_write(&dev->eth_sw_cfg->pin_ec_ethsw_reset, ture);
    // return ret;

    uint32_t value = 0x01; /* Global Software Reset */
    ret = write_ksz9567_reg(dev, GLOBAL_CHIP_ID_REG3, value);
    return ret;
}

ReturnStatus eth_sw_set_config_restart_neg(Eth_cfg *dev, uint8_t port)
{
    ReturnStatus ret = RETURN_OK;
    int write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    if (read_ksz9567_reg(dev, write_reg, &value) == RETURN_OK) {
        value = value | RESART_AUTO_NEGO;
        ret = write_ksz9567_reg(dev, write_reg, value);
    }
    return ret;
}

ReturnStatus eth_sw_get_config_speed(Eth_cfg *dev, uint8_t port,
                                     port_speed *speed)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    value = value & PORT_SPEED_SET;
    *speed = value ? ((value < 0x2000) ? SPEED_100M : SPEED_1000M) : SPEED_10M;
    return ret;
}

ReturnStatus eth_sw_get_config_duplex(Eth_cfg *dev, uint8_t port,
                                      port_duplex *duplex)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    value = value & PORT_DUPLEX_SET;
    *duplex = value ? FULL_DUPLEX : HALF_DUPLEX;
    return ret;
}

ReturnStatus eth_sw_get_config_power_down(Eth_cfg *dev, uint8_t port,
                                          uint8_t *power_dwn)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = PHY_BASIC_CONTROL_REG | port << 12;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    *power_dwn = (value & PORT_PWR_DWN_SET) ? 1 : 0;
    return ret;
}

ReturnStatus eth_sw_get_config_interrupt_enable(Eth_cfg *dev, uint8_t port,
                                                uint8_t *interrupt_enb)
{
    ReturnStatus ret = RETURN_OK;
    uint32_t write_reg = 0;
    uint32_t value = 0;
    write_reg = GLOBAL_PORT_INT_MASK_REG;
    ret = read_ksz9567_reg(dev, write_reg, &value);
    *interrupt_enb = value & (1 << (port - 1));
    return ret;
}
