/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/common/bigbrother.h"

#include "Board.h"
#include "comm/gossiper.h"
#include "common/inc/global/ocmp_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/post.h"
#include "inc/common/system_states.h"
#include "inc/subsystem/hci/hci_buzzer.h"
#include "inc/utils/ocmp_util.h"
#include "registry/SSRegistry.h"

#include <ti/sysbios/BIOS.h>

#include <stdlib.h>

/* Global Task Configuration Variables */
Task_Struct bigBrotherTask;
Char bigBrotherTaskStack[BIGBROTHER_TASK_STACK_SIZE];

eSubSystemStates oc_sys_state = SS_STATE_PWRON;
extern POSTData PostResult[30];
//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Queue object */
/*
 * Semaphore for the Big Brother task where it will be waiting on.
 * Sub systems or Gossiper post this with respective queues filled in.
 */
Semaphore_Handle semBigBrotherMsg;

static Queue_Struct bigBrotherRxMsg;
static Queue_Struct bigBrotherTxMsg;

/*
 * bigBrotherRxMsgQueue - Used by the gossiper to pass the frame once recived
 * from Ethernet or UART and processed and needs to forward to bigBrother.
 */
Queue_Handle bigBrotherRxMsgQueue;
/*
 * bigBrotherTxMsgQueue - Used by the BigBrother to pass the frame once
 * underlying subsystem processed it (GPP/RF etc) and need to send back
 * to Gosipper. This is the one all the subsystem will be listening only.
 */
Queue_Handle bigBrotherTxMsgQueue;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
static void bigbrother_taskfxn(UArg a0, UArg a1);
static void bigbrother_init(void);
static ReturnStatus bigbrother_process_rx_msg(uint8_t *pMsg);
static ReturnStatus bigbrother_process_tx_msg(uint8_t *pMsg);
extern void post_createtask(void);
static void bigborther_initiate_post(void);

