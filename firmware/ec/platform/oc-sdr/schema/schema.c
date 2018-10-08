/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_adt7481.h"
#include "common/inc/ocmp_wrappers/ocmp_dat-xxr5a-pp.h"
#include "common/inc/ocmp_wrappers/ocmp_debugi2c.h"
#include "common/inc/ocmp_wrappers/ocmp_debugmdio.h"
#include "common/inc/ocmp_wrappers/ocmp_debugocgpio.h"
#include "common/inc/ocmp_wrappers/ocmp_debugmdio.h"
#include "common/inc/ocmp_wrappers/ocmp_eeprom_cat24c04.h"
#include "common/inc/ocmp_wrappers/ocmp_eth_sw.h"
#include "common/inc/ocmp_wrappers/ocmp_fe-param.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "common/inc/ocmp_wrappers/ocmp_iridium.h"
#include "common/inc/ocmp_wrappers/ocmp_led.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4274.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4275.h"
#include "common/inc/ocmp_wrappers/ocmp_mac.h"
#include "common/inc/ocmp_wrappers/ocmp_powersource.h"
#include "common/inc/ocmp_wrappers/ocmp_rfpowermonitor.h"
#include "common/inc/ocmp_wrappers/ocmp_rfwatchdog.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "common/inc/ocmp_wrappers/ocmp_syncio.h"
#include "common/inc/ocmp_wrappers/ocmp_testmodule.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "schema.h"

/* SYS Configs*/
SCHEMA_IMPORT DriverStruct eeprom_gbc_sid;
SCHEMA_IMPORT DriverStruct eeprom_gbc_inv;
SCHEMA_IMPORT DriverStruct eeprom_sdr_inv;
SCHEMA_IMPORT DriverStruct eeprom_fe_inv;
/* Power SubSystem Configs */
SCHEMA_IMPORT DriverStruct gbc_pwr_lead_acid_ts;
SCHEMA_IMPORT DriverStruct gbc_pwr_ext_bat_charger;
SCHEMA_IMPORT DriverStruct gbc_pwr_int_bat_charger;
SCHEMA_IMPORT DriverStruct gbc_pwr_pse;
SCHEMA_IMPORT DriverStruct gbc_pwr_pd;
SCHEMA_IMPORT DriverStruct gbc_pwr_powerSource;

/* BMS SubSystem Configs */
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps_12v;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps_3p3v;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ts;

/*HCI SubSystem Configs*/
SCHEMA_IMPORT DriverStruct gbc_hci_buzzer;
SCHEMA_IMPORT DriverStruct led_hci_ts;
SCHEMA_IMPORT DriverStruct led_hci_ioexp;

/*Ethernet SubSystem Configs*/
SCHEMA_IMPORT DriverStruct gbc_eth_port0;
SCHEMA_IMPORT DriverStruct gbc_eth_port1;
SCHEMA_IMPORT DriverStruct gbc_eth_port2;
SCHEMA_IMPORT DriverStruct gbc_eth_port3;
SCHEMA_IMPORT DriverStruct gbc_eth_port4;

/*OBC SubSystem Configs*/
SCHEMA_IMPORT DriverStruct obc_irridium;
SCHEMA_IMPORT DriverStruct sync_obc_gpiocfg;

/*GPP SubSystem Configs*/
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ps;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts1;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts2;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts3;
SCHEMA_IMPORT DriverStruct gbc_gpp_msata_ps;
SCHEMA_IMPORT DriverStruct gbc_gpp_gpioCfg;

/*SDR SubSystem Configs*/
SCHEMA_IMPORT DriverStruct sdr_fpga_ps;
SCHEMA_IMPORT DriverStruct sdr_fpga_ts;
SCHEMA_IMPORT DriverStruct sdr_eeprom_inventory;
SCHEMA_IMPORT DriverStruct sdr_ps;
SCHEMA_IMPORT DriverStruct sdr_gpioCfg;
SCHEMA_IMPORT DriverStruct sdr_fx3_gpiocfg;

