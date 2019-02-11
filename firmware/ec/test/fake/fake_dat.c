/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_dat.h"
OcGpio_Port fe_ch1_gain_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
        &(PCA9557_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL1_IO_TX_ATTEN_ADDR },
        },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port fe_ch1_gain_io_invalid = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
        &(PCA9557_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C4, RFFE_CHANNEL1_INVALID_SLAVE_ADDR },
        },
    .object_data = &(PCA9557_Obj){},
};

Fe_Gain_Cfg fe_ch1_gain_invalid = {
    /* CH1_TX_ATTN_16DB */
    .pin_tx_attn_16db = { &fe_ch1_gain_io_invalid, 1 },
    /* CH1_TX_ATTN_P5DB */
    .pin_tx_attn_p5db = { &fe_ch1_gain_io_invalid, 2 },
    /* CH1_TX_ATTN_1DB */
    .pin_tx_attn_1db = { &fe_ch1_gain_io_invalid, 3 },
    /* CH1_TX_ATTN_2DB */
    .pin_tx_attn_2db = { &fe_ch1_gain_io_invalid, 4 },
    /* CH1_TX_ATTN_4DB */
    .pin_tx_attn_4db = { &fe_ch1_gain_io_invalid, 5 },
    /* CH1_TX_ATTN_8DB */
    .pin_tx_attn_8db = { &fe_ch1_gain_io_invalid, 6 },
    /* CH1_TX_ATTN_ENB */
    .pin_tx_attn_enb = { &fe_ch1_gain_io_invalid, 7 },
};
