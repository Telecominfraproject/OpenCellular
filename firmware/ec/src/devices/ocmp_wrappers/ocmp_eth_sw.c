/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_eth_sw.h"

#include "common/inc/global/Framework.h"
#include "inc/devices/eth_sw.h"

bool ETHERNET_reset(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = eth_sw_set_config_soft_reset(PORT0);
    return status;
}

bool ETHERNET_enLoopBk(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = eth_sw_enable_loopback(driver, params);
    return status;
}

bool ETHERNET_disLoopBk(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = eth_sw_disable_loopback(driver, params);
    return status;
}

bool ETHERNET_enPktGen(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = eth_sw_enable_packet_gen(driver, params);
    return status;
}

bool ETHERNET_disPktGen(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = eth_sw_disable_packet_gen(driver);
    return status;
}

bool ETHERNET_tivaClient(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = eth_sw_config_tiva_client(driver, params);
    return status;
}

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    Eth_cfg *cfg = (Eth_cfg *)driver;
    switch (param_id) {
        case ETH_SW_STATUS_SPEED: {
            uint8_t *res = return_buf;
            return (eth_sw_get_status_speed(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_STATUS_DUPLEX: {
            uint8_t *res = return_buf;
            return (eth_sw_get_status_duplex(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_STATUS_AUTONEG_ON: {
            uint8_t *res = return_buf;
            return (eth_sw_get_status_auto_neg(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_STATUS_SLEEP_MODE_EN: {
            uint8_t *res = return_buf;
            return (eth_sw_get_status_sleep_mode(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_STATUS_AUTONEG_COMPLETE: {
            uint8_t *res = return_buf;
            return (eth_sw_get_status_auto_neg_complete(cfg->eth_sw_port,
                                                        res) == RETURN_OK);
        }
        case ETH_SW_STATUS_LINK: {
            uint8_t *res = return_buf;
            return (eth_sw_get_status_link_up(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        default:
            LOGGER_ERROR("ETH_SW::Unknown status param %d\n", param_id);
            return false;
    }
}

static bool _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    Eth_cfg *cfg = (Eth_cfg *)driver;
    switch (param_id) {
        case ETH_SW_CONFIG_SPEED: {
            uint8_t *res = return_buf;
            return (eth_sw_get_config_speed(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_DUPLEX: {
            uint8_t *res = return_buf;
            return (eth_sw_get_config_duplex(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_POWER_DOWN: {
            uint8_t *res = return_buf;
            return (eth_sw_get_config_power_down(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_SLEEPMODE_EN: {
            uint8_t *res = return_buf;
            return (eth_sw_get_config_sleep_mode(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_INTERRUPT_EN: {
            uint8_t *res = return_buf;
            return (eth_sw_get_config_interrupt_enable(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_SW_RESET: {
            uint8_t *res = return_buf;
            *res = 0;
            return false;
        }
        case ETH_SW_CONFIG_RESTART_AUTONEG: {
            uint8_t *res = return_buf;
            *res = 0;
            return false;
        }
        default:
            LOGGER_ERROR("ETH_SW::Unknown config param %d\n", param_id);
            return false;
    }
}

static bool _set_config(void *driver, unsigned int param_id, const void *data)
{
    Eth_cfg *cfg = (Eth_cfg *)driver;
    switch (param_id) {
        case ETH_SW_CONFIG_SPEED: {
            uint8_t *res = (uint8_t *)data;
            return (eth_sw_set_config_speed(cfg->eth_sw_port, *res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_DUPLEX: {
            uint8_t *res = (uint8_t *)data;
            return (eth_sw_set_config_duplex(cfg->eth_sw_port, *res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_POWER_DOWN: {
            uint8_t *res = (uint8_t *)data;
            return (eth_sw_set_config_power_down(cfg->eth_sw_port, *res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_SLEEPMODE_EN: {
            uint8_t *res = (uint8_t *)data;
            return (eth_sw_set_config_sleep_mode_enable(cfg->eth_sw_port,
                                                        *res) == RETURN_OK);
        }
        case ETH_SW_CONFIG_INTERRUPT_EN: {
            uint8_t *res = (uint8_t *)data;
            return (eth_sw_set_config_interrupt_enable(cfg->eth_sw_port, res) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_SW_RESET: {
            return (eth_sw_set_config_soft_reset(cfg->eth_sw_port) ==
                    RETURN_OK);
        }
        case ETH_SW_CONFIG_RESTART_AUTONEG: {
            return (eth_sw_set_config_restart_neg(cfg->eth_sw_port) ==
                    RETURN_OK);
        }
        default:
            LOGGER_ERROR("ETH_SW::Unknown config param %d\n", param_id);
            return false;
    }
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    eth_sw_configure(driver);
    return eth_sw_probe(postData);
}

static void _alert_handler(Eth_Sw_Events evt, int16_t value, void *alert_data)
{
    unsigned int alert;
    switch (evt) {
        case ETH_EVT_SPEED:
            alert = ETH_ALERT_SPEED_CHANGE;
            break;
        case ETH_EVT_DUPLEX:
            alert = ETH_ALERT_DUPLEX_CHANGE;
            break;
        case ETH_EVT_AUTONEG:
            alert = ETH_ALERT_AUTONEG_DONE;
            break;
        case ETH_EVT_LINK:
            alert = ETH_ALERT_LINK_CHANGE;
            break;
        case ETH_EVT_CROSSOVER:
            alert = ETH_ALERT_CROSSOVER_DET;
            break;
        case ETH_EVT_ENERGY:
            alert = ETH_ALERT_ENERGY_DET;
        case ETH_EVT_POLARITY:
            alert = ETH_ALERT_POLARITY_DET;
        case ETH_EVT_JABBER:
            alert = ETH_ALERT_JABBER_DET;
            break;
        default:
            LOGGER_ERROR("ETH_SW::Unknown Ethernet Switch evt: %d\n", evt);
            return;
    }

    OCMP_GenerateAlert(alert_data, alert, &value);
    LOGGER_DEBUG("ETH_SW:: Event: %d Value: %d\n", evt, value);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    ePostCode ret = POST_DEV_CFG_FAIL;
    ret = eth_sw_init(driver);
    //TODO: Enabling of the ethernet interrupts requires soem more work.
    /*
    eth_sw_setAlertHandler(driver,_alert_handler,(void *)alert_token);
    eth_enable_interrupt();*/
    return ret;
}

const Driver_fxnTable eth_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,           .cb_init = _init,
    .cb_get_status = _get_status, .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
