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

#define OC_PMIC_ENABLE (1)
#define OC_PMIC_DISABLE (0)
#define OC_SDR_ENABLE (1)
#define OC_SDR_DISABLE (0)
#define OC_SDR_FE_IO_ENABLE (1)
#define OC_SDR_FE_IO_DISABLE (0)
#define OC_FE_ENABLE (1)
#define OC_FE_DISABLE (0)
#define OC_PWR_LION_BATT (1)
#define OC_PWR_LEAD_BATT (0)
#define OC_PWR_PSE_RESET_STATE (1)
#define OC_PWR_PSE_ON_STATE (0)
#define OC_GBC_PROC_ENABLE (1)
#define OC_GBC_PROC_RESET (0)
#define OC_SYNC_IOEXP_ENABLE (1)
#define OC_SYNC_IOEXP_RESET (0)
#define OC_HCI_LED_ENABLE (1)
#define OC_HCI_LED_DISABLE (0)
#define OC_ETH_SW_ENABLE (1)
#define OC_ETH_SW_DISABLE (0)
#define CAT24C256                              \
    {                                          \
        .page_size = 64, .mem_size = (256 / 8) \
    }

/* GBC IO expander Slave address */
#define BIGBROTHER_IOEXP0_ADDRESS 0x71
#define BIGBROTHER_IOEXP1_ADDRESS 0x70
/* SYNC IO expander Slave address */
#define SYNC_IO_DEVICE_ADDR 0x71
/* SDR IO expander Slave address */
#define SDR_FX3_IOEXP_ADDRESS 0x1E
/* RFFE IO expander Slave address */
#define RFFE_CHANNEL1_IO_TX_ATTEN_ADDR 0x18
#define RFFE_CHANNEL1_IO_RX_ATTEN_ADDR 0x1A
#define RFFE_CHANNEL2_IO_TX_ATTEN_ADDR 0x1C
#define RFFE_CHANNEL2_IO_RX_ATTEN_ADDR 0x1D
#define RFFE_IO_REVPOWER_ALERT_ADDR 0x1B

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
    PG,
    PH,
    PJ,
    PK,
    PL,
    PM,
    PN,
    PP,
    PQ
} OC_EC_PORTGroupName;

