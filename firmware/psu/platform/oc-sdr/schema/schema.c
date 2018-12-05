/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_debugocgpio.h"
#include "common/inc/ocmp_wrappers/ocmp_debugi2c.h"
#include "common/inc/ocmp_wrappers/ocmp_eeprom_cat24c04.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4274.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4295.h"
#include "common/inc/ocmp_wrappers/ocmp_powersource.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "schema.h"

/* Power SubSystem Configs */
SCHEMA_IMPORT DriverStruct eeprom_psu_sid;
SCHEMA_IMPORT DriverStruct eeprom_psu_inv;

SCHEMA_IMPORT const DriverStruct fact_bb_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_bc_se98a;
SCHEMA_IMPORT const DriverStruct fact_fe_1v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_3v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_5v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_24v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_fe_28v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_gbc_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_gen_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_gen_24v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_lithiumIon_cfg;
SCHEMA_IMPORT const DriverStruct fact_lion_12v_ps_cfg;
SCHEMA_IMPORT const DriverStruct fact_ltc4274_cfg;

SCHEMA_IMPORT DriverStruct psu_bat_ps_12v;
SCHEMA_IMPORT DriverStruct psu_bb_ps_12v;
SCHEMA_IMPORT DriverStruct psu_fe_ps_1v;
SCHEMA_IMPORT DriverStruct psu_fe_ps_3v;
SCHEMA_IMPORT DriverStruct psu_fe_ps_5v;
SCHEMA_IMPORT DriverStruct psu_fe_ps_12v;
SCHEMA_IMPORT DriverStruct psu_fe_ps_24v;
SCHEMA_IMPORT DriverStruct psu_fe_ps_28v;
SCHEMA_IMPORT DriverStruct psu_gbc_ps_12v;
SCHEMA_IMPORT DriverStruct psu_gbc_ps_24v;
SCHEMA_IMPORT DriverStruct psu_gen_ps_12v;
SCHEMA_IMPORT DriverStruct psu_gen_ps_24v;
SCHEMA_IMPORT DriverStruct psu_lead_acid_ts;

SCHEMA_IMPORT DriverStruct psu_pwr_int_bat_charger;
SCHEMA_IMPORT DriverStruct psu_pwr_pd;
SCHEMA_IMPORT DriverStruct psu_pwr_powerSource;
SCHEMA_IMPORT DriverStruct psu_pwr_pse;
SCHEMA_IMPORT DriverStruct psu_sensor_ts1;
SCHEMA_IMPORT DriverStruct psu_sensor_ts2;
SCHEMA_IMPORT DriverStruct psu_sensor_ts3;

/*Debug SubSystem Configs*/
SCHEMA_IMPORT DriverStruct debug_I2C0;
SCHEMA_IMPORT DriverStruct debug_I2C1;
SCHEMA_IMPORT DriverStruct debug_I2C2;
SCHEMA_IMPORT DriverStruct debug_I2C3;
SCHEMA_IMPORT DriverStruct debug_I2C4;
SCHEMA_IMPORT DriverStruct debug_I2C5;

SCHEMA_IMPORT DriverStruct debug_psu_gpio_pa;
SCHEMA_IMPORT DriverStruct debug_psu_gpio_pb;
SCHEMA_IMPORT DriverStruct debug_psu_gpio_pc;
SCHEMA_IMPORT DriverStruct debug_psu_gpio_pd;
SCHEMA_IMPORT DriverStruct debug_psu_gpio_pe;
SCHEMA_IMPORT DriverStruct debug_psu_gpio_pf;
SCHEMA_IMPORT DriverStruct debug_psu_gpio_pg;
SCHEMA_IMPORT DriverStruct debug_psu_ioexpander;

/* Functions */
SCHEMA_IMPORT bool psuCore_pre_init(void *driver, void *returnValue);


