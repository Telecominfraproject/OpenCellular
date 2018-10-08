/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
// Standard Header files
//*****************************************************************************
#include "inc/devices/eth_sw.h"

#include "inc/devices/88E6071_registers.h"
#include "inc/common/global_header.h"
#include "inc/devices/mdio_bb.h"
#include "registry/SSRegistry.h"
#include "src/interfaces/Ethernet/tcp_tx_rx.h"

#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>

#define CLEAR_BIT(x, y) (y = (~x) & y)
#define SET_BIT(x, y) (y = x | y)
#define MACLOOPBACK 0
#define LINELOOPBACK 1
#define EXTLOOPBACK 2
#define TCPHANDLERSTACK 1024
#define MAX 4
#define IPSTRING_LENGTH 16
#define ETHTIVACLEINT_TASK_PRIORITY 1
char ethTivaClientTaskStack[TCPHANDLERSTACK];

static bool s_eth_sw_linkup = false;
const char *destIp;
uint8_t numRepeat;
char convStr[IPSTRING_LENGTH];
char temp[IPSTRING_LENGTH];
char *tempBuf = temp;

void eth_sw_configure(Eth_cfg *ethCfg)
{
    uint8_t link_up;
    uint16_t read_val = 0;
    if (!s_eth_sw_linkup) {
        OcGpio_configure(&ethCfg->eth_sw_cfg->pin_ec_ethsw_reset,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
        SysCtlDelay(16000000); //400ms delay
    }
    read_val = mdiobb_read_by_paging(PHY_PORT_0, REG_PHY_SPEC_STATUS);
    link_up = (RT_LINK & read_val) ? 1 : 0;
    DEBUG("ETHSW: Linkup: %d \n", link_up);
    if (link_up == 1) {
        s_eth_sw_linkup = true;
    } else {
        s_eth_sw_linkup = false;
    }
}

ePostCode eth_sw_probe(POSTData *postData)
{
    ePostCode eth_sw_found = POST_DEV_MISSING;
    uint16_t switch_pid = 0;
    uint16_t devId = 0x00;
    /*Switch idenifier*/
    switch_pid = mdiobb_read(0x8, 3);
    switch_pid = (switch_pid >> 4);
    if (switch_pid == ETH_SW_PRODUCT_ID) {
        /* Phy Identifier */
        devId = mdiobb_read_by_paging(0, REG_PHY_ID_1);
        if (devId == PHY_IDENTIFIER) {
            eth_sw_found = POST_DEV_FOUND;
        }
    }
    post_update_POSTData(postData, 0xFF, 0xFF, 0xFF, devId);
    LOGGER_DEBUG("ETHSW::INFO:: Ethernet switch  %s.\n",
                 ((eth_sw_found == POST_DEV_FOUND) ? "found" : "not found"));
    return eth_sw_found;
}

void eth_enable_interrupt()
{
    uint8_t port;
    mdiobb_write(GLOBAL_1, REG_GLOBAL_CONTROL, DEV_INT_EN);
    mdiobb_write(GLOBAL_2, REG_INTERRUPT_MASK, DEFAULT_PHY_INTS);
    for (port = 0; port < 5; port++) {
        mdiobb_write_by_paging(port, REG_PHY_INTERRUPT_EN, DEFUALT_INT);
    }
}

uint16_t get_interrupt_status(uint8_t port)
{
    /* read the register REG_PHY_INTERRUPT_EN */
    return mdiobb_read_by_paging(port, REG_PHY_INTERRUPT_STATUS);
}

/*****************************************************************************
 * Internal IRQ handler - reads in triggered interrupts and dispatches CBs
 *****************************************************************************/
static void _ethernet_sw_isr(void *context)
{
    Eth_cfg *ethCfg = context;
    uint8_t port = 0;
    uint8_t value;
    if (!ethCfg->eth_sw_cfg->eth_switch.obj.alert_cb) {
        return;
    }
    /* Confirm the interrupt*/
    uint16_t read_val = mdiobb_read(GLOBAL_2, REG_INTERRUPT_MASK);
    read_val = mdiobb_read(GLOBAL_2, REG_INTERRUPT_SOURCE);
    LOGGER_DEBUG(
            "ETHSW::INFO:: Ethernet switch Interrupt mask register shows 0x%x.\n",
            read_val);
    if (read_val & 0x1F) {
        while (!((read_val >> port) & 1)) {
            port++;
        }
    }
    LOGGER_DEBUG(
            "ETHSW::INFO:: Ethernet switch context report interrupt from 0x%x.\n",
            ethCfg->eth_sw_port);
    uint16_t interrupt_status = 0;
    uint16_t i = 0;
    Eth_Sw_Events eth_Evt;
    if (interrupt_status = get_interrupt_status(port)) {
        for (i = 0; (1 << i) != 0x10000; i++) {
            switch (interrupt_status & (1 << i)) {
                case SPEED_INT_STATUS: {
                    if (mdiobb_read_by_paging(port, REG_PHY_CONTROL) &
                        AUTONEG_EN) {
                        value = (RES_SPEED &
                                 mdiobb_read_by_paging(port,
                                                       REG_PHY_SPEC_STATUS)) ?
                                        SPEED_100M :
                                        SPEED_10M;
                    } else {
                        value = (SPEED &
                                 mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ?
                                        SPEED_100M :
                                        SPEED_10M;
                    }
                    eth_Evt = ETH_EVT_SPEED;
                } break;
                case DUPLEX_INT_STATUS: {
                    if (mdiobb_read_by_paging(port, REG_PHY_CONTROL) &
                        AUTONEG_EN) {
                        value = (RES_DUPLEX &
                                 mdiobb_read_by_paging(port,
                                                       REG_PHY_SPEC_STATUS)) ?
                                        FULL_DUPLEX :
                                        HALF_DUPLEX;
                    } else {
                        value = (DUPLEX &
                                 mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ?
                                        FULL_DUPLEX :
                                        HALF_DUPLEX;
                    }
                    eth_Evt = ETH_EVT_DUPLEX;
                } break;
                case AUTONEG_COMPLETE_INT_STATUS: {
                    read_val = mdiobb_read_by_paging(port, REG_PHY_STATUS);
                    value = (AUTONEG_DONE & read_val) ? 1 : 0;
                    eth_Evt = ETH_EVT_AUTONEG;
                } break;
                case LINK_CHANGE_INT_STATUS: {
                    read_val = mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS);
                    value = (RT_LINK & read_val) ? 1 : 0;
                    eth_Evt = ETH_EVT_LINK;
                } break;
                case MDI_CROSSOVER_INT_STATUS: {
                    read_val = mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS);
                    value = (MDI_CROSSOVER_STATUS & read_val) ? 1 : 0;
                    eth_Evt = ETH_EVT_CROSSOVER;
                } break;
                case ENERGY_DET_INT_STATUS: {
                    read_val = mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS);
                    value = (SLEEP_MODE & read_val) ? 1 : 0;
                    eth_Evt = ETH_EVT_ENERGY;
                } break;
                case POLARITY_INT_STATUS: {
                    read_val = mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS);
                    value = (POLARITY & read_val) ? 1 : 0;
                    eth_Evt = ETH_EVT_POLARITY;
                    break;
                }
                case JABBER_INT_STATUS: {
                    read_val = mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS);
                    value = (JABBER_DET & read_val) ? 1 : 0;
                    eth_Evt = ETH_EVT_JABBER;
                } break;
                default: {
                    LOGGER_ERROR("ETHSW:Unknown event type\n");
                    return;
                }
            }
            ethCfg->eth_sw_cfg->eth_switch.obj.alert_cb(
                    eth_Evt, value,
                    ethCfg->eth_sw_cfg->eth_switch.obj.cb_context);
        }
    }
}