typedef enum OC_CONNECT1_GPIOName {
    //PA
    OC_EC_DEBUG_UART_RX = 0,
    OC_EC_DEBUG_UART_TX,
    OC_EC_PSE_I2C6_SCLK,
    OC_EC_PSE_I2C6_SDA,
    OC_EC_AP_UART3_RX,
    OC_EC_SOC_UART3_TX,
    OC_EC_PWRMNTR_I2C6_SCLK,
    OC_EC_PWRMNTR_I2C6_SDA,
    //PB
    OC_EC_LT4015_I2C0_SCLK = 8,
    OC_EC_LT40515I2C0_SDA,
    OC_EC_FLASH_nCS,
    OC_EC_FLASH_CLK,
    //PC
    OC_EC_JTAG_TCK = 16,
    OC_EC_JTAG_TMS,
    OC_EC_JTAG_TDI,
    OC_EC_JTAG_TDO,
    OC_EC_SYNCCONN_EC_UART_RX,
    OC_EC_SYNCCONN_UART_TX,
    OC_EC_ETHSW_MDC,
    OC_EC_ETHSW_MDIO,
    //PD
    OC_EC_SYNCCONN_I2C7_SCLK = 24,
    OC_EC_SYNCCONN_I2C7_SDA,
    OC_EC_SDR_INA_ALERT,
    OC_EC_PWR_PSE_RESET,
    OC_NOC_1,
    OC_NOC_2,
    OC_EC_PWR_PRSNT_SOLAR_AUX,
    OC_EC_SYNC_IOEXP_ALERT,
    //PE
    OC_EC_GBC_IOEXP71_ALERT = 32,
    OC_EC_FE_CONTROL, //OC_CONNECT1_GBC_TEMP_ALERT2,
    OC_EC_AP_GPIO1,
    OC_EC_GPP_AP_BM_1,
    OC_EC_FLASH_MOSI,
    OC_EC_FLASH_MISO,
    //PF
    OC_EC_JTAG_TRD2 = 40,
    OC_EC_JTAG_TRD1,
    OC_EC_JTAG_TRD0,
    OC_EC_JTAG_TRCLK,
    OC_EC_JTAG_TRD3,
    //PG
    OC_EC_TEMPSEN_I2C1_SCLK = 48,
    OC_EC_TEMPSEN_I2C1_SDA,
    //PH
    OC_EC_GPP_PMIC_CORE_PWR = 56,
    OC_EC_GPP_SOC_PLTRST, //OC_CONNECT1_PLT_RST_STATUS,//OC_GPP_SOC_PLTRST,OC_CONNECT1_PLT_RST_STATUS
    OC_EC_GPP_PMIC_CTRL,
    OC_EC_GBC_INA_ALERT,
    //PJ
    OC_EC_PWR_PD_NT2P = 64,
    OC_EC_GBC_AP_INA_ALERT,
    //PK
    OC_EC_UART4_RXD = 72,
    OC_EC_UART4_CTS,
    OC_EC_UART4_RTS,
    OC_EC_UART4_TXD,
    OC_EC_TRXFECONN_I2C3_SCLK,
    OC_EC_TRXFECONN_I2C3_SDA,
    OC_EC_TRXFECONN_I2C4_SCLK,
    OC_EC_TRXFECONN_I2C4_SDA,
    //PL
    OC_EC_TRXFECONN_I2C2_SCLK = 80,
    OC_EC_TRXFECONN_I2C2_SDA,
    OC_EC_GBC_PSE_ALERT,
    OC_EC_GPP_AP_BM_2,
    OC_EC_AP_GPIO4,
    OC_EC_PWR_PRSNT_POE,
    OC_EC_USB_DP3,
    OC_EC_USB_DN3,
    //PM
    OC_EC_PWR_LION_ALERT = 88,
    OC_EC_HCI_LED_RESET,
    OC_EC_PWR_MPPT_LION,
    OC_EC_PWR_LACID_ALERT,
    OC_EC_RFFE_TEMP_INA_ALERT,
    OC_EC_ETH_SW_RESET,
    OC_EC_GBC_IOEXP70_INT,
    OC_EC_PWR_BATT_SELECT,
    //PN
    OC_EC_PD_PWRGD_ALERT = 96,
    OC_EC_SDR_FPGA_TEMP_INA_ALERT,
    OC_EC_SDR_DEVICE_CONTROL,
    OC_EC_SDR_PWR_GD,
    OC_EC_FE_PWR_GD,
    OC_EC_MODULE_UART1_RIN,
    //PP
    OC_EC_SDR_FE_IO_RESET_CTRL =
            104, //OC_EC_MPPT_LACID = 104, //OC_SDR_FE_IO_RESET_CTRL
    OC_EC_FE_RESET_OUT,
    OC_EC_SDR_PWR_CNTRL,
    OC_EC_GPP_PWRGD_PROTECTION,
    OC_EC_RFFE_RESET,
    OC_EC_GBC_DEBUG,
    //PQ
    OC_EC_FE_TRXFE_CONN_RESET = 112,
    OC_EC_GPP_MSATA_DAS,
    OC_EC_POE_OVERRIDE,
    OC_EC_GPP_RST_TO_PROC,
    OC_EC_SYNC_RESET,
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
    OC_CONNECT1_I2C6,
    OC_CONNECT1_I2C7,
    OC_CONNECT1_I2C8,
    OC_CONNECT1_I2CCOUNT
} OC_CONNECT1_I2CName;

/*!
 *  @def    OC_CONNECT1_debugMdioName
 *  @brief  Enum of debug MDIO names for Ethernet components
 */
typedef enum OC_CONNECT1_debugMdioName {
    OC_CONNECT1_PHYPORT0 = 0,
    OC_CONNECT1_PHYPORT1,
    OC_CONNECT1_PHYPORT2,
    OC_CONNECT1_PHYPORT3,
    OC_CONNECT1_PHYPORT4,
    OC_CONNECT1_GLOBAL2 = 7,
    OC_CONNECT1_SWPORT0,
    OC_CONNECT1_SWPORT1,
    OC_CONNECT1_SWPORT2,
    OC_CONNECT1_SWPORT3,
    OC_CONNECT1_SWPORT4,
    OC_CONNECT1_SWPORT5,
    OC_CONNECT1_SWPORT6,
    OC_CONNECT1_GLOBAL1,
    OC_CONNECT1_MDIOCOUNT
} OC_CONNECT1_debugMdioName;

/*!
 *  @def    OC_CONNECT1_UARTName
 *  @brief  Enum of UARTs on the OC_CONNECT1 board
 */
typedef enum OC_CONNECT1_UARTName {
    OC_CONNECT1_UART0 = 0,
    OC_CONNECT1_UART3,
    OC_CONNECT1_UART4,

    OC_CONNECT1_UARTXR0,
    OC_CONNECT1_UARTMON,

    OC_CONNECT1_UARTCOUNT
} OC_CONNECT1_UARTName;

/*!
 *  @def    OC_CONNECT1_USBMode
 *  @brief  Enum of USB setup function on the OC_CONNECT1 board
 */
typedef enum OC_CONNECT1_USBMode {
    OC_CONNECT1_USBDEVICE,
    OC_CONNECT1_USBHOST
} OC_CONNECT1_USBMode;

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
 *  @brief  Initialize board specific USB settings
 *
 *  This function initializes the board specific USB settings and pins based on
 *  the USB mode of operation.
 *
 *  @param      usbMode    USB mode of operation
 */
void OC_CONNECT1_initUSB(OC_CONNECT1_USBMode usbMode);

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
