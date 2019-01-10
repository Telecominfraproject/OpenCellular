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
#include "common/inc/ocmp_wrappers/ocmp_slb9645.h"
#include "common/inc/ocmp_wrappers/ocmp_si1141.h"
#include "common/inc/ocmp_wrappers/ocmp_mp2951.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "schema.h"

/* SYS Configs*/
SCHEMA_IMPORT DriverStruct eeprom_gbc_sid;
SCHEMA_IMPORT DriverStruct eeprom_gbc_inv;

/* BMS SubSystem Configs */
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps_12v;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps_3p3v;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ts;
SCHEMA_IMPORT DriverStruct gbc_bms_ec_ps;
SCHEMA_IMPORT DriverStruct hci_connector_ts;

/*HCI SubSystem Configs*/
SCHEMA_IMPORT DriverStruct gbc_hci_buzzer;
SCHEMA_IMPORT DriverStruct led_hci_ts;
SCHEMA_IMPORT DriverStruct led_hci_ioexp;

/*Ethernet SubSystem Configs*/
SCHEMA_IMPORT DriverStruct gbc_eth_port1;
SCHEMA_IMPORT DriverStruct gbc_eth_port2;
SCHEMA_IMPORT DriverStruct gbc_eth_port3;
SCHEMA_IMPORT DriverStruct gbc_eth_port4;
SCHEMA_IMPORT DriverStruct gbc_eth_port5;
SCHEMA_IMPORT DriverStruct gbc_eth_port6;
SCHEMA_IMPORT DriverStruct gbc_eth_port7;

/*GPP SubSystem Configs*/
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ps;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts1;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts2;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_ts3;
SCHEMA_IMPORT DriverStruct gbc_eth_ec_ts;
SCHEMA_IMPORT DriverStruct gbc_gpp_msata_ps;
SCHEMA_IMPORT DriverStruct gbc_gpp_gpioCfg;
SCHEMA_IMPORT DriverStruct gbc_gpp_mp2951;
SCHEMA_IMPORT DriverStruct gbc_gpp_slb9645;
SCHEMA_IMPORT DriverStruct gbc_gpp_ap_mp2951_ts;

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
SCHEMA_IMPORT const DriverStruct fact_ec_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_hci_conct_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ec_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_led_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts1_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts2_cfg;
SCHEMA_IMPORT const	DriverStruct fact_ap_mp2951_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_se98a_ts3_cfg;
SCHEMA_IMPORT const	DriverStruct fact_eth_se98a_cfg;
SCHEMA_IMPORT const DriverStruct fact_ap_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_msata_3v_ps_cfg;

//Function Type
SCHEMA_IMPORT bool gpp_pre_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool gpp_post_init(void *driver, void *returnValue);
SCHEMA_IMPORT bool GPP_ap_Reset(void *driver, void *params);
SCHEMA_IMPORT bool HCI_Init(void *driver, void *returnValue);
SCHEMA_IMPORT bool SYS_cmdReset(void *driver, void *params);
SCHEMA_IMPORT bool SYS_cmdEcho(void *driver, void *params);
SCHEMA_IMPORT bool SYS_post_get_results(void **getpostResult);
SCHEMA_IMPORT bool SYS_post_enable(void **postActivate);
SCHEMA_IMPORT bool at45db_init(void* driver, void *returnValu);
SCHEMA_IMPORT bool fs_createtask(void* driver, void *returnValue);