/*****************************************************************************
 *****************************************************************************/
void eth_sw_setAlertHandler(Eth_cfg *ethCfg, Eth_Sw_CallbackFn alert_cb,
                            void *cb_context)
{
    ethCfg->eth_sw_cfg->eth_switch.obj.alert_cb = alert_cb;
    ethCfg->eth_sw_cfg->eth_switch.obj.cb_context = cb_context;
}

ePostCode eth_sw_init(Eth_cfg *ethCfg)
{
    ePostCode ret = POST_DEV_CFG_DONE;
    //TODO: Enabling of the ethernet interrupts requires some more work.
    /*
    if (ethCfg->eth_sw_cfg.pin_evt) {
        const uint32_t pin_evt_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING;
        if (OcGpio_configure(ethCfg->eth_sw_cfg.pin_evt, pin_evt_cfg) < OCGPIO_SUCCESS) {
            ret = POST_DEV_CFG_FAIL;
        } else {
            // Use a threaded interrupt to handle IRQ
            ThreadedInt_Init(ethCfg->eth_sw_cfg.pin_evt, _ethernet_sw_isr, (void *)ethCfg);
        }
    }
    */
    return ret;
}

ReturnStatus eth_sw_get_status_speed(uint8_t port, port_speed *speed)
{
    ReturnStatus ret = RETURN_OK;
    if (mdiobb_read_by_paging(port, REG_PHY_CONTROL) & AUTONEG_EN)
        *speed =
                (RES_SPEED & mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS)) ?
                        SPEED_100M :
                        SPEED_10M;
    else
        *speed = (SPEED & mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ?
                         SPEED_100M :
                         SPEED_10M;
    return ret;
}

