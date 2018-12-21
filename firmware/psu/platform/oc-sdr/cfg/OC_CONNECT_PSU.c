/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/global/ocmp_frame.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/ocmp_wrappers/ocmp_debugi2c.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4274.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "drivers/GpioSX1509.h"
#include "inc/devices/debug_ocgpio.h"
#include "inc/devices/debug_oci2c.h"
#include "inc/devices/eeprom.h"
#include "inc/devices/ina226.h"
#include "inc/devices/int_battery.h"
#include "inc/subsystem/hci/hci.h"
#include "inc/subsystem/power/power.h"
#include <stdint.h>
#include <stdbool.h>

SCHEMA_IMPORT OcGpio_Port ec_io;
SCHEMA_IMPORT OcGpio_Port pwr_io;
SCHEMA_IMPORT const Driver_fxnTable LTC4274_fxnTable;

/*****************************************************************************
 *                               EEPROM CONFIG
 *****************************************************************************/

Eeprom_PowerCfg power_line_cfg = {
    .pin_24v   = { &pwr_io, 3},
    .pin_5v0   = { &pwr_io, 4},
    .pin_3v3   = { &pwr_io, 5},
    .pin_gbcv2_on   = { &pwr_io, 6},
    .pin_12v_bb   = { &pwr_io, 7},
    .pin_12v_fe   = { &pwr_io, 8},
    .pin_20v_fe   = { &pwr_io, 9},
    .pin_1v8   = { &pwr_io, 10},
};

Eeprom_Cfg eeprom_psu_sid = {
    .i2c_dev = { OC_CONNECT1_I2C0, 0x56 },
//    .pin_wp = &pin_s_id_eeprom_wp,
    .type = CAT24C256,
//    .ss = OC_SS_SYS,
    .power_cfg = &power_line_cfg,
};

Eeprom_Cfg eeprom_psu_inv = {
    .i2c_dev = { OC_CONNECT1_I2C5, 0x56 },
//    .pin_wp = &pin_inven_eeprom_wp,
    .type = CAT24C256,
//    .ss = OC_SS_SYS,
};

/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
/* Power SubSystem Config */
//Lithium ion battery charge controller.
LTC4015_Dev psu_pwr_int_bat_charger = {
       .cfg = {
           .i2c_dev = {
               .bus = OC_CONNECT1_I2C2,
               .slave_addr = 0x68, /* LTC4015 I2C address in 7-bit format */
           },
           .chem = LTC4015_CHEM_LI_ION,
           .r_snsb = PWR_INT_BATT_RSNSB,
           .r_snsi = PWR_INT_BATT_RSNSI,
           .cellcount = 3,
           .pin_alert = &(OcGpio_Pin){ &ec_io, OC_EC_CHARGER_ALERT},
       },
       .obj = {},
};

INA226_Dev pwr_bat_ps_12v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C3,
            .slave_addr = 0x58,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_fe_ps_1v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C4,
            .slave_addr = 0x45,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_fe_ps_3v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C4,
            .slave_addr = 0x44,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_fe_ps_5v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C4,
            .slave_addr = 0x41,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_fe_ps_12v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C1,
            .slave_addr = 0x41,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_fe_ps_24v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C1,
            .slave_addr = 0x45,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_fe_ps_28v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C4,
            .slave_addr = 0x40,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_gbc_ps_12v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C1,
            .slave_addr = 0x44,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};
INA226_Dev psu_gen_ps_12v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C2,
            .slave_addr = 0x41,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_gen_ps_24v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C2,
            .slave_addr = 0x44,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};

INA226_Dev psu_bb_ps_12v = {
    /* 12V Power Sensor */
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C1,
            .slave_addr = 0x40,
        },
        .pin_alert = &(OcGpio_Pin){ &ec_io,
                                    OC_EC_CHARGER_ALERT },
    },
};
//Power Source Equipment
LTC4274_Dev psu_pwr_pse = {
    .cfg = {
        .i2c_dev = {
            .bus = OC_CONNECT1_I2C3,
            .slave_addr = 0x2F, /* LTC4274 I2C address in 7-bit format */
        },
        .pin_evt = &(OcGpio_Pin){ &ec_io,
            OC_EC_PSE_INT },
        .reset_pin ={ &ec_io, OC_EC_nPSE_RESET},
    },
    .obj = {},
};

