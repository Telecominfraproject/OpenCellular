/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "inc/common/global_header.h"
#include "inc/devices/eth_sw.h"
#include "inc/devices/88E6071_registers.h"
#include "inc/subsystem/ethernet/ethernetSS.h"
#include "registry/SSRegistry.h"

#include <ti/sysbios/BIOS.h>

#define ETHSW_TASK_PRIORITY     2
#define ETHSW_TASK_STACK_SIZE   2048

/* Global Task Configuration Variables */
static Char eth_sw_task_stack[ETHSW_TASK_STACK_SIZE];

extern void *sys_config[];
#define ETH ((Eth_Cfg *)sys_config[OC_SS_ETH_SWT])

OCSubsystem ssEth = {
    .taskStackSize = ETHSW_TASK_STACK_SIZE,
    .taskPriority = ETHSW_TASK_PRIORITY,
    .taskStack = eth_sw_task_stack,
};

void ethernet_switch_setup()
{
    const Eth_Sw_Cfg *cfg = sys_config[OC_SS_ETH_SWT];
    uint8_t link_up;
    uint16_t read_val = 0;
    uint8_t index  = 0;
    for (index = 0; index < 10; index++) {
        read_val = mdiobb_read_data(PHY_PORT_0, REG_PHY_SPEC_STATUS);
        link_up = (RT_LINK & read_val) ? 1 : 0;
        DEBUG("ETHSW: Linkup: %d, index: %d\n", link_up, index);
        if (link_up == 1) {
            break;
        }
        OcGpio_write(&cfg->pin_ec_ethsw_reset, false);
        SysCtlDelay(16000000); //400ms delay
        OcGpio_write(&cfg->pin_ec_ethsw_reset, true);

        SysCtlDelay(80000000); //5 seconds delay
    }
}

bool eth_sw_pre_init(void *unused)
{
    ethernet_switch_setup();
    return true;
}