ReturnStatus eth_sw_get_status_duplex(uint8_t port, port_duplex *duplex)
{
    ReturnStatus ret = RETURN_OK;
    if (mdiobb_read_by_paging(port, REG_PHY_CONTROL) & AUTONEG_EN)
        *duplex = (RES_DUPLEX &
                   mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS)) ?
                          FULL_DUPLEX :
                          HALF_DUPLEX;
    else
        *duplex = (DUPLEX & mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ?
                          FULL_DUPLEX :
                          HALF_DUPLEX;
    return ret;
}

ReturnStatus eth_sw_get_status_auto_neg(uint8_t port, uint8_t *autoneg_on)
{
    ReturnStatus ret = RETURN_OK;
    *autoneg_on =
            (AUTONEG_EN & mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ? 1 : 0;
    return ret;
}

ReturnStatus eth_sw_get_status_sleep_mode(uint8_t port, uint8_t *sleep_mode_en)
{
    ReturnStatus ret = RETURN_OK;
    *sleep_mode_en =
            (SLEEP_MODE & mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS)) ?
                    1 :
                    0;
    return ret;
}

ReturnStatus eth_sw_get_status_auto_neg_complete(uint8_t port,
                                                 uint8_t *autoneg_complete)
{
    ReturnStatus ret = RETURN_OK;
    *autoneg_complete =
            (AUTONEG_DONE & mdiobb_read_by_paging(port, REG_PHY_STATUS)) ? 1 :
                                                                           0;
    return ret;
}

ReturnStatus eth_sw_get_status_link_up(uint8_t port, uint8_t *link_up)
{
    ReturnStatus ret = RETURN_OK;
    uint16_t read_val = 0;
    read_val = mdiobb_read_by_paging(port, REG_PHY_SPEC_STATUS);
    *link_up = (RT_LINK & read_val) ? 1 : 0;
    return ret;
}

/* restart_autoneg - This function re-initiated autonegotiation */
ReturnStatus restart_autoneg(uint8_t port)
{
    ReturnStatus ret = RETURN_OK;
    /* write into PHY control register & in autoneg enable bit */
    mdiobb_set_bits(port, REG_PHY_CONTROL,
                    (RESTART_AUTONEG | AUTONEG_EN | SOFT_RESET));
    return ret;
}

ReturnStatus eth_sw_set_config_speed(uint8_t port, port_speed speed)
{
    ReturnStatus ret = RETURN_OK;
    uint16_t read_val = 0x0000;
    switch (speed) {
        case SPEED_10M:
            read_val = mdiobb_read_by_paging(port, REG_PHY_CONTROL);
            CLEAR_BIT((AUTONEG_EN | SPEED), read_val);
            SET_BIT(SOFT_RESET, read_val);
            mdiobb_write_by_paging(port, REG_PHY_CONTROL, read_val);
            break;
        case SPEED_100M:
            read_val = mdiobb_read_by_paging(port, REG_PHY_CONTROL);
            CLEAR_BIT(AUTONEG_EN, read_val);
            SET_BIT((SOFT_RESET | SPEED), read_val);
            mdiobb_write_by_paging(port, REG_PHY_CONTROL, read_val);
            break;
        case SPEED_AUTONEG:
            restart_autoneg(port);
            break;
        default:
            DEBUG("Invalid Ethernet speed set option");
            return RETURN_NOTOK;
    }
    return ret;
}

