/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ltc4015.h"
#include <stdint.h>

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

bool LTC4015_GpioPins[] = {
    [0x05] = 0x1,
};

uint32_t LTC4015_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

extern const OcGpio_FnTable GpioSX1509_fnTable;

OcGpio_Port gbc_io_1 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { 0, 0x45 },
        .pin_irq = NULL,
    },
    .object_data = &(SX1509_Obj){},
};

/* Invalid Device */
LTC4015_Dev gbc_pwr_invalid_dev = {
    .cfg = {
        .i2c_dev = {
            .bus = 2,
            .slave_addr = 0x52,
        },
        .chem = 0,
        .r_snsb = 30,
        .r_snsi = 7,
        .cellcount = 3,
        .pin_lt4015_i2c_sel = { &gbc_io_1, 2, 32 },
    },
};

/* Invalid Bus */
LTC4015_Dev gbc_pwr_invalid_bus = {
    .cfg = {
        .i2c_dev = {
            .bus = 0xFF,
            .slave_addr = 0x52,
        },
        .chem = 0,
        .r_snsb = 30,
        .r_snsi = 7,
        .cellcount = 3,
        .pin_lt4015_i2c_sel = { &gbc_io_1, 2, 32 },
    },
};

/* ======================== Constants & variables =========================== */
uint16_t LTC4015_regs[] = {
    [0x01] = 0x00, /* Battery voltage low alert limit */
    [0x02] = 0x00, /* Battery voltage high alert limit */
    [0x03] = 0x00, /* Input voltage low alert limit */
    [0x04] = 0x00, /* Input voltage high alert limit */
    [0x05] = 0x00, /* Output voltage low alert limit */
    [0x06] = 0x00, /* Output voltage high alert limit */
    [0x07] = 0x00, /* Input current high alert limit */
    [0x08] = 0x00, /* Charge current low alert limit */
    [0x09] = 0x00, /* Die temperature high alert limit */
    [0x0A] = 0x00, /* Battery series resistance high alert limit */
    [0x0B] = 0x00, /* Thermistor ratio high(cold battery) alert limit */
    [0x0C] = 0x00, /* Thermistor ratio low(hot battery) alert limit */
    [0x0D] = 0x00, /* Enable limit monitoring and alert notification */
    [0x0E] = 0x00, /* Enable charger state alert notification */
    [0x0F] = 0x00, /* Enable charger status alert notification */
    [0x10] = 0x00, /* Columb counter QCOUNT low alert limit */
    [0x11] = 0x00, /* Columb counter QCOUNT high alert limit */
    [0x12] = 0x00, /* Columb counter prescale factor */
    [0x13] = 0x00, /* Columb counter value */
    [0x14] = 0x00, /* Configuration Settings */
    [0x15] = 0x00, /* Input current limit setting */
    [0x16] = 0x00, /* UVCLFB input buffer limit */
    [0x17] = 0x00, /* Reserved */
    [0x18] = 0x00, /* Reserved */
    [0x19] = 0x00, /* Arm ship mode */
    [0x1A] = 0x00, /* Charge current target */
    [0x1B] = 0x00, /* Charge voltage target */
    [0x1C] = 0x00, /* Low IBAT Threshold for C/x termination */
    [0x1D] = 0x00, /* Time in seconds with battery charger in the CV state before timer termination */
    [0x1E] = 0x00, /* Time in seconds before a max_charge_time fault is declared */
    [0x1F] = 0x00, /* JEITA_T1 */
    [0x20] = 0x00, /* JEITA_T2 */
    [0x21] = 0x00, /* JEITA_T3 */
    [0x22] = 0x00, /* JEITA_T4 */
    [0x23] = 0x00, /* JEITA_T5 */
    [0x24] = 0x00, /* JEITA_T6 */
    [0x25] = 0x00, /* VCHARGE values JEITA_6_5 */
    [0x26] = 0x00, /* VCHARGE values JEITA_4_3_2 */
    [0x27] = 0x00, /* ICHARGE_TARGET values JEITA_6_5 */
    [0x28] = 0x00, /* ICHARGE_TARGET values JEITA_4_3_2 */
    [0x29] = 0x00, /* Battery charger cfguration settings */
    [0x2A] = 0x00, /* LiFePO4/lead-acid absorb voltage adder */
    [0x2B] = 0x00, /* Maximum time for LiFePO4/lead-acid absorb charge */
    [0x2C] = 0x00, /* Lead-acid equalize charge voltage adder */
    [0x2D] = 0x00, /* Lead-acid equalization time */
    [0x2E] = 0x00, /* LiFeP04 recharge threshold */
    [0x2F] = 0x00, /* Reserved */
    [0x30] = 0x00, /* Max Charge Timer for lithium chemistries */
    [0x31] = 0x00, /* Constant-voltage regulation for lithium chemistries */
    [0x32] = 0x00, /* Absorb Timer for LiFePO4 and lead-acid batteries */
    [0x33] = 0x00, /* Eqaulize Timer for lead-acid batteries */
    [0x34] = 0x00, /* Real time battery charger state indicator */
    [0x35] = 0x00, /* Charge status indicator */
    [0x36] = 0x00, /* Limit alert register */
    [0x37] = 0x00, /* Charger state alert register */
    [0x38] = 0x00, /* Charger status alert indicator */
    [0x39] = 0x00, /* Real time system status indicator */
    [0x3A] = 0x00, /* VBAT value(BATSENS) */
    [0x3B] = 0x00, /* VIN */
    [0x3C] = 0x00, /* VSYS */
    [0x3D] = 0x00, /* Battery current(IBAT) */
    [0x3E] = 0x00, /* Input Current(IIN) */
    [0x3F] = 0x00, /* Die temperature */
    [0x40] = 0x00, /* NTC thermistor ratio */
    [0x41] = 0x00, /* Battery series resistance */
    [0x42] = 0x00, /* JEITA temperature region of the NTC thermistor (Li Only) */
    [0x43] = 0x00, /* CHEM and CELLS pin settings */
    [0x44] = 0x00, /* Charge current control DAC control bits */
    [0x45] = 0x00, /* Charge voltage control DAC control bits */
    [0x46] = 0x00, /* Input current limit control DAC control word */
    [0x47] = 0x00, /* Digitally filtered battery voltage */
    [0x48] = 0x00, /* Value of IBAT (0x3D) used in calculating BSR */
    [0x49] = 0x00, /* Reserved */
    [0x4A] = 0x00, /* Measurement valid bit */
};
