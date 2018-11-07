/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/ocmp_wrappers/ocmp_debugi2c.h"

#include "common/inc/global/ocmp_frame.h"
#include "common/inc/global/Framework.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"
#include "inc/devices/debug_oci2c.h"

/* TI-RTOS driver files */
#include <ti/drivers/I2C.h>

bool i2c_read(void *i2c_cfg, void *oci2c)
{
    S_I2C_Cfg *s_oc_i2c_cfg = (S_I2C_Cfg *)i2c_cfg;
    S_OCI2C *s_oci2c = (S_OCI2C *)oci2c;
    I2C_Handle i2cHandle = i2c_open_bus(s_oc_i2c_cfg->bus);
    return (i2c_reg_read(i2cHandle, s_oci2c->slaveAddress, s_oci2c->reg_address,
                         &s_oci2c->reg_value,
                         s_oci2c->number_of_bytes) == RETURN_OK);
}

bool i2c_write(void *i2c_cfg, void *oci2c)
{
    S_I2C_Cfg *s_oc_i2c_cfg = (S_I2C_Cfg *)i2c_cfg;
    S_OCI2C *s_oci2c = (S_OCI2C *)oci2c;
    I2C_Handle i2cHandle = i2c_open_bus(s_oc_i2c_cfg->bus);
    return (i2c_reg_write(i2cHandle, s_oci2c->slaveAddress,
                          s_oci2c->reg_address, s_oci2c->reg_value,
                          s_oci2c->number_of_bytes) == RETURN_OK);
}