ReturnStatus eth_sw_set_config_duplex(uint8_t port, port_duplex duplex)
{
    ReturnStatus ret = RETURN_OK;
    uint16_t read_val = 0x0000;
    switch (duplex) {
        case SPEED_10M:
            read_val = mdiobb_read_by_paging(port, REG_PHY_CONTROL);
            CLEAR_BIT((AUTONEG_EN | DUPLEX), read_val);
            SET_BIT(SOFT_RESET, read_val);
            mdiobb_write_by_paging(port, REG_PHY_CONTROL, read_val);
            break;
        case SPEED_100M:
            read_val = mdiobb_read_by_paging(port, REG_PHY_CONTROL);
            CLEAR_BIT(AUTONEG_EN, read_val);
            SET_BIT((SOFT_RESET | DUPLEX), read_val);
            mdiobb_write_by_paging(port, REG_PHY_CONTROL, read_val);
            break;
        case SPEED_AUTONEG:
            restart_autoneg(port);
            break;
        default:
            DEBUG("Invalid Ethernet speed set option");
            return RETURN_NOTOK;
    }
    return ret;
}

ReturnStatus eth_sw_set_config_power_down(uint8_t port, uint8_t power_down)
{
    ReturnStatus ret = RETURN_OK;
    if (power_down)
        mdiobb_set_bits(port, REG_PHY_CONTROL, PWR_DOWN);
    else
        mdiobb_clear_bits(port, REG_PHY_CONTROL, PWR_DOWN);
    return ret;
}

ReturnStatus eth_sw_set_config_sleep_mode_enable(uint8_t port,
                                                 uint8_t sleep_mode_en)
{
    ReturnStatus ret = RETURN_OK;
    if (sleep_mode_en)
        mdiobb_set_bits(port, REG_PHY_SPEC_CONTROL, ENERGY_DET);
    else
        mdiobb_clear_bits(port, REG_PHY_SPEC_CONTROL, ENERGY_DET);
    return ret;
}

ReturnStatus get_interrupt(uint8_t port)
{
    /* read the register REG_PHY_INTERRUPT_EN */
    return mdiobb_read_by_paging(port, REG_PHY_INTERRUPT_EN);
}

ReturnStatus eth_sw_set_config_interrupt_enable(uint8_t port,
                                                uint8_t *interrupt_mask)
{
    ReturnStatus ret = RETURN_OK;
    uint16_t i = 0;
    uint16_t write_val = get_interrupt(port);
    for (i = 0; (1 << i) != 0x10; i++) {
        switch ((1 << i)) {
            case ETH_ALERT_SPEED_CHANGE:
                (*interrupt_mask & ETH_ALERT_SPEED_CHANGE) ?
                        (write_val |= SPEED_INT_EN) :
                        (write_val &= ~SPEED_INT_EN);
                break;
            case ETH_ALERT_DUPLEX_CHANGE:
                (*interrupt_mask & ETH_ALERT_DUPLEX_CHANGE) ?
                        (write_val |= DUPLEX_INT_EN) :
                        (write_val &= ~DUPLEX_INT_EN);
                break;
            case ETH_ALERT_AUTONEG_DONE:
                (*interrupt_mask & ETH_ALERT_AUTONEG_DONE) ?
                        (write_val |= AUTONEG_COMPLETE_INT_EN) :
                        (write_val &= ~AUTONEG_COMPLETE_INT_EN);
                break;
            case ETH_ALERT_LINK_CHANGE:
                (*interrupt_mask & ETH_ALERT_LINK_CHANGE) ?
                        (write_val |= LINK_CHANGE_INT_EN) :
                        (write_val &= ~LINK_CHANGE_INT_EN);
                break;
            case ETH_ALERT_CROSSOVER_DET:
                (*interrupt_mask & ETH_ALERT_CROSSOVER_DET) ?
                        (write_val |= MDI_CROSSOVER_INT_EN) :
                        (write_val &= ~MDI_CROSSOVER_INT_EN);
                break;
            case ETH_ALERT_ENERGY_DET:
                (*interrupt_mask & ETH_ALERT_ENERGY_DET) ?
                        (write_val |= ENERGY_DET_INT_EN) :
                        (write_val &= ~ENERGY_DET_INT_EN);
                break;
            case ETH_ALERT_POLARITY_DET:
                (*interrupt_mask & ETH_ALERT_POLARITY_DET) ?
                        (write_val |= POLARITY_INT_EN) :
                        (write_val &= ~POLARITY_INT_EN);
                break;
            case ETH_ALERT_JABBER_DET:
                (*interrupt_mask & ETH_ALERT_JABBER_DET) ?
                        (write_val |= JABBER_INT_EN) :
                        (write_val &= ~JABBER_INT_EN);
            default:
                DEBUG("Interrupt not supported");
                return RETURN_NOTOK;
        }
    }
    mdiobb_write_by_paging(port, REG_PHY_INTERRUPT_EN, write_val);
    return ret;
}

