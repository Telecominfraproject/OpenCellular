/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "src/registry/Framework.h"

#include "inc/common/global_header.h"
#include "inc/common/ocmp_frame.h"
#include "inc/devices/ocmp_wrappers/ocmp_i2c.h"

/* TI-RTOS driver files */
#include <ti/drivers/I2C.h>

static bool i2c_read(S_I2C_Cfg* i2c_cfg, S_OCI2C *s_oci2c )
{
    I2C_Handle i2cHandle =  i2c_open_bus(i2c_cfg->bus);
    return (i2c_reg_read(i2cHandle, s_oci2c->slaveAddress, s_oci2c->reg_address, &s_oci2c->reg_value, s_oci2c->number_of_bytes) == RETURN_OK);
}

static bool i2c_write(S_I2C_Cfg* i2c_cfg, S_OCI2C *s_oci2c )
{
    I2C_Handle i2cHandle =  i2c_open_bus(i2c_cfg->bus);
    return (i2c_reg_write(i2cHandle, s_oci2c->slaveAddress, s_oci2c->reg_address, s_oci2c->reg_value, s_oci2c->number_of_bytes) == RETURN_OK);
}

static ePostCode _probe(void *driver)
{
    //Dummy functions.
    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    //Dummy functions.
    return POST_DEV_CFG_DONE;
}

const Driver OC_I2C = {
    .name = "OC_I2C",
    .argList = (Parameter[]){
        { .name = "slave_address", .type = TYPE_UINT8 },
        { .name = "no_of_bytes", .type = TYPE_UINT8 },
        { .name = "reg_address", .type = TYPE_UINT8 },
        { .name = "reg_values", .type = TYPE_UINT16 },
        {}
    },
    .commands = {
         [OCMP_AXN_TYPE_GET] = &(Command){
              .name = "read",
              .cb_cmd = i2c_read,
         },
         [OCMP_AXN_TYPE_SET] = &(Command){
             .name = "write",
             .cb_cmd = i2c_write,
         },
         &(Command){}
    },
    /* Message handlers */
    //.cb_probe = _probe,
    //.cb_init = _init,
};