/*FE SubSystem Configs*/
SCHEMA_IMPORT DriverStruct fe_rffecfg;
SCHEMA_IMPORT DriverStruct fe_ch1_ps_5_7v;
SCHEMA_IMPORT DriverStruct fe_ch2_ps_5_7v;
SCHEMA_IMPORT DriverStruct fe_ch1_ts;
SCHEMA_IMPORT DriverStruct fe_ch2_ts;
SCHEMA_IMPORT DriverStruct fe_eeprom_inventory;
SCHEMA_IMPORT DriverStruct fe_ch1_ads7830;
SCHEMA_IMPORT DriverStruct fe_ch2_ads7830;
SCHEMA_IMPORT DriverStruct fe_ch1_gain;
SCHEMA_IMPORT DriverStruct fe_ch2_gain;
SCHEMA_IMPORT DriverStruct fe_ch1_lna;
SCHEMA_IMPORT DriverStruct fe_ch2_lna;
SCHEMA_IMPORT DriverStruct fe_ch1_watchdog;
SCHEMA_IMPORT DriverStruct fe_ch2_watchdog;
SCHEMA_IMPORT DriverStruct fe_ch1_tx_gain_cfg;
SCHEMA_IMPORT DriverStruct fe_ch2_tx_gain_cfg;
SCHEMA_IMPORT DriverStruct fe_ch1_rx_gain_cfg;
SCHEMA_IMPORT DriverStruct fe_ch2_rx_gain_cfg;
SCHEMA_IMPORT DriverStruct fe_ch1_bandcfg;
SCHEMA_IMPORT DriverStruct fe_ch2_bandcfg;
SCHEMA_IMPORT DriverStruct fe_ch1_pwrcfg;
SCHEMA_IMPORT DriverStruct fe_ch2_pwrcfg;

/*Sync SubSystem Configs*/
SCHEMA_IMPORT DriverStruct sync_gps_ts;
SCHEMA_IMPORT DriverStruct sync_gps_io;
SCHEMA_IMPORT DriverStruct sync_gpiocfg;

/*TestModule Subsystem Configd*/
SCHEMA_IMPORT DriverStruct testModuleCfg;

/*Debug SubSystem Configs*/
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
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pp;
SCHEMA_IMPORT DriverStruct debug_ec_gpio_pq;

SCHEMA_IMPORT DriverStruct debug_gbc_ioexpanderx70;
SCHEMA_IMPORT DriverStruct debug_gbc_ioexpanderx71;

SCHEMA_IMPORT DriverStruct debug_sdr_ioexpanderx1E;

SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx18;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1C;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1B;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1A;
SCHEMA_IMPORT DriverStruct debug_fe_ioexpanderx1D;

SCHEMA_IMPORT DriverStruct debug_sync_ioexpanderx71;

SCHEMA_IMPORT DriverStruct debug_mdio_phyport0;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport1;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport2;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport3;
SCHEMA_IMPORT DriverStruct debug_mdio_phyport4;
SCHEMA_IMPORT DriverStruct debug_mdio_global1;
SCHEMA_IMPORT DriverStruct debug_mdio_global2;
SCHEMA_IMPORT DriverStruct debug_mdio_swport0;
SCHEMA_IMPORT DriverStruct debug_mdio_swport1;
SCHEMA_IMPORT DriverStruct debug_mdio_swport2;
SCHEMA_IMPORT DriverStruct debug_mdio_swport3;
SCHEMA_IMPORT DriverStruct debug_mdio_swport4;
SCHEMA_IMPORT DriverStruct debug_mdio_swport5;
SCHEMA_IMPORT DriverStruct debug_mdio_swport6;

