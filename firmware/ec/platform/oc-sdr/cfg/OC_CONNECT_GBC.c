/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/global/ocmp_frame.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4274.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "common/inc/ocmp_wrappers/ocmp_debugi2c.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "drivers/GpioSX1509.h"
#include "inc/devices/debug_ocgpio.h"
#include "inc/devices/debug_oci2c.h"
#include "inc/devices/debug_ocmdio.h"
#include "inc/devices/ext_battery.h" /* Just for battery resistor configs */
#include "inc/devices/int_battery.h"
#include "inc/subsystem/hci/hci.h"
#include "inc/subsystem/bms/bms.h"
#include "inc/subsystem/gpp/gpp.h"
#include "inc/subsystem/power/power.h"
#include "inc/devices/eth_sw.h"
#include "inc/devices/eeprom.h"
#include <stdint.h>
#include <stdbool.h>

SCHEMA_IMPORT OcGpio_Port ec_io;
SCHEMA_IMPORT OcGpio_Port gbc_io_0;
SCHEMA_IMPORT OcGpio_Port gbc_io_1;
SCHEMA_IMPORT const Driver_fxnTable LTC4274_fxnTable;

/* These are terrible pin names, but they match the net names... */
OcGpio_Pin pin_inven_eeprom_wp = { &gbc_io_0, 1, OCGPIO_CFG_OUT_OD_NOPULL };
OcGpio_Pin pin_s_id_eeprom_wp = { &gbc_io_0, 2, OCGPIO_CFG_OUT_OD_NOPULL };
OcGpio_Pin pin_tempsen_evt1 = { &gbc_io_0, 4 };
OcGpio_Pin pin_tempsen_evt2 = { &gbc_io_0, 5 };
OcGpio_Pin pin_tempsen_evt3 = { &gbc_io_0, 6 };
OcGpio_Pin pin_tempsen_evt4 = { &gbc_io_0, 7 };
OcGpio_Pin pin_tempsen_evt5 = { &gbc_io_0, 8 };
OcGpio_Pin eth_sw_tiva_intn = { &gbc_io_0, 11 };

/*****************************************************************************
 *                               EEPROM CONFIG
 *****************************************************************************/
Eeprom_Cfg eeprom_gbc_sid = {
    .i2c_dev = { OC_CONNECT1_I2C7, 0x51 },
    .pin_wp = &pin_s_id_eeprom_wp,
    .type = CAT24C256,
    .ss = OC_SS_SYS,
};

Eeprom_Cfg eeprom_gbc_inv = {
    .i2c_dev = { OC_CONNECT1_I2C7, 0x50 },
    .pin_wp = &pin_inven_eeprom_wp,
    .type = CAT24C256,
    .ss = OC_SS_SYS,
};

/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
/* Power SubSystem Config */
//Lead Acid Temperature sensor.
SE98A_Dev gbc_pwr_lead_acid_ts = {
    .cfg =
            {
                    .dev = { .bus = OC_CONNECT1_I2C1,
                             .slave_addr =
                                     PWR_LEAD_ACID_BATT_DEV_TEMP_SENS_ADDR },
                    .pin_evt = &pin_tempsen_evt1,
            },
    .obj = {},
};

//Lead acid battery charge controller.
LTC4015_Dev gbc_pwr_ext_bat_charger = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = OC_CONNECT1_I2C0,
                                    .slave_addr =
                                            0x68, /* LTC4015 I2C address in 7-bit format */
                            },
                    .chem = LTC4015_CHEM_LEAD_ACID,
                    .r_snsb = PWR_EXT_BATT_RSNSB,
                    .r_snsi = PWR_EXT_BATT_RSNSI,
                    .cellcount = 6,
                    .pin_lt4015_i2c_sel = { &gbc_io_1, 4,
                                            OCGPIO_CFG_OUT_OD_NOPULL },
                    .pin_alert = &(OcGpio_Pin){ &ec_io, OC_EC_PWR_LACID_ALERT },
            },
    .obj = {},
};

//Lithium ion battery charge controller.
LTC4015_Dev gbc_pwr_int_bat_charger = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = OC_CONNECT1_I2C0,
                                    .slave_addr =
                                            0x68, /* LTC4015 I2C address in 7-bit format */
                            },
                    .chem = LTC4015_CHEM_LI_ION,
                    .r_snsb = PWR_INT_BATT_RSNSB,
                    .r_snsi = PWR_INT_BATT_RSNSI,
                    .cellcount = 3,
                    .pin_lt4015_i2c_sel = { &gbc_io_1, 4,
                                            OCGPIO_CFG_OUT_OD_NOPULL },
                    .pin_alert = &(OcGpio_Pin){ &ec_io, OC_EC_PWR_LION_ALERT },
            },
    .obj = {},
};

