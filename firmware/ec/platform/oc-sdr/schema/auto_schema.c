/**
* Copyright (c) 2018-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license
* found in the LICENSE file in the root directory of this
* source tree. An additional grant of patent rights can be
* found in the PATENTS file in the same directory.
*
* WARNING: Do not modify this file by hand.  It is auto
*          generated from the json schema definition.
*          Refer to sdtester.py
*/

#include "auto_schema.h"

SCHEMA_IMPORT DriverStruct debug_I2C0;
SCHEMA_IMPORT DriverStruct debug_I2C1;
SCHEMA_IMPORT DriverStruct debug_I2C2;
SCHEMA_IMPORT DriverStruct debug_I2C3;
SCHEMA_IMPORT DriverStruct debug_I2C4;
SCHEMA_IMPORT DriverStruct debug_I2C6;
SCHEMA_IMPORT DriverStruct debug_I2C7;
SCHEMA_IMPORT DriverStruct debug_I2C8;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pa;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pb;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pc;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pd;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pe;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pf;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pg;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_ph;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pj;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pk;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pl;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pm;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pn;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pq;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1A;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1B;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1C;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1D;
SCHEMA_IMPORT DriverStruct debug_gbc_ioexpanderx70;
SCHEMA_IMPORT DriverStruct debug_gbc_ioexpanderx71;
SCHEMA_IMPORT DriverStruct debug_mdio_global1;
SCHEMA_IMPORT DriverStruct debug_mdio_global2;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport0;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport1;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport2;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport3;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport4;
SCHEMA_IMPORT DriverStruct debug_mdio_swport0;
SCHEMA_IMPORT DriverStruct debug_mdio_swport1;
SCHEMA_IMPORT DriverStruct debug_mdio_swport2;
SCHEMA_IMPORT DriverStruct debug_mdio_swport3;
SCHEMA_IMPORT DriverStruct debug_mdio_swport4;
SCHEMA_IMPORT DriverStruct debug_mdio_swport5;
SCHEMA_IMPORT DriverStruct debug_mdio_swport6;
SCHEMA_IMPORT DriverStruct debug_sdr_ioexpanderx1E;
SCHEMA_IMPORT DriverStruct debug_sync_ioexpanderx71;
SCHEMA_IMPORT DriverStruct eeprom_fe_inv;
SCHEMA_IMPORT DriverStruct eeprom_gbc_inv;
SCHEMA_IMPORT DriverStruct eeprom_gbc_sid;
SCHEMA_IMPORT DriverStruct eeprom_sdr_inv;
SCHEMA_IMPORT DriverStruct fe_ch1_ads7830;
SCHEMA_IMPORT DriverStruct fe_ch1_bandcfg;
SCHEMA_IMPORT DriverStruct fe_ch1_gain;
SCHEMA_IMPORT DriverStruct fe_ch1_lna;
SCHEMA_IMPORT DriverStruct fe_ch1_ps_5_7v;
SCHEMA_IMPORT DriverStruct fe_ch1_pwrcfg;
SCHEMA_IMPORT DriverStruct fe_ch1_ts;
SCHEMA_IMPORT DriverStruct fe_ch1_watchdog;
SCHEMA_IMPORT DriverStruct fe_ch2_ads7830;
SCHEMA_IMPORT DriverStruct fe_ch2_bandcfg;
SCHEMA_IMPORT DriverStruct fe_ch2_gain;
SCHEMA_IMPORT DriverStruct fe_ch2_lna;
SCHEMA_IMPORT DriverStruct fe_ch2_ps_5_7v;
SCHEMA_IMPORT DriverStruct fe_ch2_pwrcfg;
SCHEMA_IMPORT DriverStruct fe_ch2_ts;
SCHEMA_IMPORT DriverStruct fe_ch2_watchdog;
SCHEMA_IMPORT DriverStruct fe_rffecfg;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps_12v;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps_3p3v;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ts;
SCHEMA_IMPORT DriverStruct gbc_eth_port0;
SCHEMA_IMPORT DriverStruct gbc_eth_port1;
SCHEMA_IMPORT DriverStruct gbc_eth_port2;
SCHEMA_IMPORT DriverStruct gbc_eth_port3;
SCHEMA_IMPORT DriverStruct gbc_eth_port4;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ps;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts1;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts2;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts3;
SCHEMA_IMPORT DriverStruct gbc_gpp_gpioCfg;
SCHEMA_IMPORT DriverStruct gbc_gpp_msata_ps;
SCHEMA_IMPORT DriverStruct gbc_hci_buzzer;
SCHEMA_IMPORT DriverStruct gbc_pwr_ext_bat_charger;
SCHEMA_IMPORT DriverStruct gbc_pwr_int_bat_charger;
SCHEMA_IMPORT DriverStruct gbc_pwr_lead_acid_ts;
SCHEMA_IMPORT DriverStruct gbc_pwr_pd;
SCHEMA_IMPORT DriverStruct gbc_pwr_powerSource;
SCHEMA_IMPORT DriverStruct gbc_pwr_pse;
SCHEMA_IMPORT DriverStruct led_hci_ioexp;
SCHEMA_IMPORT DriverStruct led_hci_ts;
SCHEMA_IMPORT DriverStruct obc_irridium;
SCHEMA_IMPORT DriverStruct sdr_fpga_ps;
SCHEMA_IMPORT DriverStruct sdr_fpga_ts;
SCHEMA_IMPORT DriverStruct sdr_gpioCfg;
SCHEMA_IMPORT DriverStruct sdr_ps;
SCHEMA_IMPORT DriverStruct sync_gpiocfg;
SCHEMA_IMPORT DriverStruct sync_gps_ts;
SCHEMA_IMPORT DriverStruct sync_obc_gpiocfg;
SCHEMA_IMPORT DriverStruct testModuleCfg;

