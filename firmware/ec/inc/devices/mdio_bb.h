/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef MDIO_BB_H_
#define MDIO_BB_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include <stdint.h>

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void mdiobb_set_bits(int smi_device, int reg_addr, int datamask);
void mdiobb_clear_bits(int smi_device, int reg_addr, int datamask);
int mdiobb_read_data(int smi_device, int reg_addr);
void mdiobb_write_data(int smi_device, int reg_addr, int data);
int mdiobb_write(int phy, int reg, uint16_t val);
int mdiobb_read(int phy, unsigned int reg);

#endif /* MDIO_BB_H_ */