const Component sys_schema[] = {
    {
        .name = "system",
        .components = (Component[]){
            {
                .name = "comp_all",
                .driver = &SYSTEMDRV,
                .driver_cfg = &gbc_gpp_gpioCfg, /* For reset pin, will revise */
                .components = (Component[]){
                    {
                        .name = "eeprom_sid",
                        .driver = &CAT24C04_gbc_sid,
                        .driver_cfg = &eeprom_gbc_sid,
                    },
                    {
                        .name = "eeprom_inv",
                        .driver = &CAT24C04_gbc_inv,
                        .driver_cfg = &eeprom_gbc_inv,
                    },
                    {
                        .name = "eeprom_mac",
                        .driver = &Driver_MAC,
                    },
                    {
                        .name = "flash",
                        .driver = &FLASHDRV,
                    },
                    {}
                },
                .commands = (Command[]){
                    {
                        .name = "reset",
                        .cb_cmd = SYS_cmdReset,
                    },
                    {
                        .name = "echo",
                        .cb_cmd = SYS_cmdEcho,
                    },
                   {}
                },
            },
            {}
        },
        .ssHookSet = &(SSHookSet){
            .preInitFxn = (ssHook_Cb)at45db_init,
            .postInitFxn = (ssHook_Cb)fs_createtask,
        },
    },
    {
        .name = "bms",
        .components = (Component[]){
            {
                .name = "comp_all",
                .postDisabled = POST_DISABLED,
            },
            {
                .name = "ec",
                .components = (Component[]){
                    {
                        .name = "temp_sensor1",
                        .driver = &SE98A,
                        .driver_cfg = &gbc_bms_ec_ts,
                        .factory_config = &fact_ec_se98a_cfg,
                    },
                    {
                        .name = "temp_sensor2",
                        .driver = &SE98A,
                        .driver_cfg = &hci_connector_ts,
                        .factory_config = &fact_hci_conct_se98a_cfg,
                    },
                    {
                         .name = "current_sensor1",
                         .driver = &INA226,
                         .driver_cfg = &gbc_bms_ec_ps_12v,
                         .factory_config = &fact_ec_12v_ps_cfg,
                    },
                    {
                         .name = "current_sensor2",
                         .driver = &INA226,
                         .driver_cfg = &gbc_bms_ec_ps_3p3v,
                         .factory_config = &fact_ec_3v_ps_cfg,
                    },
                    {}
                }
            },
            {
                .name = "ps",
                .components = (Component[]){
                    {
                        .name = "si1141",
                        .driver = &SI1141,
                        .driver_cfg = &gbc_bms_ec_ps,
                        .factory_config = NULL,
                    },
                    {}
                }
            },
            {}
        },
    },
    {
        .name = "hci",
        .ssHookSet = &(SSHookSet){
            .preInitFxn = (ssHook_Cb)HCI_Init,
            .postInitFxn = NULL,
        },
        .components = (Component[]){
            {
                .name = "comp_all",
                .postDisabled = POST_DISABLED,
            },
            {
                .name = "led",
                .components = (Component[]){
                    {
                        .name = "temp_sensor1",
                        .driver = &SE98A,
                        .driver_cfg = &led_hci_ts,
                        .factory_config = &fact_led_se98a_cfg,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "fw",
                        .driver = &HCI_LED,
                        .driver_cfg = &led_hci_ioexp,
                    },
                    {}
                },
            },
            {
                /* TODO: Remove buzzer component if there is no OCMP message
                 * required */
                .name = "buzzer",
                .driver_cfg =  &gbc_hci_buzzer,
                .postDisabled = POST_DISABLED,
            },
            {}
        },
    },
    {
        .name = "ethernet",
        .components = (Component[]){
            {
                .name = "comp_all",
                .postDisabled = POST_DISABLED,
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
            {
                 .name = "port5",
                 .driver = &ETH_SW,
                 .driver_cfg = &gbc_eth_port5,
            },
            {
                 .name = "port6",
                 .driver = &ETH_SW,
                 .driver_cfg = &gbc_eth_port6,
            },
            {
                 .name = "port7",
                 .driver = &ETH_SW,
                 .driver_cfg = &gbc_eth_port7,
            },
            {
                 .name = "temp_sensor", /* newly added for gbc v2*/
                 .driver = &SE98A,
                 .driver_cfg = &gbc_eth_ec_ts,
                 .factory_config = &fact_eth_se98a_cfg,
            },
            {}
        },
    },
    {
        .name = "gpp",
        .components = (Component[]){
            {
                .name = "comp_all",
                .postDisabled = POST_DISABLED,
            },
            {
                .name = "ap",
                .components = (Component[]){
                     {
                         .name = "temp_sensor1",
                         .driver = &SE98A,
                         .driver_cfg = &gbc_gpp_ap_ts1,
                         .factory_config = &fact_ap_se98a_ts3_cfg,
                     },
					  {
						  .name = "temp_sensor2", /* Near to mp2951*/
						  .driver = &SE98A,
						  .driver_cfg = &gbc_gpp_ap_mp2951_ts,
						  .factory_config = &fact_ap_mp2951_se98a_cfg,
					  },
                     {
                         .name = "current_sensor1",
                         .driver = &INA226,
                         .driver_cfg = &gbc_gpp_ap_ps,
                         .factory_config = &fact_ap_3v_ps_cfg,
                     },
					  {
					   	  .name = "mp2951",
					 	  .driver = &MP2951,
						  .driver_cfg = &gbc_gpp_mp2951,
						  .factory_config = &fact_ap_mp2951_se98a_cfg,
					  },
                     {}
                },
                .driver_cfg = &gbc_gpp_gpioCfg,
                .commands = (Command[]){
                    {
                        .name = "reset",
                        .cb_cmd = GPP_ap_Reset,
                    },
                    {}
                },
            },
            {
                .name = "msata",
                .components = (Component[]){
                    {
                        .name = "current_sensor1",
                        .driver = &INA226,
                        .driver_cfg = &gbc_gpp_msata_ps,
                        .factory_config = &fact_msata_3v_ps_cfg,
                    },
                    {}
                }
            },
            {
                .name = "tpm",
                .driver = &SLB9645,
                .driver_cfg = &gbc_gpp_slb9645,
                .factory_config = NULL,
            },
            {}
        },
        .driver_cfg =  &gbc_gpp_gpioCfg,
        .ssHookSet = &(SSHookSet){
            .preInitFxn = (ssHook_Cb)gpp_pre_init,
            .postInitFxn = (ssHook_Cb)gpp_post_init,
        },
    },
    {
        .name = "debug",
        .components = (Component[]){
            {
                .name = "comp_all",
                .postDisabled = POST_DISABLED,
            },
            {
                .name = "I2C",
                .components = (Component[]){
                    {
                        .name = "comp_all",
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus0",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C0,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus1",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C1,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus2",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C2,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus3",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C3,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus4",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C4,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus6",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C6,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus7",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C7,
                        .postDisabled = POST_DISABLED
                    },
                    {
                        .name = "bus8",
                        .driver = &OC_I2C,
                        .driver_cfg = &debug_I2C8,
                        .postDisabled = POST_DISABLED
                    },
                    {}
                },
            },
            {
                .name = "ec",
                .components = (Component[]){
                    {
                        .name = "comp_all",
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PA",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pa,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PB",
                        .driver = &OC_GPIO,
                        .driver_cfg =&debug_ec_gpio_pb,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PC",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pc,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PD",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pd,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PE",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pe,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PF",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pf,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PG",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pg,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PH",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_ph,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PJ",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pj,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PK",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pk,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PL",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pl,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PM",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pm,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PN",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pn,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PP",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pp,
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "PQ",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_ec_gpio_pq,
                        .postDisabled = POST_DISABLED,
                    },
                    {}
                },
            },
            {
                .name = "gbc",
                .components = (Component[]) {
                    {
                        .name = "comp_all",
                        .postDisabled = POST_DISABLED,
                    },
                    {
                        .name = "ioexpanderx70",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_gbc_ioexpanderx70,
                    },
                    {
                        .name = "ioexpanderx71",
                        .driver = &OC_GPIO,
                        .driver_cfg = &debug_gbc_ioexpanderx71,
                    },
                    {}
                },
            },
            {}
        },
    },
    {}
};
