/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "OC_CONNECT1.h"
#include "schema.h"
#include "src/registry/Framework.h"

/* Framework-provided drivers */
#include "inc/devices/eth_sw.h"
#include "inc/devices/ocmp_wrappers/ocmp_adt7481.h"
#include "inc/devices/ocmp_wrappers/ocmp_dat-xxr5a-pp.h"
#include "inc/devices/ocmp_wrappers/ocmp_eth_sw.h"
#include "inc/devices/ocmp_wrappers/ocmp_ina226.h"
#include "inc/devices/ocmp_wrappers/ocmp_i2c.h"
#include "inc/devices/ocmp_wrappers/ocmp_led.h"
#include "inc/devices/ocmp_wrappers/ocmp_ltc4015.h"
#include "inc/devices/ocmp_wrappers/ocmp_ltc4274.h"
#include "inc/devices/ocmp_wrappers/ocmp_ltc4275.h"
#include "inc/devices/ocmp_wrappers/ocmp_ocgpio.h"
#include "inc/devices/ocmp_wrappers/ocmp_powerSource.h"
#include "inc/devices/ocmp_wrappers/ocmp_se98a.h"
#include "inc/devices/ltc4274.h"

#include "inc/subsystem/bms/bms.h" /* Temporary - for sharing bms config */
#include "inc/subsystem/ethernet/ethernetSS.h"
#include "inc/subsystem/hci/hci.h" /* Temporary - for sharing hci config */
#include "inc/subsystem/gpp/gpp.h" /* Temporary - for sharing gpp config */
#include "inc/subsystem/obc/obc.h"
#include "inc/subsystem/power/power.h" /* Temporary - for sharing power config */
#include "inc/subsystem/sdr/sdr.h" /* Temporary - for sharing sdr config */
#include "inc/subsystem/rffe/rffe.h" /* Temporary - for sharing fe config */
#include "inc/subsystem/rffe/rffe_powermonitor.h" /* For driver export */
#include "inc/subsystem/sync/sync.h" /* Temporary - for sharing sdr config */
#include "inc/subsystem/sys/sys.h"
#include "inc/subsystem/testModule/testModule.h"

extern OcGpio_Port ec_io;
extern OcGpio_Port gbc_io_0;
extern OcGpio_Port gbc_io_1;
extern OcGpio_Port sdr_fx3_io;
extern OcGpio_Port fe_ch1_gain_io;
extern OcGpio_Port fe_ch2_gain_io ;
extern OcGpio_Port fe_ch1_lna_io;
extern OcGpio_Port fe_ch2_lna_io;
extern OcGpio_Port fe_watchdog_io;
extern OcGpio_Port sync_io;

extern OCSubsystem ssSystem;
extern OCSubsystem ssPower;
extern OCSubsystem ssBms;
extern OCSubsystem ssHci;
extern OCSubsystem ssEth;
extern OCSubsystem ssObc;
extern OCSubsystem ssGpp;
extern OCSubsystem ssSdr;
extern OCSubsystem ssRf;
extern OCSubsystem ssSync;
extern OCSubsystem ssTestmod;
extern OCSubsystem ssDbg;

extern Power_Cfg g_power_cfg;
extern Bms_Cfg g_bms_cfg;
extern Hci_Cfg g_hci_cfg;
extern Gpp_Cfg g_gpp_cfg;
extern Obc_Cfg g_obc_cfg;
extern Sdr_Cfg g_sdr_cfg;
extern Fe_Cfg g_fe_cfg;
extern Sync_Cfg g_sync_cfg;
extern Eth_Sw_Cfg  g_eth_cfg;

extern Eeprom_Cfg eeprom_gbc_sid;
extern Eeprom_Cfg eeprom_gbc_inv;