extern void gossiper_createtask(void);
extern void usb_rx_createtask(void);
extern void usb_tx_createtask(void);
extern void uartdma_rx_createtask(void);
extern void uartdma_tx_createtask(void);
extern void ebmp_create_task(void);
extern void watchdog_create_task(void);
;

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_process_tx_msg
 **
 **    DESCRIPTION     : Processes the big brother outgoing messages.
 **
 **    ARGUMENTS       : Pointer to BIGBROTHER_TXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus bigbrother_process_tx_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("BIGBROTHER:INFO:: Processing Big Brother TX Message.\n");
    if (pMsg != NULL) {
        Util_enqueueMsg(gossiperTxMsgQueue, semGossiperMsg, (uint8_t *)pMsg);
    } else {
        LOGGER_ERROR("BIGBROTHER::ERROR::No Valid Pointer.\n");
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : uart_enable
 **
 **    DESCRIPTION     : Enable UART link between EC and AP.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/

extern OcGpio_Pin pin_uart_sel;
ReturnStatus uart_enable()
{
    if (OcGpio_write(&pin_uart_sel, true) < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }
    return RETURN_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_process_rx_msg
 **
 **    DESCRIPTION     : Processes the big brother incoming messages.
 **
 **    ARGUMENTS       : Pointer to BIGBROTHER_RXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus bigbrother_process_rx_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("BIGBROTHER:INFO:: Processing Big Brother RX Message.\n");
    OCMPMessageFrame *pOCMPMessageFrame = (OCMPMessageFrame *)pMsg;
    if (pOCMPMessageFrame != NULL) {
        LOGGER_DEBUG("BIGBROTHER:INFO:: RX Msg recieved with Length: 0x%x,"
                     "Interface: 0x%x, Seq.No: 0x%x, TimeStamp: 0x%x.\n",
                     pOCMPMessageFrame->header.ocmpFrameLen,
                     pOCMPMessageFrame->header.ocmpInterface,
                     pOCMPMessageFrame->header.ocmpSeqNumber,
                     pOCMPMessageFrame->header.ocmpTimestamp);
        // Forward this to respective subsystem.
        if (!SSRegistry_sendMessage(pOCMPMessageFrame->message.subsystem,
                                    pMsg)) {
            LOGGER_ERROR("BIGBROTHER::ERROR::Subsystem %d doesn't exist\n",
                         pOCMPMessageFrame->message.subsystem);
            free(pMsg);
        }
    } else {
        LOGGER_ERROR("BIGBROTHER:ERROR:: No message recieved.\n");
        free(pMsg);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : bigborther_initiate_post
 **
 **    DESCRIPTION     : Creates POST test task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void bigborther_initiate_post(void)
{
    LOGGER_DEBUG("BIGBROTHER:INFO::Creating task to perform POST.\n");
    post_createtask();
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_ioexp_init
 **
 **    DESCRIPTION     : Initializes Io expander SX1509.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/

extern OcGpio_Port gbc_io_1;
extern OcGpio_Port gbc_io_0;

/* These pins aren't properly referenced in a subsystem yet, so we'll define
 * them here for now */

//OcGpio_Pin pin_r_irq_intrpt         = { &gbc_io_0, 0, OCGPIO_CFG_IN_PU };
//OcGpio_Pin pin_inven_eeprom_wp      = { &gbc_io_0, 1, OCGPIO_CFG_OUT_OD_NOPULL };
//OcGpio_Pin pin_s_id_eeprom_wp       = { &gbc_io_0, 2, OCGPIO_CFG_OUT_OD_NOPULL };
OcGpio_Pin pin_uart_sel = { &gbc_io_0, 3, OCGPIO_CFG_OUT_OD_NOPULL };
//OcGpio_Pin pin_tempsen_evt1         = { &gbc_io_0, 4 };
//OcGpio_Pin pin_tempsen_evt2         = { &gbc_io_0, 5 };
//OcGpio_Pin pin_tempsen_evt3         = { &gbc_io_0, 6 };
//OcGpio_Pin pin_tempsen_evt4         = { &gbc_io_0, 7 };
//OcGpio_Pin pin_tempsen_evt5         = { &gbc_io_0, 8 };
//OcGpio_Pin pin_buzzer_on            = { &gbc_io_0, 10, OCGPIO_CFG_OUT_OD_NOPULL };
//OcGpio_Pin pin_int_bat_prsnt        = { &gbc_io_0, 11 };
//OcGpio_Pin pin_ext_bat_prsnt        = { &gbc_io_0, 12 };
OcGpio_Pin pin_ec_syncconn_gpio1 = { &gbc_io_0, 13, OCGPIO_CFG_OUT_OD_NOPULL };
OcGpio_Pin pin_eth_sw_ec_intn = { &gbc_io_0, 14 };

OcGpio_Pin pin_v5_a_pgood = { &gbc_io_1, 3, OCGPIO_CFG_IN_PU };

extern OcGpio_Port sync_io;
extern OcGpio_Port sdr_fx3_io;
extern OcGpio_Port fe_ch1_gain_io;
extern OcGpio_Port fe_ch2_gain_io;
extern OcGpio_Port fe_ch1_lna_io;
extern OcGpio_Port fe_ch2_lna_io;
extern OcGpio_Port fe_watchdog_io;

ReturnStatus bigbrother_ioexp_init(void)
{
    ReturnStatus status = RETURN_OK;

    /* Initialize IO Expander SX1509 GPIO Pins - 0x70 */
    /* IO pins - 0x70
     * IO0  - 2G_SIM_PRESENCE        - IN
     * IO1  - 2GMODULE_POWEROFF      - OUT
     * IO2  - EC_2GMODULE_PWR_ON   - OUT
     * IO3  - V5_A_PGOOD             - IN
     * IO4  - EC_LT4015_I2C_SEL    - OUT
     * IO5  - NA
     * IO6  - NA
     * IO7  - NA
     * IO8  - NA
     * IO9  - NA
     * IO10 - NA
     * IO11 - NA
     * IO12 - NA
     * IO13 - NA
     * IO14 - NA
     * IO15 - NA
    */

    /* TODO: we need a better spot to init. our IO expanders, but this works
     * for now
     */
    OcGpio_init(&gbc_io_1);
    OcGpio_init(&sync_io);
    OcGpio_init(&sdr_fx3_io);
    OcGpio_init(&fe_ch1_gain_io);
    OcGpio_init(&fe_ch2_gain_io);
    OcGpio_init(&fe_ch1_lna_io);
    OcGpio_init(&fe_ch2_lna_io);
    OcGpio_init(&fe_watchdog_io);

    /* Initialize pins that aren't covered yet by a subsystem */
    OcGpio_configure(&pin_v5_a_pgood, OCGPIO_CFG_INPUT);

    /* Initialize IO Expander SX1509 GPIO Pins - 0x71 */
    /* IO pins - 0x71
     * IO0  - R_IRQ_INTRPT            - IN
     * IO1  - IO_EXP_INVEN_EEPROM_WP  - OUT
     * IO2  - IO_EXP_S_ID_EEPROM_WP   - OUT
     * IO3  - UART_SEL                - OUT
     * IO4  - TEMPSEN_IOEXP_EVNT1     - IN
     * IO5  - TEMPSEN_IOEXP_EVNT2     - IN
     * IO6  - TEMPSEN_IOEXP_EVNT3     - IN
     * IO7  - TEMPSEN_IOEXP_EVNT4     - IN
     * IO8  - TEMPSEN_IOEXP_EVNT5     - IN
     * IO9  - NA
     * IO10 - BUZZER_ON               - OUT
     * IO11 - INT_BAT_PRSNT           - IN
     * IO12 - EXT_BAT_PRSNT           - IN
     * IO13 - EC_SYNCCONN_GPIO1     - OUT
     * IO14 - ETH_SW_EC_INTN        - IN
     * IO15 - NA
     */
    OcGpio_init(&gbc_io_0);

    //OcGpio_configure(&pin_r_irq_intrpt, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_inven_eeprom_wp, OCGPIO_CFG_OUTPUT);
    //OcGpio_configure(&pin_s_id_eeprom_wp, OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&pin_uart_sel, OCGPIO_CFG_OUTPUT);
    //OcGpio_configure(&pin_tempsen_evt1, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_tempsen_evt2, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_tempsen_evt3, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_tempsen_evt4, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_tempsen_evt5, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_buzzer_on, OCGPIO_CFG_OUTPUT);
    //OcGpio_configure(&pin_int_bat_prsnt, OCGPIO_CFG_INPUT);
    //OcGpio_configure(&pin_ext_bat_prsnt, OCGPIO_CFG_INPUT);
    OcGpio_configure(&pin_ec_syncconn_gpio1, OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&pin_eth_sw_ec_intn, OCGPIO_CFG_INPUT);

    return status;
}

/*******************************************************************************
 **    FUNCTION NAME   : bigborther_spwan_task
 **
 **    DESCRIPTION     : Application task start up point for open cellular.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
static void bigborther_spwan_task(void)
{
    /* Read OC UID EEPROM */

    /* Check the list for possible devices connected. */

    /* Launches other tasks */
    usb_rx_createtask(); // P - 05
    usb_tx_createtask(); // P - 04
    gossiper_createtask(); // P - 06
    ebmp_create_task();
    watchdog_create_task();

    /* Initialize subsystem interface to set up interfaces and launch
     * subsystem tasks */
    SSRegistry_init();
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_init
 **
 **    DESCRIPTION     : Initializes the Big Brother task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void bigbrother_init(void)
{
    /*Creating Semaphore for RX Message Queue*/
    semBigBrotherMsg = Semaphore_create(0, NULL, NULL);
    if (semBigBrotherMsg == NULL) {
        LOGGER_ERROR(
                "BIGBROTHER:ERROR::BIGBROTHER RX Semaphore creation failed.\n");
    }
    /*Creating RX Message Queue*/
    bigBrotherRxMsgQueue = Util_constructQueue(&bigBrotherRxMsg);
    LOGGER_DEBUG(
            "BIGBROTHER:INFO::Constructing message Queue for 0x%x Big Brother RX Messages.\n",
            bigBrotherRxMsgQueue);

    /*Creating TX Message Queue*/
    bigBrotherTxMsgQueue = Util_constructQueue(&bigBrotherTxMsg);
    LOGGER_DEBUG(
            "BIGBROTHER:INFO::Constructing message Queue for 0x%x Big Brother RX Messages.\n",
            bigBrotherTxMsgQueue);
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_taskfxn
 **
 **    DESCRIPTION     : handles the system state and subsystem states.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void bigbrother_taskfxn(UArg a0, UArg a1)
{
    bigbrother_init();

    /* Initialize GPIO Expander SX1509 */
    bigbrother_ioexp_init();
    hci_buzzer_beep(1);

    //Create Tasks.
    bigborther_spwan_task();
    //Perform POST
    bigborther_initiate_post();
    while (true) {
        if (Semaphore_pend(semBigBrotherMsg, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(bigBrotherRxMsgQueue)) {
                uint8_t *pWrite =
                        (uint8_t *)Util_dequeueMsg(bigBrotherRxMsgQueue);
                if (pWrite) {
                    bigbrother_process_rx_msg(pWrite);
                }
            }
            while (!Queue_empty(bigBrotherTxMsgQueue)) {
                uint8_t *pWrite =
                        (uint8_t *)Util_dequeueMsg(bigBrotherTxMsgQueue);
                if (pWrite) {
                    bigbrother_process_tx_msg(pWrite);
                }
            }
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_createtask
 **
 **    DESCRIPTION     : Creates task for Big Brother task
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void bigbrother_createtask(void)
{
    Task_Params taskParams;
    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = bigBrotherTaskStack;
    taskParams.stackSize = BIGBROTHER_TASK_STACK_SIZE;
    taskParams.priority = BIGBROTHER_TASK_PRIORITY;
    Task_construct(&bigBrotherTask, bigbrother_taskfxn, &taskParams, NULL);
    LOGGER_DEBUG("BIGBROTHER:INFO::Creating a BigBrother task.\n");
}