SCHEMA_IMPORT const DriverStruct fact_bc_se98a;
SCHEMA_IMPORT const DriverStruct fact_leadAcid_cfg;
SCHEMA_IMPORT const DriverStruct fact_lithiumIon_cfg;
SCHEMA_IMPORT const DriverStruct fact_ltc4274_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_led_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts1_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts2_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts3_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_msata_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_sdr_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_sdr_fpga_adt7481_cfg;
SCHEMA_IMPORT const DriverStruct fact_sdr_fpga_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch1_adt7481_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch1_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch2_adt7481_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_ch2_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch1_tx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch1_rx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch2_tx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch2_rx_gain_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch1_band_cfg;
SCHEMA_IMPORT const DriverStruct fact_ch2_band_cfg;
SCHEMA_IMPORT const DriverStruct fact_sync_ts_cfg;

//Function Type
SCHEMA_IMPORT bool gpp_pre_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool gpp_post_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool GPP_ap_Reset(void *driver, void *params);
SCHEMA_IMPORT bool HCI_Init(void *driver, void *returnValue);
SCHEMA_IMPORT bool RFFE_enablePA(void *driver, void *params);
SCHEMA_IMPORT bool RFFE_disablePA(void *driver, void *params);
SCHEMA_IMPORT bool rffe_pre_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool rffe_post_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool RFFE_reset(void *driver, void *params);
SCHEMA_IMPORT bool SDR_Init(void *driver, void *returnValue);
SCHEMA_IMPORT bool SDR_fx3Reset(void *driver, void *params);
SCHEMA_IMPORT bool SDR_reset(void *driver, void *returnValue);
SCHEMA_IMPORT bool SYNC_Init(void *driver, void *returnValue);
SCHEMA_IMPORT bool SYNC_reset(void *driver, void *params);
SCHEMA_IMPORT bool SYS_cmdReset(void *driver, void *params);
SCHEMA_IMPORT bool SYS_cmdEcho(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdEnable(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdDisable(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdDisconnect(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdConnect(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdSendSms(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdDial(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdAnswer(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdHangup(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdReset(void *driver, void *params);
SCHEMA_IMPORT bool obc_pre_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool SYS_post_get_results(void **getpostResult);
SCHEMA_IMPORT bool SYS_post_enable(void **postActivate);

const Component sys_schema[] = {
    {
            .name = "system",
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .driver = &SYSTEMDRV,
                                    .driver_cfg =
                                            &gbc_gpp_gpioCfg, /* For reset pin, will revise */
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "eeprom_sid",
                                                            .driver =
                                                                    &CAT24C04_gbc_sid,
                                                            .driver_cfg =
                                                                    &eeprom_gbc_sid,
                                                    },
                                                    {
                                                            .name = "eeprom_inv",
                                                            .driver =
                                                                    &CAT24C04_gbc_inv,
                                                            .driver_cfg =
                                                                    &eeprom_gbc_inv,
                                                    },
                                                    {
                                                            .name = "eeprom_mac",
                                                            .driver =
                                                                    &Driver_MAC,
                                                    },
                                                    {} },
                                    .commands =
                                            (Command[]){
                                                    {
                                                            .name = "reset",
                                                            .cb_cmd =
                                                                    SYS_cmdReset,
                                                    },
                                                    {
                                                            .name = "echo",
                                                            .cb_cmd =
                                                                    SYS_cmdEcho,
                                                    },
                                                    {} },
                            },
                            {} },
    },
    {
            .name = "power",
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "powerSource",
                                                            .driver = &PWRSRC,
                                                            .driver_cfg =
                                                                    &gbc_pwr_powerSource,
                                                            .postDisabled =
                                                                    POST_DISABLED,
                                                    },
                                                    {} },
                            },
                            { .name = "leadacid_sensor",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "temp_sensor1",
                                                      .driver = &SE98A,
                                                      .driver_cfg =
                                                              &gbc_pwr_lead_acid_ts,
                                                      .factory_config =
                                                              &fact_bc_se98a,
                                              },
                                              {} } },
                            { .name = "leadacid",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "battery",
                                                      .driver = &LTC4015,
                                                      .driver_cfg =
                                                              &gbc_pwr_ext_bat_charger,
                                                      .factory_config =
                                                              &fact_leadAcid_cfg,
                                              },
                                              {} } },
                            { .name = "lion",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "battery",
                                                      .driver = &LTC4015,
                                                      .driver_cfg =
                                                              &gbc_pwr_int_bat_charger,
                                                      .factory_config =
                                                              &fact_lithiumIon_cfg,
                                              },
                                              {} } },
                            {
                                    .name = "pse",
                                    .driver = &LTC4274,
                                    .driver_cfg = &gbc_pwr_pse,
                                    .factory_config = &fact_ltc4274_cfg,
                            },
                            {
                                    .name = "pd",
                                    .driver = &LTC4275,
                                    .driver_cfg = &gbc_pwr_pd,
                            },
                            {} },
    },
    {
            .name = "bms",
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .postDisabled = POST_DISABLED,
                            },
                            { .name = "ec",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "temp_sensor1",
                                                      .driver = &SE98A,
                                                      .driver_cfg =
                                                              &gbc_bms_ec_ts,
                                                      .factory_config =
                                                              &fact_ec_se98a_cfg,
                                              },
                                              {
                                                      .name = "current_sensor1",
                                                      .driver = &INA226,
                                                      .driver_cfg =
                                                              &gbc_bms_ec_ps_12v,
                                                      .factory_config =
                                                              &fact_ec_12v_ps_cfg,
                                              },
                                              {
                                                      .name = "current_sensor2",
                                                      .driver = &INA226,
                                                      .driver_cfg =
                                                              &gbc_bms_ec_ps_3p3v,
                                                      .factory_config =
                                                              &fact_ec_3v_ps_cfg,
                                              },
                                              {} } },
                            {} },
    },
    {
            .name = "hci",
            .ssHookSet =
                    &(SSHookSet){
                            .preInitFxn = (ssHook_Cb)HCI_Init,
                            .postInitFxn = NULL,
                    },
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .postDisabled = POST_DISABLED,
                            },
                            {
                                    .name = "led",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "temp_sensor1",
                                                            .driver = &SE98A,
                                                            .driver_cfg =
                                                                    &led_hci_ts,
                                                            .factory_config =
                                                                    &fact_led_se98a_cfg,
                                                            .postDisabled =
                                                                    POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "fw",
                                                            .driver = &HCI_LED,
                                                            .driver_cfg =
                                                                    &led_hci_ioexp,
                                                    },
                                                    {} },
                            },
                            {
                                    /* TODO: Remove buzzer component if there is no OCMP message
                 * required */
                                    .name = "buzzer",
                                    .driver_cfg = &gbc_hci_buzzer,
                                    .postDisabled = POST_DISABLED,
                            },
                            {} },
    },
    {
            .name = "ethernet",
            .components = (Component[]){ {
                                                 .name = "comp_all",
                                                 .postDisabled = POST_DISABLED,
                                         },
                                         {
                                                 .name = "port0",
                                                 .driver = &ETH_SW,
                                                 .driver_cfg = &gbc_eth_port0,
                                         },
                                         {
                                                 .name = "port1",
                                                 .driver = &ETH_SW,
                                                 .driver_cfg = &gbc_eth_port1,
                                         },
                                         {
                                                 .name = "port2",
                                                 .driver = &ETH_SW,
                                                 .driver_cfg = &gbc_eth_port2,
                                         },
                                         {
                                                 .name = "port3",
                                                 .driver = &ETH_SW,
                                                 .driver_cfg = &gbc_eth_port3,
                                         },
                                         {
                                                 .name = "port4",
                                                 .driver = &ETH_SW,
                                                 .driver_cfg = &gbc_eth_port4,
                                         },
                                         {} },
    },
    {
            .name = "obc",
            .ssHookSet =
                    &(SSHookSet){
                            .preInitFxn = (ssHook_Cb)obc_pre_init,
                            .postInitFxn = NULL,
                    },
            .driver_cfg = &sync_obc_gpiocfg,
            .components = (Component[]){ {
                                                 .name = "comp_all",
                                                 .postDisabled = POST_DISABLED,
                                         },
                                         {
                                                 .name = "iridium",
                                                 .driver = &OBC_Iridium,
                                                 .driver_cfg = &obc_irridium,
                                         },
                                         {} },
    },
    {
            .name = "gpp",
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .postDisabled = POST_DISABLED,
                            },

                            {
                                    .name = "ap",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "temp_sensor1",
                                                            .driver = &SE98A,
                                                            .driver_cfg =
                                                                    &gbc_gpp_ap_ts1,
                                                            .factory_config =
                                                                    &fact_ap_se98a_ts1_cfg,
                                                    },
                                                    {
                                                            .name = "temp_sensor2",
                                                            .driver = &SE98A,
                                                            .driver_cfg =
                                                                    &gbc_gpp_ap_ts2,
                                                            .factory_config =
                                                                    &fact_ap_se98a_ts2_cfg,
                                                    },
                                                    {
                                                            .name = "temp_sensor3",
                                                            .driver = &SE98A,
                                                            .driver_cfg =
                                                                    &gbc_gpp_ap_ts3,
                                                            .factory_config =
                                                                    &fact_ap_se98a_ts3_cfg,
                                                    },
                                                    {
                                                            .name = "current_sensor1",
                                                            .driver = &INA226,
                                                            .driver_cfg =
                                                                    &gbc_gpp_ap_ps,
                                                            .factory_config =
                                                                    &fact_ap_3v_ps_cfg,
                                                    },
                                                    {} },
                                    .driver_cfg = &gbc_gpp_gpioCfg,
                                    .commands =
                                            (Command[]){
                                                    {
                                                            .name = "reset",
                                                            .cb_cmd =
                                                                    GPP_ap_Reset,
                                                    },
                                                    {} },
                            },
                            { .name = "msata",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "current_sensor1",
                                                      .driver = &INA226,
                                                      .driver_cfg =
                                                              &gbc_gpp_msata_ps,
                                                      .factory_config =
                                                              &fact_msata_3v_ps_cfg,
                                              },
                                              {} } },
                            {} },
            .driver_cfg = &gbc_gpp_gpioCfg,
            .ssHookSet =
                    &(SSHookSet){
                            .preInitFxn = (ssHook_Cb)gpp_pre_init,
                            .postInitFxn = (ssHook_Cb)gpp_post_init,
                    },
    },
    {
            .name = "sdr",
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "current_sensor1",
                                                            .driver = &INA226,
                                                            .driver_cfg =
                                                                    &sdr_ps,
                                                            .factory_config =
                                                                    &fact_sdr_3v_ps_cfg,
                                                    },
                                                    {
                                                            /* TODO: this is pretty hw-specific, I think we can
                          * dedupe for the other boards, but I don't think
                          * a framework level driver is appropriate (although,
                          * a proper OC-DB driver might have us revisit this) */
                                                            /* TODO: "eeprom" makes the CLI command pretty verbose,
                          * maybe see about a way of making this better:
                          * sdr.comp_all.eeprom.dev_id is kind of long */
                                                            .name = "eeprom",
                                                            .driver_cfg =
                                                                    &eeprom_sdr_inv,
                                                            .driver = &CAT24C04_sdr_inv,
                                                    },
                                                    {} },
                                    .driver_cfg = &sdr_gpioCfg,
                                    .commands =
                                            (Command[]){
                                                    {
                                                            .name = "reset",
                                                            .cb_cmd = SDR_reset,
                                                    },
                                                    {} },
                            },
                            { .name = "fpga",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "temp_sensor1",
                                                      .driver = &ADT7481,
                                                      .driver_cfg =
                                                              &sdr_fpga_ts,
                                                      .factory_config =
                                                              &fact_sdr_fpga_adt7481_cfg,
                                              },
                                              {
                                                      .name = "current_sensor1",
                                                      .driver = &INA226,
                                                      .driver_cfg =
                                                              &sdr_fpga_ps,
                                                      .factory_config =
                                                              &fact_sdr_fpga_ps_cfg,
                                              },
                                              {} } },
                            {
                                    .name = "fx3",
                                    .driver_cfg = &sdr_gpioCfg,
                                    .commands =
                                            (Command[]){
                                                    {
                                                            .name = "reset",
                                                            .cb_cmd =
                                                                    SDR_fx3Reset,
                                                    },
                                                    {} },
                                    .postDisabled = POST_DISABLED,
                            },
                            {} },
            .driver_cfg = &sdr_gpioCfg,
            .ssHookSet =
                    &(SSHookSet){
                            .preInitFxn = (ssHook_Cb)SDR_Init,
                            .postInitFxn = NULL,
                    },
    },
    {
            .name = "rffe",
            .driver_cfg = &fe_rffecfg,
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "eeprom",
                                                            .driver =
                                                                    &CAT24C04_fe_inv,
                                                            .driver_cfg =
                                                                    &eeprom_fe_inv,
                                                    },
                                                    {} },
                                    .driver_cfg = &sdr_gpioCfg,
                                    .commands =
                                            (Command[]){
                                                    {
                                                            .name = "reset",
                                                            .cb_cmd =
                                                                    RFFE_reset,
                                                    },
                                                    {} },
                            },
                            { .name = "ch1_sensor",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "temp_sensor1",
                                                      .driver = &ADT7481,
                                                      .driver_cfg = &fe_ch1_ts,
                                                      .factory_config =
                                                              &fact_fe_ch1_adt7481_cfg,
                                              },
                                              {
                                                      .name = "current_sensor1",
                                                      .driver = &INA226,
                                                      .driver_cfg =
                                                              &fe_ch1_ps_5_7v,
                                                      .factory_config =
                                                              &fact_fe_ch1_ps_cfg,
                                              },
                                              {} } },
                            { .name = "ch2_sensor",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "temp_sensor1",
                                                      .driver = &ADT7481,
                                                      .driver_cfg = &fe_ch2_ts,
                                                      .factory_config =
                                                              &fact_fe_ch2_adt7481_cfg,
                                              },
                                              {
                                                      .name = "current_sensor1",
                                                      .driver = &INA226,
                                                      .driver_cfg =
                                                              &fe_ch2_ps_5_7v,
                                                      .factory_config =
                                                              &fact_fe_ch2_ps_cfg,
                                              },
                                              {} } },
                            {
                                    .name = "ch1_fe",
                                    .driver_cfg =
                                            &fe_ch1_pwrcfg, /* For en/dis context */
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "ch1_band",
                                                            /* Placeholder driver to let us test the DAT driver */
                                                            .driver = &FE_Param,
                                                            .driver_cfg =
                                                                    &fe_ch1_bandcfg,
                                                            .factory_config =
                                                                    &fact_ch1_band_cfg,
                                                    },
                                                    {
                                                            .name = "watchdog",
                                                            .driver =
                                                                    &RFFEWatchdog,
                                                            .driver_cfg =
                                                                    &fe_ch1_watchdog,
                                                    },
                                                    {
                                                            .name = "power",
                                                            .driver =
                                                                    &RFPowerMonitor,
                                                            .driver_cfg =
                                                                    &fe_ch1_ads7830,
                                                    },
                                                    {
                                                            .name = "tx",
                                                            .driver =
                                                                    &DATXXR5APP,
                                                            /* this struct should be compatible with the DAT cfg struct */
                                                            .driver_cfg =
                                                                    &fe_ch1_gain,
                                                            .factory_config =
                                                                    &fact_ch1_tx_gain_cfg,
                                                    },
                                                    {
                                                            .name = "rx",
                                                            .driver =
                                                                    &DATXXR5APP,
                                                            /* this struct should be compatible with the DAT cfg struct */
                                                            .driver_cfg =
                                                                    &fe_ch1_lna,
                                                            .factory_config =
                                                                    &fact_ch1_rx_gain_cfg,
                                                    },
                                                    {} },
                                    .commands =
                                            (Command[]){
                                                    {
                                                            .name = "enable",
                                                            .cb_cmd =
                                                                    RFFE_enablePA,
                                                    },
                                                    {
                                                            .name = "disable",
                                                            .cb_cmd = RFFE_disablePA,
                                                    },
                                                    {} },
                            },
                            {
                                    .name = "ch2_fe",
                                    .driver_cfg =
                                            &fe_ch2_pwrcfg, /* For en/dis context */
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "ch2_band",
                                                            /* Placeholder driver to let us test the DAT driver */
                                                            .driver = &FE_Param,
                                                            .driver_cfg =
                                                                    &fe_ch2_bandcfg,
                                                            .factory_config =
                                                                    &fact_ch2_band_cfg,
                                                    },
                                                    {
                                                            .name = "watchdog",
                                                            .driver = &RFFEWatchdog,
                                                            .driver_cfg =
                                                                    &fe_ch2_watchdog,
                                                    },
                                                    {
                                                            .name = "power",
                                                            .driver = &RFPowerMonitor,
                                                            .driver_cfg =
                                                                    &fe_ch2_ads7830,
                                                    },
                                                    {
                                                            .name = "tx",
                                                            .driver = &DATXXR5APP,
                                                            /* this struct should be compatible with the DAT cfg struct */
                                                            .driver_cfg =
                                                                    &fe_ch2_gain,
                                                            .factory_config =
                                                                    &fact_ch2_tx_gain_cfg,
                                                    },
                                                    {
                                                            .name = "rx",
                                                            .driver = &DATXXR5APP,
                                                            /* this struct should be compatible with the DAT cfg struct */
                                                            .driver_cfg =
                                                                    &fe_ch2_lna,
                                                            .factory_config =
                                                                    &fact_ch2_rx_gain_cfg,
                                                    },
                                                    {} },
                                    .commands =
                                            (Command[]){ {
                                                                 .name = "enable",
                                                                 .cb_cmd =
                                                                         RFFE_enablePA,
                                                         },
                                                         {
                                                                 .name = "disable",
                                                                 .cb_cmd =
                                                                         RFFE_disablePA,
                                                         },
                                                         {} },
                            },
                            {} },
            .driver_cfg = &fe_rffecfg,
            .ssHookSet =
                    &(SSHookSet){
                            .preInitFxn = (ssHook_Cb)rffe_pre_init,
                            .postInitFxn = (ssHook_Cb)rffe_post_init,
                    },
    },
    { .name = "sync",
      .driver_cfg = &sync_gpiocfg,
      .ssHookSet =
              &(SSHookSet){
                      .preInitFxn = (ssHook_Cb)SYNC_Init,
                      .postInitFxn = NULL,
              },
      .components =
              (Component[]){
                      {
                              .name = "comp_all",
                              .driver_cfg = &sync_gpiocfg,
                              .commands =
                                      (Command[]){
                                              {
                                                      .name = "reset",
                                                      .cb_cmd = SYNC_reset,
                                              },
                                              {} },
                              .postDisabled = POST_DISABLED,
                      },
                      {
                              .name = "gps",
                              .driver_cfg = &sync_gpiocfg,
                              .driver = &Sync_IO,
                      },
                      { .name = "sensor",
                        .components =
                                (Component[]){
                                        {
                                                .name = "temp_sensor1",
                                                .driver = &ADT7481,
                                                .driver_cfg = &sync_gps_ts,
                                                .factory_config =
                                                        &fact_sync_ts_cfg,
                                        },
                                        {} } },
                      {} } },
    { .name = "testmodule",
      .components =
              (Component[]){
                      {
                              .name = "comp_all",
                              .commands =
                                      (Command[]){
                                              {
                                                      .name = "reset",
                                                      .cb_cmd =
                                                              TestMod_cmdReset,
                                              },
                                              {} },
                              .postDisabled = POST_DISABLED,
                      },
                      {
                              .name = "2gsim",
                              .driver = &Testmod_G510,
                              .driver_cfg = &testModuleCfg,
                      },
                      {} } },
    {
            .name = "debug",
            .components =
                    (Component[]){
                            {
                                    .name = "comp_all",
                                    .postDisabled = POST_DISABLED,
                            },
                            {
                                    .name = "I2C",
                                    .components =
                                            (Component[]){
                                                    { .name = "comp_all",
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus0",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C0,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus1",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C1,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus2",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C2,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus3",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C3,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus4",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C4,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus6",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C6,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus7",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C7,
                                                      .postDisabled = POST_DISABLED },
                                                    { .name = "bus8",
                                                      .driver = &OC_I2C,
                                                      .driver_cfg = &debug_I2C8,
                                                      .postDisabled = POST_DISABLED },
                                                    {} },
                            },
                            {
                                    .name = "ec",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "comp_all",
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PA",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pa,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PB",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pb,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PC",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pc,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PD",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pd,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PE",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pe,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PF",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pf,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PG",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pg,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PH",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_ph,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PJ",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pj,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PK",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pk,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PL",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pl,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PM",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pm,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PN",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pn,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PP",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pn,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "PQ",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_ec_gpio_pq,
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {} },
                            },
                            {
                                    .name = "gbc",
                                    .components =
                                            (Component[]){
                                                    {
                                                            .name = "comp_all",
                                                            .postDisabled = POST_DISABLED,
                                                    },
                                                    {
                                                            .name = "ioexpanderx70",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_gbc_ioexpanderx70,
                                                    },
                                                    {
                                                            .name = "ioexpanderx71",
                                                            .driver = &OC_GPIO,
                                                            .driver_cfg =
                                                                    &debug_gbc_ioexpanderx71,
                                                    },
                                                    {} },
                            },
                            { .name = "sdr",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "comp_all",
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "ioexpanderx1E",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_sdr_ioexpanderx1E,
                                              },
                                              {} } },
                            { .name = "fe",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "comp_all",
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "ioexpanderx18",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_sdr_ioexpanderx1E,
                                              },
                                              {
                                                      .name = "ioexpanderx1C",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_fe_ioexpanderx1C,
                                              },
                                              {
                                                      .name = "ioexpanderx1B",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_fe_ioexpanderx1B,
                                              },
                                              {
                                                      .name = "ioexpanderx1A",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_fe_ioexpanderx1A,
                                              },
                                              {
                                                      .name = "ioexpanderx1D",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_fe_ioexpanderx1D,
                                              },
                                              {}

                                      } },
                            { .name = "sync",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "comp_all",
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "ioexpanderx71",
                                                      .driver = &OC_GPIO,
                                                      .driver_cfg =
                                                              &debug_sync_ioexpanderx71,
                                              },
                                              {} } },
                            { .name = "ethernet",
                              .components =
                                      (Component[]){
                                              {
                                                      .name = "comp_all",
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "port0",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_phyport0,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "port1",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_phyport1,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "port2",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_phyport2,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "port3",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_phyport3,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "port4",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_phyport4,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "global1",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_global1,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "global2",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_global2,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport0",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport0,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport1",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport1,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport2",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport2,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport3",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport3,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport4",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport4,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport5",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport5,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {
                                                      .name = "swport6",
                                                      .driver = &OC_MDIO,
                                                      .driver_cfg =
                                                              &debug_mdio_swport6,
                                                      .postDisabled = POST_DISABLED,
                                              },
                                              {} } },
                            {} },
    },
    {}
};