//Power Source Equipment
LTC4274_Dev gbc_pwr_pse = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = OC_CONNECT1_I2C8,
                                    .slave_addr =
                                            0x2F, /* LTC4274 I2C address in 7-bit format */
                            },
                    .pin_evt = &(OcGpio_Pin){ &ec_io, OC_EC_GBC_PSE_ALERT },
                    .reset_pin = { &ec_io, OC_EC_PWR_PSE_RESET },
            },
    .obj = {},
};

//Power Device
LTC4275_Dev gbc_pwr_pd = {
    .cfg =
            {
                    .pin_evt = &(OcGpio_Pin){ &ec_io, OC_EC_PD_PWRGD_ALERT },
                    .pin_detect = &(OcGpio_Pin){ &ec_io, OC_EC_PWR_PD_NT2P },

            },
    .obj = {},
};

//Power Source
PWRSRC_Dev gbc_pwr_powerSource = {
    /*Added as a place holder for now.*/
    .cfg =
            {
                    /* SOLAR_AUX_PRSNT_N */
                    .pin_solar_aux_prsnt_n = { &ec_io,
                                               OC_EC_PWR_PRSNT_SOLAR_AUX },
                    /* POE_PRSNT_N */
                    .pin_poe_prsnt_n = { &ec_io, OC_EC_PWR_PRSNT_POE },
                    /* INT_BAT_PRSNT */
                    .pin_int_bat_prsnt = { &gbc_io_0, 11 },
                    /* EXT_BAT_PRSNT */
                    .pin_ext_bat_prsnt = { &gbc_io_0, 12 },
            },
    .obj = {},
};

/* BMS SubSystem Config */
//EC Power sensor for 12V rail.
INA226_Dev gbc_bms_ec_ps_12v = {
    /* 12V Power Sensor */
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C6,
                                    .slave_addr =
                                            BMS_EC_CURRENT_SENSOR_12V_ADDR,
                            },
                    .pin_alert = &(OcGpio_Pin){ &ec_io, OC_EC_GBC_INA_ALERT },
            },
};

//EC Power sensor for 3.3V rail.
INA226_Dev gbc_bms_ec_ps_3p3v = {
    /* 3.3V Power Sensor */
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C7,
                                    .slave_addr =
                                            BMS_EC_CURRENT_SENSOR_3P3V_ADDR,
                            },
                    .pin_alert = &(OcGpio_Pin){ &ec_io, OC_EC_GBC_INA_ALERT },
            },
};

// EC Temperature sensor.
SE98A_Dev gbc_bms_ec_ts = {
    .cfg =
            {
                    .dev = { .bus = OC_CONNECT1_I2C1,
                             .slave_addr = BMS_EC_TEMP_SENSOR_ADDR },
                    .pin_evt = &pin_tempsen_evt2,
            },
    .obj = {},
};

/* HCI SubSystem Config */
// Buzzer
HciBuzzer_Cfg gbc_hci_buzzer = {
    .pin_en = { &gbc_io_0, 10, OCGPIO_CFG_OUT_OD_NOPULL },
};

/* Ethernet Subsystem Config */
Eth_Sw_Cfg g_eth_cfg = {
    .pin_evt = NULL,
    .pin_ec_ethsw_reset = { &ec_io, OC_EC_ETH_SW_RESET },
    .eth_switch = {},
};

//PORT 0
Eth_cfg gbc_eth_port0 = {
    .eth_sw_cfg = &g_eth_cfg,
    .eth_sw_port = PORT0,
};

//PORT 1
Eth_cfg gbc_eth_port1 = {
    .eth_sw_cfg = &g_eth_cfg,
    .eth_sw_port = PORT1,
};

//PORT 2
Eth_cfg gbc_eth_port2 = {
    .eth_sw_cfg = &g_eth_cfg,
    .eth_sw_port = PORT2,
};

//PORT 3
Eth_cfg gbc_eth_port3 = {
    .eth_sw_cfg = &g_eth_cfg,
    .eth_sw_port = PORT3,
};

//PORT 4
Eth_cfg gbc_eth_port4 = {
    .eth_sw_cfg = &g_eth_cfg,
    .eth_sw_port = PORT4,
};

/* GPP Subsystem Config*/
//EC Power sensor for 12V rail.
INA226_Dev gbc_gpp_ap_ps = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C6,
                                    .slave_addr = GPP_AP_CURRENT_SENSOR_ADDR,
                            },
                    .pin_alert =
                            &(OcGpio_Pin){ &ec_io, OC_EC_GBC_AP_INA_ALERT },
            },
};