ReturnStatus eth_sw_set_config_soft_reset(uint8_t port)
{
    ReturnStatus ret = RETURN_OK;
    /* write into REG_PHY_CONTROL bit# 15 */
    mdiobb_set_bits(port, REG_PHY_CONTROL, SOFT_RESET);
    return ret;
}

ReturnStatus eth_sw_enable_loopback(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    Eth_cfg *s_eth_cfg = (Eth_cfg *)driver;
    Eth_LoopBack_Params *s_eth_lpback = (Eth_LoopBack_Params *)params;
    switch (s_eth_lpback->loopBackType) {
        case MACLOOPBACK:
            status = eth_sw_enable_macloopback(s_eth_cfg->eth_sw_port);
            break;
        /*TODO: Implementation to be done for Line and External Loopback*/
        case LINELOOPBACK:
        case EXTLOOPBACK:
        default:
            break;
    }
    return status;
}

ReturnStatus eth_sw_disable_loopback(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    Eth_cfg *s_eth_cfg = (Eth_cfg *)driver;
    Eth_LoopBack_Params *s_eth_lpback = (Eth_LoopBack_Params *)params;
    switch (s_eth_lpback->loopBackType) {
        case MACLOOPBACK:
            status = eth_sw_disable_macloopback(s_eth_cfg->eth_sw_port);
            break;
        /*TODO: Implementation to be done for Line and External Loopback*/
        case LINELOOPBACK:
        case EXTLOOPBACK:
        default:
            break;
    }
    return status;
}

ReturnStatus eth_sw_enable_macloopback(uint8_t port)
{
    ReturnStatus ret = RETURN_OK;
    /*For MacLoopback, autonegotiation must be disabled*/
    mdiobb_clear_bits(port, REG_PHY_CONTROL, AUTONEG_EN);
    /*Set Loopback bit in PHY control register*/
    mdiobb_set_bits(port, REG_PHY_CONTROL, LOOPBACK_EN);
    return ret;
}

ReturnStatus eth_sw_disable_macloopback(uint8_t port)
{
    ReturnStatus ret = RETURN_OK;
    mdiobb_clear_bits(port, REG_PHY_CONTROL, LOOPBACK_EN);
    return ret;
}

ReturnStatus eth_sw_enable_packet_gen(void *driver, void *params)
{
    ReturnStatus ret = RETURN_OK;
    Eth_cfg *s_eth_cfg = (Eth_cfg *)driver;
    Eth_PacketGen_Params *s_eth_packetParams = (Eth_PacketGen_Params *)params;
    /*Packet generator params such as packet length, payload type, frame count etc are set in REG_C45_PACKET_GEN*/
    mdiobb_write_by_paging_c45(s_eth_cfg->eth_sw_port, REG_C45_PACKET_GEN,
                               s_eth_packetParams->reg_value);
    return ret;
}

ReturnStatus eth_sw_disable_packet_gen(void *driver)
{
    ReturnStatus ret = RETURN_OK;
    Eth_cfg *s_eth_cfg = (Eth_cfg *)driver;
    mdiobb_clear_bits_C45(s_eth_cfg->eth_sw_port, REG_C45_PACKET_GEN,
                          PACKET_GEN_EN);
    return ret;
}

