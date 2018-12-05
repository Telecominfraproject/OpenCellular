/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef __OC_CONNECT1_H
#define __OC_CONNECT1_H

#ifdef __cplusplus
extern "C" {
#endif

#define OC_PMIC_ENABLE          (1)
#define OC_PMIC_DISABLE         (0)
#define OC_SDR_ENABLE           (1)
#define OC_SDR_DISABLE          (0)
#define OC_SDR_FE_IO_ENABLE     (1)
#define OC_SDR_FE_IO_DISABLE    (0)
#define OC_FE_ENABLE            (1)
#define OC_FE_DISABLE           (0)
#define OC_PWR_LION_BATT        (1)
#define OC_PWR_LEAD_BATT        (0)
#define OC_PWR_PSE_RESET_STATE  (1)
#define OC_PWR_PSE_ON_STATE     (0)
#define OC_GBC_PROC_ENABLE      (1)
#define OC_GBC_PROC_RESET       (0)
#define OC_SYNC_IOEXP_ENABLE    (1)
#define OC_SYNC_IOEXP_RESET     (0)
#define OC_HCI_LED_ENABLE       (1)
#define OC_HCI_LED_DISABLE      (0)
#define OC_ETH_SW_ENABLE        (1)
#define OC_ETH_SW_DISABLE       (0)
#define CAT24C256 { .page_size = 64, .mem_size = (256 / 8) }

/* GBC IO expander Slave address */
#define BIGBROTHER_IOEXP0_ADDRESS           0x71
#define BIGBROTHER_IOEXP1_ADDRESS           0x70
/* SYNC IO expander Slave address */
#define SYNC_IO_DEVICE_ADDR                 0x71
/* SDR IO expander Slave address */
#define SDR_FX3_IOEXP_ADDRESS               0x1E
/* RFFE IO expander Slave address */
#define RFFE_CHANNEL1_IO_TX_ATTEN_ADDR      0x18
#define RFFE_CHANNEL1_IO_RX_ATTEN_ADDR      0x1A
#define RFFE_CHANNEL2_IO_TX_ATTEN_ADDR      0x1C
#define RFFE_CHANNEL2_IO_RX_ATTEN_ADDR      0x1D
#define RFFE_IO_REVPOWER_ALERT_ADDR         0x1B

/*!
 *  @def    OC_CONNECT1_EMACName
 *  @brief  Enum of EMAC names on the OC_CONNECT1 board
 */
typedef enum OC_CONNECT1_EMACName {
    OC_CONNECT1_EMAC0 = 0,

    OC_CONNECT1_EMACCOUNT
} OC_CONNECT1_EMACName;

/*!
 *  @def    OC_CONNECT1_GPIOName
 *  @brief  Enum of GPIO names on the OC_CONNECT1 board
 */
typedef enum OC_EC_PORTGroupName {
    PA = 1,
    PB,
    PC,
    PD,
    PE,
    PF,
    PG
}OC_EC_PORTGroupName;

typedef enum OC_CONNECT1_GPIOName {
    //PA
    OC_EC_PSE_INT = 0,
    OC_EC_nPSE_RESET,
    OC_EC_WD_INPUT,
    OC_EC_ENABLE_OC_INPUT,
    OC_EC_POWER_OFF,
    OC_EC_NOC_PA5,
    OC_EC_CS_I2C1_SCLK,
    OC_EC_CS_I2C1_SDA,
    //PB
    OC_EC_DISABLE_DC_INPUT = 8,
    OC_EC_IOEXP_INT,
    OC_EC_CS_I2C0_SCLK,
    OC_EC_CS_I2C0_SDA,
    OC_EC_ENABLE_PASSIVE_POE,
    OC_EC_CS_ALERT_24V_20V,
    OC_EC_TEMP_I2C5_SCL,
    OC_EC_TEMP_I2C5_SDA,
    //PC
    OC_EC_JTAG_TCK = 16,
    OC_EC_JTAG_TMS,
    OC_EC_JTAG_TDI,
    OC_EC_JTAG_TDO,
    OC_EC_UART_RX,
    OC_EC_EN_INT_BATT_PWR,
    OC_EC_DC_INPUT_FAULT,
    OC_EC_UART_TX,
    //PD
    OC_EC_CS_ALERT = 24,
    OC_EC_DC_IN_PRESENT,
    OC_EC_CS_ALERT_12V_GBC,
    OC_EC_CS_ALERT_12V_BB,
    OC_EC_CS_ALERT_12V_FE,
    OC_EC_PGOOD_5V0,
    OC_EC_PGOOD_12V0,
    OC_NOC_PD7,
    //PE
    OC_EC_IVINMON = 32,
    OC_EC_ISMON,//OC_CONNECT1_GBC_TEMP_ALERT2,
    OC_EC_CHARGER_ALERT,
    OC_EC_PGOOD_BOOST_CONV_BATT,
    OC_EC_CHARGER_I2C2_SCL,
    OC_EC_CHARGER_I2C2_SDA,
    //PF
    OC_EC_TIVA_GPIO1 = 40,
    OC_EC_TIVA_GPIO2,
    OC_EC_BUZZER_ON,
    OC_EC_PD_T2P,
    OC_EC_TEMP_EVENT,
    //PG
    OC_EC_PB_PSE_I2C3_SCL = 48,
    OC_EC_PB_PSE_I2C3_SDA,
    OC_EC_CS_I2C4_SCL,
    OC_EC_CS_I2C4_SDA,
    OC_EC_OC_IN_PRESENT,
    OC_EC_POE_IN_PRESENT,
    OC_EC_GPIOCOUNT
} OC_CONNECT1_GPIOName;

/*!
 *  @def    OC_CONNECT1_I2CName
 *  @brief  Enum of I2C names on the OC_CONNECT1 board
 */
typedef enum OC_CONNECT1_I2CName {
    OC_CONNECT1_I2C0 = 0,
    OC_CONNECT1_I2C1,
    OC_CONNECT1_I2C2,
    OC_CONNECT1_I2C3,
    OC_CONNECT1_I2C4,
    OC_CONNECT1_I2C5,
    OC_CONNECT1_I2CCOUNT
} OC_CONNECT1_I2CName;

/*!
 *  @def    OC_CONNECT1_UARTName
 *  @brief  Enum of UARTs on the OC_CONNECT1 board
 */
typedef enum OC_CONNECT1_UARTName {
    OC_CONNECT1_UART3,
    OC_CONNECT1_UART4,
    OC_CONNECT1_UARTCOUNT
} OC_CONNECT1_UARTName;

/*
 *  @def    OC_CONNECT1_WatchdogName
 *  @brief  Enum of Watchdogs on the OC_CONNECT1 board
 */
typedef enum OC_CONNECT1_WatchdogName {
    OC_CONNECT1_WATCHDOG0 = 0,
    OC_CONNECT1_WATCHDOGCOUNT
} OC_CONNECT1_WatchdogName;

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 *  This includes:
 *     - Enable clock sources for peripherals
 */
void OC_CONNECT1_initGeneral(void);

/*!
 *  @brief Initialize board specific EMAC settings
 *
 *  This function initializes the board specific EMAC settings and
 *  then calls the EMAC_init API to initialize the EMAC module.
 *
 *  The EMAC address is programmed as part of this call.
 *
 */
void OC_CONNECT1_initEMAC(void);

/*!
 *  @brief  Initialize board specific GPIO settings
 *
 *  This function initializes the board specific GPIO settings and
 *  then calls the GPIO_init API to initialize the GPIO module.
 *
 *  The GPIOs controlled by the GPIO module are determined by the GPIO_PinConfig
 *  variable.
 */
void OC_CONNECT1_initGPIO(void);

/*!
 *  @brief  Initialize board specific I2C settings
 *
 *  This function initializes the board specific I2C settings and then calls
 *  the I2C_init API to initialize the I2C module.
 *
 *  The I2C peripherals controlled by the I2C module are determined by the
 *  I2C_config variable.
 */
void OC_CONNECT1_initI2C(void);

/*!
 *  @brief  Initialize board specific UART settings
 *
 *  This function initializes the board specific UART settings and then calls
 *  the UART_init API to initialize the UART module.
 *
 *  The UART peripherals controlled by the UART module are determined by the
 *  UART_config variable.
 */
void OC_CONNECT1_initUART(void);

/*!
 *  @brief  Initialize board specific Watchdog settings
 *
 *  This function initializes the board specific Watchdog settings and then
 *  calls the Watchdog_init API to initialize the Watchdog module.
 *
 *  The Watchdog peripherals controlled by the Watchdog module are determined
 *  by the Watchdog_config variable.
 */
void OC_CONNECT1_initWatchdog(void);

#ifdef __cplusplus
}
#endif

#endif /* __OC_CONNECT1_H */