// AP Temperature sensor
SE98A_Dev gbc_gpp_ap_ts1 = {
    .cfg =
            {
                    .dev = { .bus = OC_CONNECT1_I2C1,
                             .slave_addr = GPP_AP_TEMPSENS1_ADDR },
                    .pin_evt = &pin_tempsen_evt3,
            },
    .obj = {},
};

SE98A_Dev gbc_gpp_ap_ts2 = {
    .cfg =
            {
                    .dev = { .bus = OC_CONNECT1_I2C1,
                             .slave_addr = GPP_AP_TEMPSENS2_ADDR },
                    .pin_evt = &pin_tempsen_evt5,
            },
    .obj = {},
};

SE98A_Dev gbc_gpp_ap_ts3 = {
    .cfg =
            {
                    .dev = { .bus = OC_CONNECT1_I2C1,
                             .slave_addr = GPP_AP_TEMPSENS3_ADDR },
                    .pin_evt = &pin_tempsen_evt4,
            },
    .obj = {},
};

//mSATA power sensor
INA226_Dev gbc_gpp_msata_ps = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C6,
                                    .slave_addr = GPP_MSATA_CURRENT_SENSOR_ADDR,
                            },
                    .pin_alert =
                            &(OcGpio_Pin){ &ec_io, OC_EC_GBC_AP_INA_ALERT },
            },
};

Gpp_gpioCfg gbc_gpp_gpioCfg = (Gpp_gpioCfg){
    /* SOC_PLTRST_N */
    .pin_soc_pltrst_n = { &ec_io, OC_EC_GPP_SOC_PLTRST },
    /* TIVA_SOC_GPIO2 */
    .pin_ap_boot_alert1 = { &ec_io, OC_EC_GPP_AP_BM_1 },
    /* TIVA_SOC_GPIO3 */
    .pin_ap_boot_alert2 = { &ec_io, OC_EC_GPP_AP_BM_2 },
    /* SOC_COREPWROK */
    .pin_soc_corepwr_ok = { &ec_io, OC_EC_GPP_PMIC_CORE_PWR },
    /* MSATA_EC_DAS */
    .pin_msata_ec_das = { &ec_io, OC_EC_GPP_MSATA_DAS },
    /* LT4256_EC_PWRGD */
    .pin_lt4256_ec_pwrgd = { &ec_io, OC_EC_GPP_PWRGD_PROTECTION },
    /* AP_12V_ONOFF */
    .pin_ap_12v_onoff = { &ec_io, OC_EC_GPP_PMIC_CTRL },
    /* EC_RESET_TO_PROC */
    .pin_ec_reset_to_proc = { &ec_io, OC_EC_GPP_RST_TO_PROC },
};

/* Debug Subsystem Config.*/
//I2C Bus
S_I2C_Cfg debug_I2C0 = {
    .bus = OC_CONNECT1_I2C0,
};

S_I2C_Cfg debug_I2C1 = {
    .bus = OC_CONNECT1_I2C1,
};

S_I2C_Cfg debug_I2C2 = {
    .bus = OC_CONNECT1_I2C2,
};

S_I2C_Cfg debug_I2C3 = {
    .bus = OC_CONNECT1_I2C3,
};

S_I2C_Cfg debug_I2C4 = {
    .bus = OC_CONNECT1_I2C4,
};

S_I2C_Cfg debug_I2C6 = {
    .bus = OC_CONNECT1_I2C6,
};

S_I2C_Cfg debug_I2C7 = {
    .bus = OC_CONNECT1_I2C7,
};

S_I2C_Cfg debug_I2C8 = {
    .bus = OC_CONNECT1_I2C8,
};

// MDIO PORTS
S_MDIO_Cfg debug_mdio_phyport0 = {
    .port = OC_CONNECT1_PHYPORT0,
};

S_MDIO_Cfg debug_mdio_phyport1 = {
    .port = OC_CONNECT1_PHYPORT1,
};

S_MDIO_Cfg debug_mdio_phyport2 = {
    .port = OC_CONNECT1_PHYPORT2,
};

S_MDIO_Cfg debug_mdio_phyport3 = {
    .port = OC_CONNECT1_PHYPORT3,
};

S_MDIO_Cfg debug_mdio_phyport4 = {
    .port = OC_CONNECT1_PHYPORT4,
};

S_MDIO_Cfg debug_mdio_global2 = {
    .port = OC_CONNECT1_GLOBAL2,
};

S_MDIO_Cfg debug_mdio_swport0 = {
    .port = OC_CONNECT1_SWPORT0,
};

S_MDIO_Cfg debug_mdio_swport1 = {
    .port = OC_CONNECT1_SWPORT1,
};

