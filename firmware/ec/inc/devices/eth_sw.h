/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef ETH_SW_H_
#define ETH_SW_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"
#include "inc/common/post.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define ETH_SW_PRODUCT_ID   0x9567
#define PHY_IDENTIFIER      0x00221631

#define DEFAULT_PHY_INTS  (PHY_4_INT_EN | PHY_3_INT_EN | PHY_2_INT_EN |PHY_1_INT_EN | PHY_0_INT_EN)
#define DEFUALT_INT       (LINK_CHANGE_INT_EN)
/*
 * MDC MDIO definitions
 */
#define NO_CPU_MODE                 1
#define CPU_MODE                    0

//#define ETH_SW_ADDR
#define MDIO_PORT                   GPIO_PORTC_BASE
#define MDC_PIN                     GPIO_PIN_6
#define MDIO_PIN                    GPIO_PIN_7

#define LAN_MUX_SELECT_PORT         GPIO_PORTN_BASE
#define LAN_MUX_SELECT_PIN          GPIO_PIN_1

#define ETH_SW_DEV_SERIAL_NO        1

/*
 * Ethernet Components ID. This is the part of the OCMPMsg in componentID field.
 */
typedef enum {
    ETH_COMP_ALL    = 0x0,
    PORT_0,     // PORT# 0
    PORT_1,
    PORT_2,
    PORT_3,
    PORT_4,
    ETH_COMPONENT_MAX           // Limiter
} e_ethernet_component_ID;

typedef enum {
    PORT1 = 1,
    PORT2,
    PORT3,
    PORT4,
	PORT5,
	PORT6,
	PORT7,
}Eth_Sw_Port;

typedef enum {
    SPEED_10M = 0,
    SPEED_100M,
	SPEED_1000M,
    SPEED_AUTONEG
} port_speed;

typedef enum {
    HALF_DUPLEX = 0,
    FULL_DUPLEX,
    DUPLEX_AUTONEG
} port_duplex;

typedef enum Eth_Sw_Status {
    ETH_SW_STATUS_SPEED = 0x00,
    ETH_SW_STATUS_DUPLEX,
    ETH_SW_STATUS_AUTONEG_ON,
	ETH_SW_STATUS_POWER_DOWN_MODE_EN,
    ETH_SW_STATUS_AUTONEG_COMPLETE,
    ETH_SW_STATUS_LINK,
    ETH_SW_STATUS_MAX
} Eth_Sw_Status;

typedef enum Eth_Sw_Config {
    ETH_SW_CONFIG_SPEED = 0x00,
    ETH_SW_CONFIG_DUPLEX,
    ETH_SW_CONFIG_POWER_DOWN,
    ETH_SW_CONFIG_INTERRUPT_EN,
    ETH_SW_CONFIG_SW_RESET,
    ETH_SW_CONFIG_RESTART_AUTONEG,
    ETH_SW_CONFIG_SW_MAX_PARAM
} Eth_Sw_Config;

typedef enum Eth_Sw_Alert {
    ETH_ALERT_LINK_UP = 0x00,
    ETH_ALERT_REMOTE_FAULT = 0x01,
    ETH_ALERT_LINK_DOWN = 0x02,
    ETH_ALERT_PARALLEL_FAULT = 0x04,
    ETH_ALERT_RECEIVE_ERROR = 0x06,
    ETH_ALERT_JABBER_DET=0x07
} Eth_Sw_Alert;

typedef enum {
    ETH_EVT_LINKUP =0x00,
    ETH_EVT_REMOTEFAULT,
    ETH_EVT_LINKDOWN,
    ETH_EVT_PARALLELFAULT,
    ETH_EVT_RECEIVE_ERROR,
    ETH_EVT_JABBER,
}Eth_Sw_Events;


typedef void (*Eth_Sw_CallbackFn) (Eth_Sw_Events evt, int16_t value,
                                   void *context);

typedef struct Eth_Sw_Obj {
    Eth_Sw_CallbackFn alert_cb;
    void *cb_context;
} Eth_Sw_Obj;

typedef struct Eth_Sw_Dev {
    Eth_Sw_Obj obj;
} Eth_Sw_Dev;

typedef struct Eth_Sw_Cfg {
    OcGpio_Pin* pin_evt;
    Eth_Sw_Dev eth_switch;
    OcGpio_Pin pin_ec_ethsw_reset;
    I2C_Dev dev_cfg;
} Eth_Sw_Cfg;

/* Schema config - pass driver instance + port # */
typedef struct Eth_cfg {
    Eth_Sw_Cfg *eth_sw_cfg;
    Eth_Sw_Port eth_sw_port;
} Eth_cfg;

ePostCode eth_sw_probe(Eth_cfg *dev, POSTData *postData);
ePostCode eth_sw_init();
void eth_enable_interrupt(Eth_cfg *dev);
ReturnStatus get_interrupt(Eth_cfg *dev, uint8_t port);
ReturnStatus eth_sw_get_status_speed(Eth_cfg *dev, uint8_t port, port_speed *speed);
ReturnStatus eth_sw_get_status_duplex(Eth_cfg *dev,uint8_t port, port_duplex *duplex);
ReturnStatus eth_sw_get_status_auto_neg(Eth_cfg *dev, uint8_t port, port_duplex *autoneg_on);
ReturnStatus eth_sw_get_status_power_down_mode(Eth_cfg *dev, uint8_t port, port_duplex *sleep_mode_en);
ReturnStatus eth_sw_get_status_auto_neg_complete(Eth_cfg *dev, uint8_t port, port_duplex *autoneg_complete);
ReturnStatus eth_sw_get_status_link_up(Eth_cfg *dev, uint8_t port, port_duplex *link_up);
ReturnStatus restart_autoneg(Eth_cfg *dev, uint8_t port);
ReturnStatus eth_sw_set_config_speed(Eth_cfg *dev, uint8_t port, port_speed speed);
ReturnStatus eth_sw_set_config_duplex(Eth_cfg *dev, uint8_t port, port_duplex duplex);
ReturnStatus eth_sw_set_config_power_down(Eth_cfg *dev, uint8_t port,uint8_t power_down);
ReturnStatus eth_sw_set_config_sleep_mode_enable(Eth_cfg *dev, uint8_t port,uint8_t sleep_mode_en);
ReturnStatus eth_sw_set_config_restart_neg(Eth_cfg *dev, uint8_t port);
ReturnStatus eth_sw_set_config_interrupt_enable(Eth_cfg *dev, uint8_t port, uint8_t *interrupt_mask);
ReturnStatus eth_sw_set_config_soft_reset(Eth_cfg *dev);
ReturnStatus eth_sw_get_config_speed(Eth_cfg *dev, uint8_t port, port_speed* speed);
ReturnStatus eth_sw_get_config_duplex(Eth_cfg *dev, uint8_t port, port_duplex* duplex);
ReturnStatus eth_sw_get_config_power_down(Eth_cfg *dev, uint8_t port, uint8_t* power_dwn);
ReturnStatus eth_sw_get_config_sleep_mode(Eth_cfg *dev, uint8_t port, uint8_t* sleep_mode);
ReturnStatus eth_sw_get_config_interrupt_enable(Eth_cfg *dev, uint8_t port, uint8_t* interrupt_enb);
void eth_sw_setAlertHandler(Eth_cfg *ethCfg, Eth_Sw_CallbackFn alert_cb, void *cb_context);

#endif /* INC_DEVICES_ETH_SW_H_ */