//Power Device
LTC4295_Dev psu_pwr_pd = {
    .cfg = {
            //TODO: find the powergood pin
            .pin_evt = &(OcGpio_Pin){ &ec_io,
                                           OC_EC_POE_IN_PRESENT},
            .pin_detect = &(OcGpio_Pin){ &ec_io,
                                           OC_EC_PD_T2P},
        },
        .obj = {},
    };

//Power Source
PWRSRC_Dev psu_pwr_powerSource = { /*Added as a place holder for now.*/
        .cfg = {
            /* DC_POWER_PRESENT */
            .pin_dc_present         = { &ec_io, OC_EC_DC_IN_PRESENT},
            /* POE_PRSNT_N */
            .pin_poe_prsnt_n        = { &ec_io, OC_EC_POE_IN_PRESENT},
            /* INT_BAT_PRSNT */
            .pin_int_bat_prsnt      = { &ec_io, OC_EC_EN_INT_BATT_PWR},

            .pin_disable_dc_input   = { &ec_io, OC_EC_DISABLE_DC_INPUT},
            .pin_dc_input_fault     = { &ec_io, OC_EC_DC_INPUT_FAULT},
            .pin_oc_input_present   = { &ec_io, OC_EC_OC_IN_PRESENT},
            .pin_power_off          = { &ec_io, OC_EC_POWER_OFF},
        },
        .obj = {},
};
OcGpio_Pin pin_tempsen_evt1         = { &ec_io, OC_EC_TEMP_EVENT };

SE98A_Dev psu_sensor_ts1 = {
    .cfg = {
        .dev = {
            .bus = OC_CONNECT1_I2C5,
            .slave_addr = 0x18
        },
        .pin_evt = &pin_tempsen_evt1,
    },
    .obj = {},
};

SE98A_Dev psu_sensor_ts2 = {
    .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C5,
                .slave_addr = 0x19,
            },
            .pin_evt = &pin_tempsen_evt1,
    },
    .obj = {},
};

SE98A_Dev psu_sensor_ts3 = {
    .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C5,
                .slave_addr = 0x1A,
            },
            .pin_evt = &pin_tempsen_evt1,
    },
    .obj = {},
};
/* HCI SubSystem Config */
// Buzzer
HciBuzzer_Cfg psu_hci_buzzer = {
    //.pin_en = { &, 10, OCGPIO_CFG_OUT_OD_NOPULL },
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

S_I2C_Cfg debug_I2C5 = {
    .bus = OC_CONNECT1_I2C5,
};

//Native GPIO
S_OCGPIO_Cfg debug_psu_gpio_pa = {
    .port = &ec_io,
    .group = PA,
};

S_OCGPIO_Cfg debug_psu_gpio_pb = {
    .port = &ec_io,
    .group = PB,
};

S_OCGPIO_Cfg debug_psu_gpio_pc = {
    .port = &ec_io,
    .group = PC,
};

S_OCGPIO_Cfg debug_psu_gpio_pd = {
    .port = &ec_io,
    .group = PD,
};

S_OCGPIO_Cfg debug_psu_gpio_pe = {
    .port = &ec_io,
    .group = PE,
};

S_OCGPIO_Cfg debug_psu_gpio_pf = {
    .port = &ec_io,
    .group = PF,
};

S_OCGPIO_Cfg debug_psu_gpio_pg = {
    .port = &ec_io,
    .group = PG,
};

// IO EXPANDERS
S_OCGPIO_Cfg debug_psu_ioexpander = {
    .port = &pwr_io,
};

/* Factory Configuration for the Devices*/
//Power Factory Config.
const SE98A_Config fact_bc_se98a = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
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

const INA226_Config fact_lion_12v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_fe_1v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_fe_3v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_fe_5v_ps_cfg = {
    .current_lim = 1500,
};
const INA226_Config fact_fe_12v_ps_cfg = {
    .current_lim = 2000,
};
const INA226_Config fact_fe_24v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_fe_28v_ps_cfg = {
    .current_lim = 1000,
};

const INA226_Config fact_psu_12v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_gen_12v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_gen_24v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_bb_12v_ps_cfg = {
    .current_lim = 1000,
};
const INA226_Config fact_ec_12v_ps_cfg = {
    .current_lim = 1000,
};

const INA226_Config fact_ec_3v_ps_cfg = {
    .current_lim = 1000,
};