SCHEMA_IMPORT const DriverStruct fact_ap_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts1_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts2_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts3_cfg;
SCHEMA_IMPORT const DriverStruct fact_bc_se98a;
SCHEMA_IMPORT const DriverStruct fact_ch1_band_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch1_rx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch1_tx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch2_band_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch2_rx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch2_tx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch1_adt7481_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch1_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch2_adt7481_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch2_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_leadAcid_cfg;
SCHEMA_IMPORT const DriverStruct fact_led_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_lithiumIon_cfg;
SCHEMA_IMPORT const DriverStruct fact_ltc4274_cfg;
SCHEMA_IMPORT const DriverStruct fact_msata_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_sdr_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_sdr_fpga_adt7481_cfg;
SCHEMA_IMPORT const DriverStruct fact_sdr_fpga_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_sync_ts_cfg;

SCHEMA_IMPORT bool GPP_ap_Reset(void *, void *);
SCHEMA_IMPORT bool HCI_Init(void *, void *);
SCHEMA_IMPORT bool RFFE_disablePA(void *, void *);
SCHEMA_IMPORT bool RFFE_enablePA(void *, void *);
SCHEMA_IMPORT bool RFFE_reset(void *, void *);
SCHEMA_IMPORT bool SDR_Init(void *, void *);
SCHEMA_IMPORT bool SDR_fx3Reset(void *, void *);
SCHEMA_IMPORT bool SDR_reset(void *, void *);
SCHEMA_IMPORT bool SYNC_Init(void *, void *);
SCHEMA_IMPORT bool SYNC_reset(void *, void *);
SCHEMA_IMPORT bool SYS_cmdEcho(void *, void *);
SCHEMA_IMPORT bool SYS_cmdReset(void *, void *);
SCHEMA_IMPORT bool TestMod_cmdReset(void *, void *);
SCHEMA_IMPORT bool gpp_post_init(void *, void *);
SCHEMA_IMPORT bool gpp_pre_init(void *, void *);
SCHEMA_IMPORT bool obc_pre_init(void *, void *);
SCHEMA_IMPORT bool rffe_post_init(void *, void *);
SCHEMA_IMPORT bool rffe_pre_init(void *, void *);