const Component sys_schema[SUBSYSTEM_COUNT] = {
    [OC_SS_SYS] = {
        .name = "system",
        .ss = &ssSystem,
        .components = (Component[]){
            {
                .name = "comp_all",
                .driver_cfg = &g_gpp_cfg, /* For reset pin, will revise */
                .components = (Component[]){
                    {
                        .name = "",
                        .driver = &Driver_EepromSID,
                        .driver_cfg = &eeprom_gbc_sid,
                    },
                    {
                        .name = "",
                        .driver = &Driver_EepromInv,
                        .driver_cfg = &eeprom_gbc_inv,
                    },
                    {
                        .name = "",
                        .driver = &Driver_MAC,
                    },
                    {}
                },
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = SYS_cmdReset,
                    },
                    [OCMP_AXN_TYPE_ECHO] = &(Command){
                        .name = "echo",
                        .cb_cmd = SYS_cmdEcho,
                    },
                    &(Command){}
                },
            },
            {}
        },
    },
    [OC_SS_PWR] = {
        .name = "power",
        .ss = &ssPower,
        .components = (Component[]){
            {
                .name = "comp_all",
                .components = (Component[]){
                    {
                        .name = "powerSource",
                        .driver = &PWRSRC,
                        .driver_cfg = (void *)&g_power_cfg.powerSource,
                    },
                    {}
                },

            },
            {
                .name = "leadacid_tempsensor",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &SE98A,
                        .driver_cfg = (void *)&g_power_cfg.lead_acid_temp_sens,
                        .factory_config = &(SE98A_Config){
                            .lowlimit = -20,
                            .highlimit = 75,
                            .critlimit = 80,
                        }
                    },
                    {}
                }
            },
            {
                .name = "leadacid_battery_charger",
                .components = (Component[]){
                    {
                        .name = "bc",
                        .driver = &LTC4015,
                        .driver_cfg = (void *)&g_power_cfg.ext_bat_charger,
                        .factory_config = &(LTC4015_Config){
                            .batteryVoltageLow = 9500,
                            .batteryVoltageHigh = 13800,
                            .batteryCurrentLow = 100,
                            .inputVoltageLow = 16200,
                            .inputCurrentHigh = 17000,
                            .inputCurrentLimit = 16500,
                            .icharge = 10660,
                            .vcharge = 12000,
                        }
                    },
                    {}
                }
            },
            {
                .name = "lion_battery_charger",
                .components = (Component[]){
                    {
                        .name = "bc",
                        .driver = &LTC4015,
                        .driver_cfg = (void *)&g_power_cfg.int_bat_charger,
                        .factory_config = &(LTC4015_Config){
                            .batteryVoltageLow = 9500,
                            .batteryVoltageHigh = 12600,
                            .batteryCurrentLow = 100,
                            .inputVoltageLow = 16200,
                            .inputCurrentHigh = 5000,
                            .inputCurrentLimit = 5570,
                        }
                    },
                    {}
                }
            },
            {
                .name = "PSE",
                .driver = &LTC4274,
                .driver_cfg = (void *)&g_power_cfg.pse,
                .factory_config = &(LTC4274_Config){
                    .operatingMode = LTC4274_AUTO_MODE,
                    .detectEnable = LTC4274_DETECT_ENABLE,
                    .interruptMask = LTC4274_INTERRUPT_MASK,
                    .interruptEnable = true,
                    .pseHpEnable = LTC4274_HP_ENABLE,
                }
            },
            {
                .name = "PD",
                .driver = &LTC4275,
                .driver_cfg = (void *)&g_power_cfg.pd,
            },
            {}
        },
        .ssHookSet = &(SSHookSet){
            .preInitFxn = pwr_pre_init,
            .postInitFxn = pwr_post_init,
        },
    },
    [OC_SS_BMS] = {
        .name = "bms",
        .ss = &ssBms,
        .components = (Component[]){
            {
                .name = "comp_all"
            },
            {
                .name = "ec",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &SE98A,
                        .driver_cfg = &g_bms_cfg.ec_temp_sensor,
                        .factory_config = &(SE98A_Config){
                            .lowlimit = -20,
                            .highlimit = 75,
                            .critlimit = 80,
                        },
                    },
                    {
                         .name = "12v",
                         .driver = &INA226,
                         .driver_cfg = &g_bms_cfg.ec_current_sensor_12v,
                         .factory_config = &(INA226_Config){
                             .current_lim = 1000,
                         },
                    },
                    {
                         .name = "3v",
                         .driver = &INA226,
                         .driver_cfg = &g_bms_cfg.ec_current_sensor_3p3v,
                         .factory_config = &(INA226_Config){
                             .current_lim = 1500,
                         },
                    },
                    {}
                }
            },
            {}
        },
    },
    [OC_SS_HCI] = {
        .name = "hci",
        .ss = &ssHci,
        .ssHookSet = &(SSHookSet){
            .preInitFxn = HCI_Init,
            .postInitFxn = NULL,
        },
        .components = (Component[]){
            {
                .name = "comp_all"
            },
            {
                .name = "led",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &SE98A,
                        .driver_cfg = &g_hci_cfg.led.temp_sensor,
                        .factory_config = &(SE98A_Config){
                            .lowlimit = -20,
                            .highlimit = 75,
                            .critlimit = 80,
                        },
                    },
                    {
                        .name = "",
                        .driver = &HCI_LED,
                    },
                    {}
                },
                .commands = {
                    [OCMP_AXN_TYPE_SET] = &(Command){
                        .name = "set",
                        .cb_cmd = led_testpattern_control,
                    },
                    &(Command){}
                },
            },
            {
                /* TODO: Remove buzzer component if there is no OCMP message
                 * required */
                .name = "buzzer",
            },
            {}
        },
    },
    [OC_SS_ETH_SWT] = {
        .name = "Ethernet",
        .ss = &ssEth,
        .components = (Component[]){
            {
                .name = "comp_all",
            },
            {
                .name = "port0",
                .driver = &ETH_SW,
                .driver_cfg = &(Eth_cfg){
                    .eth_sw_cfg = &g_eth_cfg,
                    .eth_sw_port = PORT0,
                }
            },
            {
                .name = "port1",
                .driver = &ETH_SW,
                .driver_cfg = &(Eth_cfg){
                    .eth_sw_cfg = &g_eth_cfg,
                    .eth_sw_port = PORT1,
                }
            },
            {
                .name = "port2",
                 .driver = &ETH_SW,
                 .driver_cfg = &(Eth_cfg){
                     .eth_sw_cfg = &g_eth_cfg,
                     .eth_sw_port = PORT2,
                 }
            },
            {
                 .name = "port3",
                 .driver = &ETH_SW,
                 .driver_cfg = &(Eth_cfg){
                     .eth_sw_cfg = &g_eth_cfg,
                     .eth_sw_port = PORT3,
                 }
            },
            {
                 .name = "port4",
                 .driver = &ETH_SW,
                 .driver_cfg = &(Eth_cfg){
                     .eth_sw_cfg = &g_eth_cfg,
                     .eth_sw_port = PORT4,
                 }
            },
            {}
        },
        .ssHookSet = &(SSHookSet){
            .preInitFxn = eth_sw_pre_init,
        },
    },
    [OC_SS_OBC] = {
        .name = "obc",
        .ss = &ssObc,
        .ssHookSet = &(SSHookSet){
            .preInitFxn = obc_pre_init,
        },
        .components = (Component[]){
            {
                .name = "comp_all",
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        //.cb_cmd = GPP_ap_Reset,
                    },
                    &(Command){}
                },
            },
            {
                .name = "iridium",
                .driver = &OBC_Iridium,
                .driver_cfg = &g_obc_cfg.iridium_cfg,
            },
            {}
        },
    },
    [OC_SS_GPP] = {
        .name = "gpp",
        .ss = &ssGpp,
        .components = (Component[]){
            {
                .name = "comp_all",
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = GPP_ap_Reset,
                    },
                    &(Command){}
                },
            },
            {
                .name = "ap",
                .components = (Component[]){
                    {
                         .name = "ts1",
                         .driver = &SE98A,
                         .driver_cfg = &g_gpp_cfg.ap.temp_sensor[0],
                         .factory_config = &(SE98A_Config){
                             .lowlimit = -20,
                             .highlimit = 75,
                             .critlimit = 80,
                         },
                     },
                     {
                         .name = "ts2",
                         .driver = &SE98A,
                         .driver_cfg = &g_gpp_cfg.ap.temp_sensor[1],
                         .factory_config = &(SE98A_Config){
                             .lowlimit = -20,
                             .highlimit = 75,
                             .critlimit = 80,
                         },
                     },
                     {
                         .name = "ts3",
                         .driver = &SE98A,
                         .driver_cfg = &g_gpp_cfg.ap.temp_sensor[2],
                         .factory_config = &(SE98A_Config){
                             .lowlimit = -20,
                             .highlimit = 75,
                             .critlimit = 80,
                         },
                     },
                     {
                         .name = "3v",
                         .driver = &INA226,
                         .driver_cfg = &g_bms_cfg.ec_current_sensor_3p3v,
                         .factory_config = &(INA226_Config){
                             .current_lim = 1500,
                         },
                     },
                     {}
                }
            },
            {
                .name = "msata",
                .components = (Component[]){
                    {
                        .name = "3v",
                        .driver = &INA226,
                        .driver_cfg = &g_bms_cfg.ec_current_sensor_3p3v,
                        .factory_config = &(INA226_Config){
                            .current_lim = 1500,
                        },
                    },
                    {}
                }
            },
            {}
        },
        .ssHookSet = &(SSHookSet){
            .preInitFxn = gpp_pre_init,
            .postInitFxn = gpp_post_init,
        },
    },
    [OC_SS_SDR] = {
        .name = "sdr",
        .ss = &ssSdr,
        .components = (Component[]){
            {
                .name = "comp_all",
                .components = (Component[]){
                    {
                        .name = "ps",
                        .driver = &INA226,
                        .driver_cfg = &g_sdr_cfg.current_sensor,
                        .factory_config = &(INA226_Config){
                            .current_lim = 3000,
                        },
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
                        .driver_cfg = (void *)OC_SS_SDR,
                        .driver = &(Driver){
                        .name = "Inventory",
                        .status = (Parameter[]){
                            { .name = "dev_id", .type = TYPE_STR,
                              .size = (OC_SDR_BOARD_INFO_SIZE + 1) },
                              {}
                            },
                            .cb_get_status = Sdr_InventoryGetStatus,
                        }
                    },
                    {}
                },
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = SDR_reset,
                    },
                    &(Command){}
                },
            },
            {
                .name = "fpga",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &ADT7481,
                        .driver_cfg = (void *)&g_sdr_cfg.fpga.temp_sensor,
                        .factory_config = &(ADT7481_Config){
                            .lowlimit = -20,
                            .highlimit = 80,
                            .critlimit = 85,
                        }
                    },
                    {
                        .name = "ps",
                        .driver = &INA226,
                        .driver_cfg = &g_sdr_cfg.fpga.current_sensor,
                        .factory_config = &(INA226_Config){
                            .current_lim = 500,
                        },
                    },
                    {}
                }
            },
            {
                .name = "fx3",
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = SDR_fx3Reset,
                    },
                    &(Command){}
                },
            },
            {}
        },
        .ssHookSet = &(SSHookSet){
            .preInitFxn = SDR_Init,
            .postInitFxn = NULL,
        },
    },
    [OC_SS_RF] = {
        .name = "rffe",
        .ss = &ssRf,
        .components = (Component[]){
            {
                .name = "comp_all",
                .components = (Component[]){
                    {
                        .name = "eeprom",
                        .driver_cfg = (void *)OC_SS_RF,
                        .driver = &(Driver){
                            .name = "Inventory",
                            .status = (Parameter[]){
                                { .name = "dev_id", .type = TYPE_STR,
                                  .size = (OC_RFFE_BOARD_INFO_SIZE + 1) },
                                 {}
                            },
                            .cb_get_status = RFFE_InventoryGetStatus,
                        }
                    },
                    {}
                },
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = RFFE_reset,
                    },
                    &(Command){}
                },
            },
            {
                .name = "ch1_sensor",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &ADT7481,
                        .driver_cfg = (void *)&g_fe_cfg.adt7481_ch1,
                        .factory_config = &(ADT7481_Config){
                            .lowlimit = -20,
                            .highlimit = 80,
                            .critlimit = 85,
                        }
                    },
                    {
                        .name = "ps",
                        .driver = &INA226,
                        .driver_cfg = &g_fe_cfg.ina226_ch1_5_7v,
                        .factory_config = &(INA226_Config){
                            .current_lim = 2000,
                        },
                    },
                    {}
                }
            },
            {
                .name = "ch2_sensor",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &ADT7481,
                        .driver_cfg = (void *)&g_fe_cfg.adt7481_ch2,
                        .factory_config = &(ADT7481_Config){
                            .lowlimit = -20,
                            .highlimit = 80,
                            .critlimit = 85,
                        }
                    },
                    {
                        .name = "ps",
                        .driver = &INA226,
                        .driver_cfg = &g_fe_cfg.ina226_ch2_5_7v,
                        .factory_config = &(INA226_Config){
                            .current_lim = 2000,
                        },
                    },
                    {}
                }
            },
            {
                .name = "ch1_fe",
                .driver_cfg = (void *)RFFE_CHANNEL1, /* For en/dis context */
                .components = (Component[]){
                    {
                        .name = "",
                        /* Placeholder driver to let us test the DAT driver */
                        .driver = &(Driver){
                            .config = (Parameter[]){
                                { .name = "band", .type = TYPE_UINT16 },
                                { .name = "arfcn", .type = TYPE_UINT16 },
                                {}
                            },
                        },
                    },
                    {
                        .name = "watchdog",
                        .driver = &RFFEWatchdog,
                        .driver_cfg = &(RfWatchdog_Cfg){
                            .pin_alert_lb = &g_fe_cfg.fe_watchdog_cfg.pin_co6_wd,
                            .pin_alert_hb = &g_fe_cfg.fe_watchdog_cfg.pin_co5_wd,
                            .pin_interrupt = &g_fe_cfg.pin_trxfe_conn_reset,
                        }
                    },
                    {
                        .name = "power",
                        .driver = &RFPowerMonitor,
                        .driver_cfg = &g_fe_cfg.ads7830_ch1,
                    },
                    {
                        .name = "tx",
                        .driver = &DATXXR5APP,
                        /* this struct should be compatible with the DAT cfg struct */
                        .driver_cfg = &g_fe_cfg.fe_ch1_gain_cfg.fe_gain_cfg,
                        .factory_config = &(DATR5APP_Config){
                            .attenuation = INT16_MAX, /* Default to max attenuation */
                        }
                    },
                    {
                        .name = "rx",
                        .driver = &DATXXR5APP,
                        /* this struct should be compatible with the DAT cfg struct */
                        .driver_cfg = &g_fe_cfg.fe_ch1_lna_cfg.fe_lna_cfg,
                        .factory_config = &(DATR5APP_Config){
                            .attenuation = INT16_MAX, /* Default to max attenuation */
                        }
                    },
                    {}
                },
                .commands = {
                    [OCMP_AXN_TYPE_ENABLE] = &(Command){
                        .name = "enable",
                        .cb_cmd = RFFE_enablePA,
                    },
                    [OCMP_AXN_TYPE_DISABLE] = &(Command){
                        .name = "disable",
                        .cb_cmd = RFFE_disablePA,
                    },
                    &(Command){}
                },
            },
            {
                .name = "ch2_fe",
                .driver_cfg = (void *)RFFE_CHANNEL2, /* For en/dis context */
                .components = (Component[]){
                    {
                        .name = "",
                        /* Placeholder driver to let us test the DAT driver */
                        .driver = &(Driver){
                            .config = (Parameter[]){
                                { .name = "band", .type = TYPE_UINT16 },
                                { .name = "arfcn", .type = TYPE_UINT16 },
                                {}
                            },
                        }
                    },
                    {
                        .name = "watchdog",
                        .driver = &RFFEWatchdog,
                        .driver_cfg = &(RfWatchdog_Cfg){
                            .pin_alert_lb = &g_fe_cfg.fe_watchdog_cfg.pin_co3_wd,
                            .pin_alert_hb = &g_fe_cfg.fe_watchdog_cfg.pin_co4_wd,
                            .pin_interrupt = &g_fe_cfg.pin_trxfe_conn_reset,
                        }
                    },
                    {
                        .name = "power",
                        .driver = &RFPowerMonitor,
                        .driver_cfg = &g_fe_cfg.ads7830_ch2,
                    },
                    {
                        .name = "tx",
                        .driver = &DATXXR5APP,
                        /* this struct should be compatible with the DAT cfg struct */
                        .driver_cfg = &g_fe_cfg.fe_ch2_gain_cfg.fe_gain_cfg,
                        .factory_config = &(DATR5APP_Config){
                            .attenuation = INT16_MAX, /* Default to max attenuation */
                        }
                    },
                    {
                        .name = "rx",
                        .driver = &DATXXR5APP,
                        /* this struct should be compatible with the DAT cfg struct */
                        .driver_cfg = &g_fe_cfg.fe_ch2_lna_cfg.fe_lna_cfg,
                        .factory_config = &(DATR5APP_Config){
                            .attenuation = INT16_MAX, /* Default to max attenuation */
                        }
                    },
                    {}
                },
                .commands = {
                    [OCMP_AXN_TYPE_ENABLE] = &(Command){
                        .name = "enable",
                        .cb_cmd = RFFE_enablePA,
                    },
                    [OCMP_AXN_TYPE_DISABLE] = &(Command){
                        .name = "disable",
                        .cb_cmd = RFFE_disablePA,
                    },
                    &(Command){}
                },
            },
            {}
        },
        .ssHookSet = &(SSHookSet){
            .preInitFxn = rffe_pre_init,
            .postInitFxn = rffe_post_init,
        },
    },
    [OC_SS_SYNC] = {
        .name = "sync",
        .ss = &ssSync,
        .ssHookSet = &(SSHookSet){
            .preInitFxn = SYNC_Init,
            .postInitFxn = NULL,
        },
        .components = (Component[]){
            {
                .name = "comp_all",
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = SYNC_reset,
                    },
                    &(Command){}
                },
            },
            {
                .name = "gps",
                .driver_cfg = (void *)OC_SS_SYNC,
                .driver = &(Driver){
                    .name = "sync_ioexp",
                    .status = (Parameter[]){
                        {
                            .name = "gps_lock",
                            .type = TYPE_ENUM,
                            .values = (Enum_Map[]){
                                {0, "Gps Not Locked" },
                                {1, "Gps Locked" },
                                {}
                            },
                        },
                        {}
                    },
                    .cb_get_status = SYNC_GpsStatus,
                }
            },
            {
                .name = "temp_sensor",
                .components = (Component[]){
                    {
                        .name = "ts",
                        .driver = &ADT7481,
                        .driver_cfg = (void *)&g_sync_cfg.temp_sens,
                        .factory_config = &(ADT7481_Config){
                            .lowlimit = -20,
                            .highlimit = 80,
                            .critlimit = 85,
                        }
                    },
                    {}
                }
            },
            {}
        }
    },
    [OC_SS_TEST_MODULE] = {
        .name = "testmodule",
        .ss = &ssTestmod,
        .components = (Component[]){
            {
                .name = "comp_all",
                .commands = {
                    [OCMP_AXN_TYPE_RESET] = &(Command){
                        .name = "reset",
                        .cb_cmd = TestMod_cmdReset,
                    },
                    &(Command){}
                },
            },
            {
                .name = "2gsim",
                .driver = &Testmod_G510,
                .commands = {
                    [OCMP_AXN_DIS_NETWORK] = &(Command){
                        .name = "disconnect",
                        .cb_cmd = TestMod_cmdDisconnect,
                    },
                    [OCMP_AXN_CONN_NETWORK] = &(Command){
                        .name = "connect",
                        .cb_cmd = TestMod_cmdConnect,
                    },
                    [OCMP_AXN_SEND_SMS] = &(Command){
                        .name = "send",
                        .cb_cmd = TestMod_cmdSendSms,
                    },
                    [OCMP_AXN_DIAL_NUMBER] = &(Command){
                        .name = "dial",
                        .cb_cmd = TestMod_cmdDial,
                    },
                    [OCMP_AXN_ANSWER] = &(Command){
                        .name = "answer",
                        .cb_cmd = TestMod_cmdAnswer,
                    },
                    [OCMP_AXN_HANGUP] = &(Command){
                        .name = "hangup",
                        .cb_cmd = TestMod_cmdHangup,
                    },
                    [OCMP_AXN_TYPE_ENABLE] = &(Command){
                        .name = "enable",
                        .cb_cmd = TestMod_cmdEnable,
                    },
                    [OCMP_AXN_TYPE_DISABLE] = &(Command){
                        .name = "disable",
                        .cb_cmd = TestMod_cmdDisable,
                    },
                    &(Command){}
                },
            },
            {}
        }
    },
    [OC_SS_DEBUG] = {
        .name = "debug",
        .ss = &ssDbg,
        .components = (Component[]){
            {
                .name = "comp_all",
            },
            {
                .name = "I2C_0",
                .components = (Component[]){
                    {
                        .name = "i2c_0",
                        .driver = &OC_I2C,
                        .driver_cfg = &(S_I2C_Cfg) {
                            .bus = OC_CONNECT1_I2C0,
                        }
                    },
                    {}
                },
            },
            {
                .name = "I2C_1",
                .components = (Component[]){
                    {
                        .name = "i2c_1",
                        .driver = &OC_I2C,
                        .driver_cfg = &(S_I2C_Cfg) {
                            .bus = OC_CONNECT1_I2C1,
                        }
                    },
                    {}
                },
            },
            {
                .name = "I2C_2",
                .components = (Component[]){
                 {
                     .name = "i2c_2",
                     .driver = &OC_I2C,
                     .driver_cfg = &(S_I2C_Cfg) {
                         .bus = OC_CONNECT1_I2C2,
                     }
                 },
                 {}
             },
            },
            {
                 .name = "I2C_3",
                 .components = (Component[]){
                     {
                         .name = "i2c_3",
                         .driver = &OC_I2C,
                         .driver_cfg = &(S_I2C_Cfg) {
                             .bus = OC_CONNECT1_I2C3,
                         }
                     },
                     {}
                 },
            },
            {
                .name = "I2C_4",
                .components = (Component[]){
                    {
                        .name = "i2c_4",
                        .driver = &OC_I2C,
                        .driver_cfg = &(S_I2C_Cfg) {
                            .bus = OC_CONNECT1_I2C4,
                        }
                    },
                 {}
                },
            },
            {
             .name = "I2C_6",
             .components = (Component[]){
                 {
                     .name = "i2c_6",
                     .driver = &OC_I2C,
                     .driver_cfg = &(S_I2C_Cfg) {
                         .bus = OC_CONNECT1_I2C6,
                     }
                 },
                 {}
             },
            },
            {
                .name = "I2C_7",
                .components = (Component[]){
                    {
                        .name = "i2c_7",
                        .driver = &OC_I2C,
                        .driver_cfg = &(S_I2C_Cfg) {
                            .bus = OC_CONNECT1_I2C7,
                        }
                    },
                    {}
                },
            },
            {
                 .name = "I2C_8",
                 .components = (Component[]){
                     {
                         .name = "i2c_8",
                         .driver = &OC_I2C,
                         .driver_cfg = &(S_I2C_Cfg) {
                             .bus = OC_CONNECT1_I2C8,
                         }
                     },
                     {}
                 },
             },
             {
                  .name = "EC",
                  .components = (Component[]) {
                      {
                          .name = "comp_all",
                      },
                      {
                          .name = "PA",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PA,
                          }
                      },
                      {
                          .name = "PB",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PB,
                          }
                      },
                      {
                          .name = "PC",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PC,
                          }
                      },
                      {
                          .name = "PD",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PD,
                          }
                      },
                      {
                          .name = "PE",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PE,
                          }
                      },
                      {
                          .name = "PF",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PF,
                          }
                      },
                      {
                          .name = "PG",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PG,
                          }
                      },
                      {
                          .name = "PH",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PH,
                          }
                      },
                      {
                          .name = "PJ",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PJ,
                          }
                      },
                      {
                          .name = "PK",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PK,
                          }
                      },
                      {
                          .name = "PL",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PL,
                          }
                      },
                      {
                          .name = "PM",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PM,
                          }
                      },
                      {
                          .name = "PN",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PN,
                          }
                      },
                      {
                          .name = "PP",
                          .driver = &OC_GPIO,
                          .driver_cfg =&(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PP,
                          }
                      },
                      {
                          .name = "PQ",
                          .driver = &OC_GPIO,
                          .driver_cfg = &(S_OCGPIO_Cfg) {
                              .port = &ec_io,
                              .group = PQ,
                          }
                      },
                      {}
                  },
             },
             {
                 .name = "GBC",
                 .components = (Component[]) {
                     {
                         .name = "comp_all",
                     },
                     {
                         .name = "ioexpanderx70",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &gbc_io_1,
                         }
                     },
                     {
                         .name = "ioexpanderx71",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &gbc_io_0,
                         }
                     },
                     {}
                 },
             },
             {
                 .name = "SDR",
                 .components = (Component[]) {
                     {
                         .name = "comp_all",
                     },
                     {
                         .name = "ioexpanderx1E",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &sdr_fx3_io,
                         }
                     },
                     {}
                 }
             },
             {
                 .name = "FE",
                 .components = (Component[]) {
                     {
                         .name = "comp_all",
                     },
                     {
                         .name = "ioexpanderx18",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &fe_ch1_gain_io,
                         }
                     },
                     {
                         .name = "ioexpanderx1C",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &fe_ch2_gain_io,
                         }
                     },
                     {
                         .name = "ioexpanderx1B",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &fe_watchdog_io,
                         }
                     },
                     {
                         .name = "ioexpanderx1A",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &fe_ch1_lna_io,
                         }
                     },
                     {
                         .name = "ioexpanderx1D",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &fe_ch2_lna_io,
                         }
                     },
                     {}

                 }
             },
             {
                 .name = "SYNC",
                 .components = (Component[]) {
                     {
                         .name = "comp_all",
                     },
                     {
                         .name = "ioexpanderx71",
                         .driver = &OC_GPIO,
                         .driver_cfg = &(S_OCGPIO_Cfg) {
                             .port = &sync_io,
                         }
                     },
                     {}
                 }
             },
             {}
         },
     }
};