const Component sys_schema[] = {
    {
     .name = "psuCore",
     .ssHookSet =
         &(SSHookSet){
             .preInitFxn = (ssHook_Cb)psuCore_pre_init,
         },
     .driver_cfg = &psu_pwr_powerSource,
     .components = (Component[]){
         {
             .name = "comp_all",
             .driver = &PWRSRC,
             .driver_cfg = &psu_pwr_powerSource,
         },
         {
             .name = "eeprom1",
             .driver = &CAT24C04_psu_inv,
             .driver_cfg = &eeprom_psu_sid,
         },
         {
             .name = "eeprom2",
             .driver = &CAT24C04_psu_inv,
             .driver_cfg = &eeprom_psu_inv,
         },
         {
             .name = "lion",
             .components = (Component[]){
                 {
                     .name = "battery",
                     .driver = &LTC4015,
                     .driver_cfg = &psu_pwr_int_bat_charger,
                     .factory_config = &fact_lithiumIon_cfg,
                 },
                 {}
             }
         },
         {
             .name = "pse",
             .driver = &LTC4274,
             .driver_cfg = &psu_pwr_pse,
             .factory_config = &fact_ltc4274_cfg,
         },
         {
             .name = "pd",
             .driver = &LTC4295,
             .driver_cfg = &psu_pwr_pd,
         },
         {
             .name = "sensors",
             .components = (Component[]){
                 {
                     .name = "temp_sensor1",
                     .driver = &SE98A,
                     .driver_cfg = &psu_sensor_ts1,
                     .factory_config = &fact_bc_se98a,
                 },
                 {
                     .name = "temp_sensor2",
                     .driver = &SE98A,
                     .driver_cfg = &psu_sensor_ts2,
                     .factory_config = &fact_bc_se98a,
                 },
                 {
                     .name = "temp_sensor3",
                     .driver = &SE98A,
                     .driver_cfg = &psu_sensor_ts3,
                     .factory_config = &fact_bc_se98a,
                 },
                 {}
             },
         },
         {}
       },
    },
    {
        .name = "psubms",
        .components = (Component[]){
            {
                .name = "comp_all",
                .postDisabled = POST_DISABLED,
            },
            {
             .name = "debugI2C",
             .postDisabled = POST_DISABLED,
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
                     .name = "bus5",
                     .driver = &OC_I2C,
                     .driver_cfg = &debug_I2C5,
                     .postDisabled = POST_DISABLED
                 },
                 {}
            },
         },
         {
             .name = "debugGPIO",
             .postDisabled = POST_DISABLED,
             .components = (Component[]){
                 {
                     .name = "comp_all",
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PA",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_gpio_pa,
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PB",
                     .driver = &OC_GPIO,
                     .driver_cfg =&debug_psu_gpio_pb,
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PC",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_gpio_pc,
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PD",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_gpio_pd,
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PE",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_gpio_pe,
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PF",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_gpio_pf,
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "PG",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_gpio_pg,
                     .postDisabled = POST_DISABLED,
                 },
                 {}
             },
         },
         {
             .name = "debugIOexpander",
             .postDisabled = POST_DISABLED,
             .components = (Component[]){
                 {
                     .name = "comp_all",
                     .postDisabled = POST_DISABLED,
                 },
                 {
                     .name = "ioexpander",
                     .driver = &OC_GPIO,
                     .driver_cfg = &debug_psu_ioexpander,
                 },
                 {}
             },
         },
         {
             .name = "fe1",
             .components = (Component[]){
                 {
                     .name = "current_sensor1",
                     .driver = &INA226,
                     .driver_cfg = &psu_fe_ps_1v,
                     .factory_config = &fact_fe_1v_ps_cfg,
                 },
                 {
                     .name = "current_sensor2",
                     .driver = &INA226,
                     .driver_cfg = &psu_fe_ps_3v,
                     .factory_config = &fact_fe_3v_ps_cfg,
                 },
                 {
                     .name = "current_sensor3",
                     .driver = &INA226,
                     .driver_cfg = &psu_fe_ps_5v,
                     .factory_config = &fact_fe_5v_ps_cfg,
                 },
                 {}
             }
         },
         {
             .name = "fe2",
             .components = (Component[]){
                 {
                     .name = "current_sensor4",
                     .driver = &INA226,
                     .driver_cfg = &psu_fe_ps_12v,
                     .factory_config = &fact_fe_12v_ps_cfg,
                 },
                 {
                     .name = "current_sensor5",
                     .driver = &INA226,
                     .driver_cfg = &psu_fe_ps_24v,
                     .factory_config = &fact_fe_24v_ps_cfg,
                 },
                 {
                     .name = "current_sensor6",
                     .driver = &INA226,
                     .driver_cfg = &psu_fe_ps_28v,
                     .factory_config = &fact_fe_28v_ps_cfg,
                 },
                 {}
             },
         },
         {
             .name = "gbc",
             .components = (Component[]){
                 {
                     .name = "current_sensor1",
                     .driver = &INA226,
                     .driver_cfg = &psu_gbc_ps_12v,
                     .factory_config = &fact_gbc_12v_ps_cfg,
                 },
                 {
                     .name = "current_sensor2",
                     .driver = &INA226,
                     .driver_cfg = &psu_gen_ps_12v,
                     .factory_config = &fact_gen_12v_ps_cfg,
                 },
                 {
                     .name = "current_sensor3",
                     .driver = &INA226,
                     .driver_cfg = &psu_gen_ps_24v,
                     .factory_config = &fact_gen_24v_ps_cfg,
                 },
                 {}
             },
         },
         {
             .name = "bb",
             .components = (Component[]){
                 {
                     .name = "current_sensor",
                     .driver = &INA226,
                     .driver_cfg = &psu_bb_ps_12v,
                     .factory_config = &fact_bb_12v_ps_cfg,
                 },
                 {}
             },
         },
         {}
       },
    },
    {}
};