char *convString(int i, char *result)
{
    sprintf(result, "%d", i);
    return result;
}

ReturnStatus eth_sw_config_tiva_client(void *driver, void *params)
{
    ReturnStatus ret = RETURN_OK;
    int count = 0;
    Eth_cfg *s_eth_cfg = (Eth_cfg *)driver;
    Eth_TcpClient_Params *s_eth_tcpParams = (Eth_TcpClient_Params *)params;

    Task_Handle taskHandle_client;
    Task_Params taskParams;
    Error_Block eb;

    /* Make sure Error_Block is initialized */
    Error_init(&eb);

    /*Convert 4 bytes received from host to a proper Ipv4 address*/
    memset(&convStr, '\0', IPSTRING_LENGTH);
    do {
        convString(s_eth_tcpParams->ipAddress[count], tempBuf);
        strcat(convStr, tempBuf);
        if ((MAX - 1) != count) {
            strcat(convStr, ".");
        }
        count++;
    } while (count < MAX);
    destIp = convStr;
    numRepeat = s_eth_tcpParams->repeat;
    /*
     *  Create the Task that farms outgoing TCP connection.
     *  arg0 will be the port that this task sends the test data to.
     */
    Task_Params_init(&taskParams);
    taskParams.stack = ethTivaClientTaskStack;
    taskParams.stackSize = TCPHANDLERSTACK;
    taskParams.priority = ETHTIVACLEINT_TASK_PRIORITY;
    taskParams.arg0 = s_eth_tcpParams->tcpPort;

    taskHandle_client =
            Task_create((Task_FuncPtr)tcpHandler_client, &taskParams, &eb);
    if (taskHandle_client == NULL) {
        System_printf("Failed to create taskHandle_client Task\n");
    }
    System_flush();
    return ret;
}

ReturnStatus eth_sw_set_config_restart_neg(uint8_t port)
{
    ReturnStatus ret = RETURN_OK;
    /* write into PHY control register & in autoneg enable bit */
    mdiobb_set_bits(port, REG_PHY_CONTROL,
                    (RESTART_AUTONEG | AUTONEG_EN | SOFT_RESET));
    return ret;
}

ReturnStatus eth_sw_get_config_speed(uint8_t port, port_speed *speed)
{
    ReturnStatus ret = RETURN_OK;
    if (AUTONEG_EN & mdiobb_read_by_paging(port, REG_PHY_CONTROL))
        *speed = SPEED_AUTONEG;
    else
        *speed = SPEED & mdiobb_read_by_paging(port, REG_PHY_CONTROL) ?
                         SPEED_100M :
                         SPEED_10M;
    return ret;
}

ReturnStatus eth_sw_get_config_duplex(uint8_t port, port_duplex *duplex)
{
    ReturnStatus ret = RETURN_OK;
    if (AUTONEG_EN & mdiobb_read_by_paging(port, REG_PHY_CONTROL))
        *duplex = DUPLEX_AUTONEG;
    else
        *duplex = (DUPLEX & mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ?
                          FULL_DUPLEX :
                          HALF_DUPLEX;
    return ret;
}

ReturnStatus eth_sw_get_config_power_down(uint8_t port, uint8_t *power_dwn)
{
    ReturnStatus ret = RETURN_OK;
    *power_dwn =
            (PWR_DOWN & mdiobb_read_by_paging(port, REG_PHY_CONTROL)) ? 1 : 0;
    return ret;
}

ReturnStatus eth_sw_get_config_sleep_mode(uint8_t port, uint8_t *sleep_mode)
{
    ReturnStatus ret = RETURN_OK;
    *sleep_mode =
            (ENERGY_DET & mdiobb_read_by_paging(port, REG_PHY_SPEC_CONTROL)) ?
                    1 :
                    0;
    return ret;
}

ReturnStatus eth_sw_get_config_interrupt_enable(uint8_t port,
                                                uint8_t *interrupt_enb)
{
    ReturnStatus ret = RETURN_OK;
    /* read the register REG_PHY_INTERRUPT_EN */
    *interrupt_enb = mdiobb_read_by_paging(port, REG_PHY_INTERRUPT_EN);
    return ret;
}
