/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/ocmp_wrappers/ocmp_debugmdio.h"

#include "common/inc/global/ocmp_frame.h"
#include "common/inc/global/Framework.h"
#include "inc/common/global_header.h"
#include "inc/devices/88E6071_registers.h"
#include "inc/devices/debug_ocmdio.h"

#include "inc/devices/mdio_bb.h"

#define CLAUSE_45_REQUEST(reg_address) check_clause_45(s_ocmdio->reg_address)
#define PORT_REG_REQUEST(port) check_clause_22(s_oc_mdio_cfg->port)
#define PHY_PORT_MAX 5

bool check_clause_45(uint16_t regAddr)
{
    bool status = false;
    if (REG_C45_PACKET_GEN == regAddr || REG_C45_CRC_ERROR_COUNTER == regAddr)
        status = true;
    return status;
}

bool check_clause_22(uint8_t port)
{
    bool status = false;
    if (port < PHY_PORT_MAX)
        status = true;
    return status;
}

bool mdio_read(void *mdio_cfg, void *ocmdio)
{
    S_MDIO_Cfg *s_oc_mdio_cfg = (S_MDIO_Cfg *)mdio_cfg;
    S_OCMDIO *s_ocmdio = (S_OCMDIO *)ocmdio;
    s_ocmdio->reg_value = 0xf00f;

    if (CLAUSE_45_REQUEST(reg_address))
        /*PHY registers use Reg 13 and Reg 14 as paging mechanism to access Clause 45 registers*/
        s_ocmdio->reg_value = mdiobb_read_by_paging_c45(s_oc_mdio_cfg->port,
                                                        s_ocmdio->reg_address);
    else if (PORT_REG_REQUEST(port))
        /*PHY registers use Reg 13 and Reg 14 as paging mechanism to access Clause 22 registers*/
        s_ocmdio->reg_value = mdiobb_read_by_paging(s_oc_mdio_cfg->port,
                                                    s_ocmdio->reg_address);
    else
        /*GLOBAL and SWITCH registers can be accessed directly*/
        s_ocmdio->reg_value =
                mdiobb_read(s_oc_mdio_cfg->port, s_ocmdio->reg_address);
    return 0;
}

bool mdio_write(void *mdio_cfg, void *ocmdio)
{
    S_MDIO_Cfg *s_oc_mdio_cfg = (S_MDIO_Cfg *)mdio_cfg;
    S_OCMDIO *s_ocmdio = (S_OCMDIO *)ocmdio;

    if (CLAUSE_45_REQUEST(reg_address)) {
        /*PHY registers use Reg 13 and Reg 14 as paging mechanism to access Clause 45 registers*/
        mdiobb_write_by_paging_c45(s_oc_mdio_cfg->port, s_ocmdio->reg_address,
                                   s_ocmdio->reg_value);
        s_ocmdio->reg_value = mdiobb_read_by_paging_c45(s_oc_mdio_cfg->port,
                                                        s_ocmdio->reg_address);
    } else if (PORT_REG_REQUEST(port)) {
        /*PHY registers use Reg 13 and Reg 14 as paging mechanism to access Clause 22 registers*/
        mdiobb_write_by_paging(s_oc_mdio_cfg->port, s_ocmdio->reg_address,
                               s_ocmdio->reg_value);
        s_ocmdio->reg_value = mdiobb_read_by_paging(s_oc_mdio_cfg->port,
                                                    s_ocmdio->reg_address);
    } else {
        /*GLOBAL and SWITCH registers can be accessed directly*/
        mdiobb_write(s_oc_mdio_cfg->port, s_ocmdio->reg_address,
                     s_ocmdio->reg_value);
        s_ocmdio->reg_value =
                mdiobb_read(s_oc_mdio_cfg->port, s_ocmdio->reg_address);
    }
    return 0;
}