S_MDIO_Cfg debug_mdio_swport2 = {
    .port = OC_CONNECT1_SWPORT2,
};

S_MDIO_Cfg debug_mdio_swport3 = {
    .port = OC_CONNECT1_SWPORT3,
};

S_MDIO_Cfg debug_mdio_swport4 = {
    .port = OC_CONNECT1_SWPORT4,
};

S_MDIO_Cfg debug_mdio_swport5 = {
    .port = OC_CONNECT1_SWPORT5,
};

S_MDIO_Cfg debug_mdio_swport6 = {
    .port = OC_CONNECT1_SWPORT0,
};

S_MDIO_Cfg debug_mdio_global1 = {
    .port = OC_CONNECT1_GLOBAL1,
};

//Native GPIO
S_OCGPIO_Cfg debug_ec_gpio_pa = {
    .port = &ec_io,
    .group = PA,
};

S_OCGPIO_Cfg debug_ec_gpio_pb = {
    .port = &ec_io,
    .group = PB,
};

S_OCGPIO_Cfg debug_ec_gpio_pc = {
    .port = &ec_io,
    .group = PC,
};

S_OCGPIO_Cfg debug_ec_gpio_pd = {
    .port = &ec_io,
    .group = PD,
};

S_OCGPIO_Cfg debug_ec_gpio_pe = {
    .port = &ec_io,
    .group = PE,
};

S_OCGPIO_Cfg debug_ec_gpio_pf = {
    .port = &ec_io,
    .group = PF,
};

S_OCGPIO_Cfg debug_ec_gpio_pg = {
    .port = &ec_io,
    .group = PG,
};

S_OCGPIO_Cfg debug_ec_gpio_ph = {
    .port = &ec_io,
    .group = PH,
};

S_OCGPIO_Cfg debug_ec_gpio_pj = {
    .port = &ec_io,
    .group = PJ,
};

S_OCGPIO_Cfg debug_ec_gpio_pk = {
    .port = &ec_io,
    .group = PK,
};

S_OCGPIO_Cfg debug_ec_gpio_pl = {
    .port = &ec_io,
    .group = PL,
};

S_OCGPIO_Cfg debug_ec_gpio_pm = {
    .port = &ec_io,
    .group = PM,
};

S_OCGPIO_Cfg debug_ec_gpio_pn = {
    .port = &ec_io,
    .group = PN,
};

S_OCGPIO_Cfg debug_ec_gpio_pp = {
    .port = &ec_io,
    .group = PP,
};

S_OCGPIO_Cfg debug_ec_gpio_pq = {
    .port = &ec_io,
    .group = PQ,
};

// GBC IO EXPANDERS
S_OCGPIO_Cfg debug_gbc_ioexpanderx70 = {
    .port = &gbc_io_1,
};

S_OCGPIO_Cfg debug_gbc_ioexpanderx71 = {
    .port = &gbc_io_0,
};

/* Factory Configuration for the Devices*/
//Power Factory Config.
const SE98A_Config fact_bc_se98a = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};

const LTC4015_Config fact_leadAcid_cfg = {
    .batteryVoltageLow = 9500,
    .batteryVoltageHigh = 13800,
    .batteryCurrentLow = 100,
    .inputVoltageLow = 16200,
    .inputCurrentHigh = 17000,
    .inputCurrentLimit = 16500,
    .icharge = 10660,
    .vcharge = 12000,
};

const LTC4015_Config fact_lithiumIon_cfg = {
    .batteryVoltageLow = 9500,
    .batteryVoltageHigh = 12600,
    .batteryCurrentLow = 100,
    .inputVoltageLow = 16200,
    .inputCurrentHigh = 5000,
    .inputCurrentLimit = 5570,
};

const LTC4274_Config fact_ltc4274_cfg = {
    .operatingMode = LTC4274_AUTO_MODE,
    .detectEnable = LTC4274_DETECT_ENABLE,
    .interruptMask = LTC4274_INTERRUPT_MASK,
    .interruptEnable = true,
    .pseHpEnable = LTC4274_HP_ENABLE,
};

//BMS factory config.
const SE98A_Config fact_ec_se98a_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};

const INA226_Config fact_ec_12v_ps_cfg = {
    .current_lim = 1000,
};

const INA226_Config fact_ec_3v_ps_cfg = {
    .current_lim = 1000,
};

//GPP fact config
const SE98A_Config fact_ap_se98a_ts1_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};

const SE98A_Config fact_ap_se98a_ts2_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};

const SE98A_Config fact_ap_se98a_ts3_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};

const INA226_Config fact_ap_3v_ps_cfg = {
    .current_lim = 1500,
};

const INA226_Config fact_msata_3v_ps_cfg = {
    .current_lim = 1500,
};