const Component sys_schema[] = {
    {
        .components = (Component[]) {
            {
                .driver_cfg = &gbc_gpp_gpioCfg,
                .components = (Component[]) {
                    {
                        .driver_cfg = &eeprom_gbc_sid,
                        .driver = &CAT24C04_gbc_sid,
                        .name = "eeprom_sid",
                    },
                    {
                        .driver_cfg = &eeprom_gbc_inv,
                        .driver = &CAT24C04_gbc_inv,
                        .name = "eeprom_inv",
                    },
                    {
                        .driver = &Driver_MAC,
                        .name = "eeprom_mac",
                    },
                    {}
                },
                .driver = &SYSTEMDRV,
                .commands = (Command[]) {
                    {
                        .cb_cmd = SYS_cmdReset,
                        .name = "reset",
                    },
                    {
                        .cb_cmd = SYS_cmdEcho,
                        .name = "echo",
                    },
                    {}
                },
                .name = "comp_all",
            },
            {}
        },
        .name = "system",
    },
    {
        .components = (Component[]) {
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &gbc_pwr_powerSource,
                        .driver = &PWRSRC,
                        .name = "powerSource",
                    },
                    {}
                },
                .name = "comp_all",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &gbc_pwr_lead_acid_ts,
                        .driver = &SE98A,
                        .name = "temp_sensor1",
                        .factory_config = &fact_bc_se98a,
                    },
                    {}
                },
                .name = "leadacid_sensor",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &gbc_pwr_ext_bat_charger,
                        .driver = &LTC4015,
                        .name = "battery",
                        .factory_config = &fact_leadAcid_cfg,
                    },
                    {}
                },
                .name = "leadacid",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &gbc_pwr_int_bat_charger,
                        .driver = &LTC4015,
                        .name = "battery",
                        .factory_config = &fact_lithiumIon_cfg,
                    },
                    {}
                },
                .name = "lion",
            },
            {
                .driver_cfg = &gbc_pwr_pse,
                .driver = &LTC4274,
                .name = "pse",
                .factory_config = &fact_ltc4274_cfg,
            },
            {
                .driver_cfg = &gbc_pwr_pd,
                .driver = &LTC4275,
                .name = "pd",
            },
            {}
        },
        .name = "power",
    },
    {
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .name = "comp_all",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &gbc_bms_ec_ts,
                        .driver = &SE98A,
                        .name = "temp_sensor1",
                        .factory_config = &fact_ec_se98a_cfg,
                    },
                    {
                        .driver_cfg = &gbc_bms_ec_ps_12v,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_ec_12v_ps_cfg,
                    },
                    {
                        .driver_cfg = &gbc_bms_ec_ps_3p3v,
                        .driver = &INA226,
                        .name = "current_sensor2",
                        .factory_config = &fact_ec_3v_ps_cfg,
                    },
                    {}
                },
                .name = "ec",
            },
            {}
        },
        .name = "bms",
    },
    {
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .name = "comp_all",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &led_hci_ts,
                        .driver = &SE98A,
                        .name = "temp_sensor1",
                        .factory_config = &fact_led_se98a_cfg,
                    },
                    {
                        .driver_cfg = &led_hci_ioexp,
                        .driver = &HCI_LED,
                        .name = "fw",
                    },
                    {}
                },
                .name = "led",
            },
            {
                .postDisabled = POST_DISABLED,
                .driver_cfg = &gbc_hci_buzzer,
                .name = "buzzer",
            },
            {}
        },
        .ssHookSet = &(SSHookSet) {
            .preInitFxn = (ssHook_Cb)HCI_Init,
            .postInitFxn = NULL,
        },
        .name = "hci",
    },
    {
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .name = "comp_all",
            },
            {
                .driver_cfg = &gbc_eth_port0,
                .driver = &ETH_SW,
                .name = "port0",
            },
            {
                .driver_cfg = &gbc_eth_port1,
                .driver = &ETH_SW,
                .name = "port1",
            },
            {
                .driver_cfg = &gbc_eth_port2,
                .driver = &ETH_SW,
                .name = "port2",
            },
            {
                .driver_cfg = &gbc_eth_port3,
                .driver = &ETH_SW,
                .name = "port3",
            },
            {
                .driver_cfg = &gbc_eth_port4,
                .driver = &ETH_SW,
                .name = "port4",
            },
            {}
        },
        .name = "ethernet",
    },
    {
        .driver_cfg = &sync_obc_gpiocfg,
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .name = "comp_all",
            },
            {
                .driver_cfg = &obc_irridium,
                .driver = &OBC_Iridium,
                .name = "iridium",
            },
            {}
        },
        .ssHookSet = &(SSHookSet) {
            .preInitFxn = (ssHook_Cb)obc_pre_init,
            .postInitFxn = NULL,
        },
        .name = "obc",
    },
    {
        .driver_cfg = &gbc_gpp_gpioCfg,
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .name = "comp_all",
            },
            {
                .driver_cfg = &gbc_gpp_gpioCfg,
                .components = (Component[]) {
                    {
                        .driver_cfg = &gbc_gpp_ap_ts1,
                        .driver = &SE98A,
                        .name = "temp_sensor1",
                        .factory_config = &fact_ap_se98a_ts1_cfg,
                    },
                    {
                        .driver_cfg = &gbc_gpp_ap_ts2,
                        .driver = &SE98A,
                        .name = "temp_sensor2",
                        .factory_config = &fact_ap_se98a_ts2_cfg,
                    },
                    {
                        .driver_cfg = &gbc_gpp_ap_ts3,
                        .driver = &SE98A,
                        .name = "temp_sensor3",
                        .factory_config = &fact_ap_se98a_ts3_cfg,
                    },
                    {
                        .driver_cfg = &gbc_gpp_ap_ps,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_ap_3v_ps_cfg,
                    },
                    {}
                },
                .commands = (Command[]) {
                    {
                        .cb_cmd = GPP_ap_Reset,
                        .name = "reset",
                    },
                    {}
                },
                .name = "ap",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &gbc_gpp_msata_ps,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_msata_3v_ps_cfg,
                    },
                    {}
                },
                .name = "msata",
            },
            {}
        },
        .ssHookSet = &(SSHookSet) {
            .preInitFxn = (ssHook_Cb)gpp_pre_init,
            .postInitFxn = (ssHook_Cb)gpp_post_init,
        },
        .name = "gpp",
    },
    {
        .driver_cfg = &sdr_gpioCfg,
        .components = (Component[]) {
            {
                .driver_cfg = &sdr_gpioCfg,
                .components = (Component[]) {
                    {
                        .driver_cfg = &sdr_ps,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_sdr_3v_ps_cfg,
                    },
                    {
                        .driver_cfg = &eeprom_sdr_inv,
                        .driver = &CAT24C04_sdr_inv,
                        .name = "eeprom",
                    },
                    {}
                },
                .commands = (Command[]) {
                    {
                        .cb_cmd = SDR_reset,
                        .name = "reset",
                    },
                    {}
                },
                .name = "comp_all",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &sdr_fpga_ts,
                        .driver = &ADT7481,
                        .name = "temp_sensor1",
                        .factory_config = &fact_sdr_fpga_adt7481_cfg,
                    },
                    {
                        .driver_cfg = &sdr_fpga_ps,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_sdr_fpga_ps_cfg,
                    },
                    {}
                },
                .name = "fpga",
            },
            {
                .postDisabled = POST_DISABLED,
                .driver_cfg = &sdr_gpioCfg,
                .commands = (Command[]) {
                    {
                        .cb_cmd = SDR_fx3Reset,
                        .name = "reset",
                    },
                    {}
                },
                .name = "fx3",
            },
            {}
        },
        .ssHookSet = &(SSHookSet) {
            .preInitFxn = (ssHook_Cb)SDR_Init,
            .postInitFxn = NULL,
        },
        .name = "sdr",
    },
    {
        .driver_cfg = &fe_rffecfg,
        .components = (Component[]) {
            {
                .driver_cfg = &sdr_gpioCfg,
                .components = (Component[]) {
                    {
                        .driver_cfg = &eeprom_fe_inv,
                        .driver = &CAT24C04_fe_inv,
                        .name = "eeprom",
                    },
                    {}
                },
                .commands = (Command[]) {
                    {
                        .cb_cmd = RFFE_reset,
                        .name = "reset",
                    },
                    {}
                },
                .name = "comp_all",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &fe_ch1_ts,
                        .driver = &ADT7481,
                        .name = "temp_sensor1",
                        .factory_config = &fact_fe_ch1_adt7481_cfg,
                    },
                    {
                        .driver_cfg = &fe_ch1_ps_5_7v,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_fe_ch1_ps_cfg,
                    },
                    {}
                },
                .name = "ch1_sensor",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &fe_ch2_ts,
                        .driver = &ADT7481,
                        .name = "temp_sensor1",
                        .factory_config = &fact_fe_ch2_adt7481_cfg,
                    },
                    {
                        .driver_cfg = &fe_ch2_ps_5_7v,
                        .driver = &INA226,
                        .name = "current_sensor1",
                        .factory_config = &fact_fe_ch2_ps_cfg,
                    },
                    {}
                },
                .name = "ch2_sensor",
            },
            {
                .driver_cfg = &fe_ch1_pwrcfg,
                .components = (Component[]) {
                    {
                        .driver_cfg = &fe_ch1_bandcfg,
                        .driver = &FE_Param,
                        .name = "ch1_band",
                        .factory_config = &fact_ch1_band_cfg,
                    },
                    {
                        .driver_cfg = &fe_ch1_watchdog,
                        .driver = &RFFEWatchdog,
                        .name = "watchdog",
                    },
                    {
                        .driver_cfg = &fe_ch1_ads7830,
                        .driver = &RFPowerMonitor,
                        .name = "power",
                    },
                    {
                        .driver_cfg = &fe_ch1_gain,
                        .driver = &DATXXR5APP,
                        .name = "tx",
                        .factory_config = &fact_ch1_tx_gain_cfg,
                    },
                    {
                        .driver_cfg = &fe_ch1_lna,
                        .driver = &DATXXR5APP,
                        .name = "rx",
                        .factory_config = &fact_ch1_rx_gain_cfg,
                    },
                    {}
                },
                .commands = (Command[]) {
                    {
                        .cb_cmd = RFFE_enablePA,
                        .name = "enable",
                    },
                    {
                        .cb_cmd = RFFE_disablePA,
                        .name = "disable",
                    },
                    {}
                },
                .name = "ch1_fe",
            },
            {
                .driver_cfg = &fe_ch2_pwrcfg,
                .components = (Component[]) {
                    {
                        .driver_cfg = &fe_ch2_bandcfg,
                        .driver = &FE_Param,
                        .name = "ch2_band",
                        .factory_config = &fact_ch2_band_cfg,
                    },
                    {
                        .driver_cfg = &fe_ch2_watchdog,
                        .driver = &RFFEWatchdog,
                        .name = "watchdog",
                    },
                    {
                        .driver_cfg = &fe_ch2_ads7830,
                        .driver = &RFPowerMonitor,
                        .name = "power",
                    },
                    {
                        .driver_cfg = &fe_ch2_gain,
                        .driver = &DATXXR5APP,
                        .name = "tx",
                        .factory_config = &fact_ch2_tx_gain_cfg,
                    },
                    {
                        .driver_cfg = &fe_ch2_lna,
                        .driver = &DATXXR5APP,
                        .name = "rx",
                        .factory_config = &fact_ch2_rx_gain_cfg,
                    },
                    {}
                },
                .commands = (Command[]) {
                    {
                        .cb_cmd = RFFE_enablePA,
                        .name = "enable",
                    },
                    {
                        .cb_cmd = RFFE_disablePA,
                        .name = "disable",
                    },
                    {}
                },
                .name = "ch2_fe",
            },
            {}
        },
        .ssHookSet = &(SSHookSet) {
            .preInitFxn = (ssHook_Cb)rffe_pre_init,
            .postInitFxn = (ssHook_Cb)rffe_post_init,
        },
        .name = "rffe",
    },
    {
        .driver_cfg = &sync_gpiocfg,
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .driver_cfg = &sync_gpiocfg,
                .commands = (Command[]) {
                    {
                        .cb_cmd = SYNC_reset,
                        .name = "reset",
                    },
                    {}
                },
                .name = "comp_all",
            },
            {
                .driver_cfg = &sync_gpiocfg,
                .driver = &Sync_IO,
                .name = "gps",
            },
            {
                .components = (Component[]) {
                    {
                        .driver_cfg = &sync_gps_ts,
                        .driver = &ADT7481,
                        .name = "temp_sensor1",
                        .factory_config = &fact_sync_ts_cfg,
                    },
                    {}
                },
                .name = "sensor",
            },
            {}
        },
        .ssHookSet = &(SSHookSet) {
            .preInitFxn = (ssHook_Cb)SYNC_Init,
            .postInitFxn = NULL,
        },
        .name = "sync",
    },
    {
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .commands = (Command[]) {
                    {
                        .cb_cmd = TestMod_cmdReset,
                        .name = "reset",
                    },
                    {}
                },
                .name = "comp_all",
            },
            {
                .driver_cfg = &testModuleCfg,
                .driver = &Testmod_G510,
                .name = "2gsim",
            },
            {}
        },
        .name = "testmodule",
    },
    {
        .components = (Component[]) {
            {
                .postDisabled = POST_DISABLED,
                .name = "comp_all",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C0,
                        .driver = &OC_I2C,
                        .name = "bus0",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C1,
                        .driver = &OC_I2C,
                        .name = "bus1",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C2,
                        .driver = &OC_I2C,
                        .name = "bus2",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C3,
                        .driver = &OC_I2C,
                        .name = "bus3",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C4,
                        .driver = &OC_I2C,
                        .name = "bus4",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C6,
                        .driver = &OC_I2C,
                        .name = "bus6",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C7,
                        .driver = &OC_I2C,
                        .name = "bus7",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_I2C8,
                        .driver = &OC_I2C,
                        .name = "bus8",
                    },
                    {}
                },
                .name = "I2C",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pa,
                        .driver = &OC_GPIO,
                        .name = "PA",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pb,
                        .driver = &OC_GPIO,
                        .name = "PB",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pc,
                        .driver = &OC_GPIO,
                        .name = "PC",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pd,
                        .driver = &OC_GPIO,
                        .name = "PD",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pe,
                        .driver = &OC_GPIO,
                        .name = "PE",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pf,
                        .driver = &OC_GPIO,
                        .name = "PF",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pg,
                        .driver = &OC_GPIO,
                        .name = "PG",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_ph,
                        .driver = &OC_GPIO,
                        .name = "PH",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pj,
                        .driver = &OC_GPIO,
                        .name = "PJ",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pk,
                        .driver = &OC_GPIO,
                        .name = "PK",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pl,
                        .driver = &OC_GPIO,
                        .name = "PL",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pm,
                        .driver = &OC_GPIO,
                        .name = "PM",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pn,
                        .driver = &OC_GPIO,
                        .name = "PN",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pn,
                        .driver = &OC_GPIO,
                        .name = "PP",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_ec_gpio_pq,
                        .driver = &OC_GPIO,
                        .name = "PQ",
                    },
                    {}
                },
                .name = "ec",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .driver_cfg = &debug_gbc_ioexpanderx70,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx70",
                    },
                    {
                        .driver_cfg = &debug_gbc_ioexpanderx71,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx71",
                    },
                    {}
                },
                .name = "gbc",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .driver_cfg = &debug_sdr_ioexpanderx1E,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx1E",
                    },
                    {}
                },
                .name = "sdr",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .driver_cfg = &debug_sdr_ioexpanderx1E,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx18",
                    },
                    {
                        .driver_cfg = &debug_fe_ioexpanderx1C,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx1C",
                    },
                    {
                        .driver_cfg = &debug_fe_ioexpanderx1B,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx1B",
                    },
                    {
                        .driver_cfg = &debug_fe_ioexpanderx1A,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx1A",
                    },
                    {
                        .driver_cfg = &debug_fe_ioexpanderx1D,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx1D",
                    },
                    {}
                },
                .name = "fe",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .driver_cfg = &debug_sync_ioexpanderx71,
                        .driver = &OC_GPIO,
                        .name = "ioexpanderx71",
                    },
                    {}
                },
                .name = "sync",
            },
            {
                .components = (Component[]) {
                    {
                        .postDisabled = POST_DISABLED,
                        .name = "comp_all",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_phyport0,
                        .driver = &OC_MDIO,
                        .name = "port0",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_phyport1,
                        .driver = &OC_MDIO,
                        .name = "port1",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_phyport2,
                        .driver = &OC_MDIO,
                        .name = "port2",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_phyport3,
                        .driver = &OC_MDIO,
                        .name = "port3",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_phyport4,
                        .driver = &OC_MDIO,
                        .name = "port4",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_global1,
                        .driver = &OC_MDIO,
                        .name = "global1",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_global2,
                        .driver = &OC_MDIO,
                        .name = "global2",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport0,
                        .driver = &OC_MDIO,
                        .name = "swport0",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport1,
                        .driver = &OC_MDIO,
                        .name = "swport1",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport2,
                        .driver = &OC_MDIO,
                        .name = "swport2",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport3,
                        .driver = &OC_MDIO,
                        .name = "swport3",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport4,
                        .driver = &OC_MDIO,
                        .name = "swport4",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport5,
                        .driver = &OC_MDIO,
                        .name = "swport5",
                    },
                    {
                        .postDisabled = POST_DISABLED,
                        .driver_cfg = &debug_mdio_swport6,
                        .driver = &OC_MDIO,
                        .name = "swport6",
                    },
                    {}
                },
                .name = "ethernet",
            },
            {}
        },
        .name = "debug",
    },
    {}
};
