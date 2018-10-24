/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* OC includes */
#include <ocwdg_daemon.h>
#include <logger.h>

#define OCWDG_NUMBER_ZERO   0
#define OCWDG_NUMBER_ONE    1

extern int32_t ocmw_sem_wait_nointr(sem_t *sem);

/******************************************************************************
 * Function Name    : ocwdg_init
 * Description      : This Function used to initialize the watchdog semaphore
 *                    and thread.
 * Input(s)         :
 * Output(s)        :
 ******************************************************************************/
int32_t ocwdg_init(void)
{
    pthread_t ocWdgThreadid;
    int32_t ret = OCWDG_NUMBER_ZERO;

    printf("inside ocwdg_init() \n");
    ret = sem_init(&semEcWdgMsg, OCWDG_NUMBER_ZERO, OCWDG_NUMBER_ZERO);
    if (ret != OCWDG_NUMBER_ZERO) {
        return ret;
    } else {
        /* Create the msg parser thread to parse
         * the msg coming from UART ec to ap
         */
        ret = pthread_create(&ocWdgThreadid, NULL, ocwdg_thread_comm_with_ec,
                NULL);
        if (ret != OCWDG_NUMBER_ZERO) {
            return ret;
        }
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocwdg_thread_comm_with_ec
 * Description      : Thread to send watchdog response to ec
 *                    through uart or ethernet communication
 * Input(s)         : pthreadData
 * Output(s)        :
 ******************************************************************************/
void * ocwdg_thread_comm_with_ec(void *pthreadData)
{
    OCMPMessageFrame ecMsgFrame;
    OCMPHeader ecMsgHeader;
    OCMPMessage ecCoreMsg;
    int32_t ret = OCWDG_NUMBER_ZERO;

    while (OCWDG_NUMBER_ONE) {
        /* Waiting on the  semEcWdgMsg to be released by
         * msg receiving thread from ec
         */
        ret = ocmw_sem_wait_nointr(&semEcWdgMsg);
        if (ret != OCWDG_NUMBER_ZERO) {
            perror("ocwdg_thread_comm_with_ec: ocmw_sem_wait_nointr");
            continue;
        }

        /* Frame the header packet for sending data to ec */
        ecMsgHeader.ocmpFrameLen = OCWDG_NUMBER_ZERO;
#ifdef INTERFACE_ETHERNET
        ecMsgHeader.ocmpInterface = OCMP_COMM_IFACE_ETHERNET;
#else
        ecMsgHeader.ocmpInterface = OCMP_COMM_IFACE_UART;
#endif /* INTERFACE_ETHERNET */
        ecMsgHeader.ocmpSeqNumber = OCWDG_NUMBER_ZERO;
        ecMsgHeader.ocmpSof = OCMP_MSG_SOF;
        ecMsgHeader.ocmpTimestamp = OCWDG_NUMBER_ZERO;

        /* Frame the Core packet for sending data to ec */
        ecCoreMsg.action = OCMP_AXN_TYPE_REPLY;
        ecCoreMsg.msgtype = OCMP_MSG_TYPE_WATCHDOG;
        ecCoreMsg.parameters = 0;

        /* Construct the final packet */
        ecMsgFrame.header = ecMsgHeader;
        ecMsgFrame.message = ecCoreMsg;

        /* Populate the Core packet payload */
        ecMsgFrame.message.info = (int8_t *) malloc(
                sizeof(char) * MAX_PARM_COUNT);
        if (ecMsgFrame.message.info == NULL) {
            printf("\n Memory allocation failed \n");
        }
        memset(ecMsgFrame.message.info, OCWDG_NUMBER_ZERO, MAX_PARM_COUNT);

#ifdef INTERFACE_ETHERNET
        /* Send the packetize data to ec  through ethernet*/
        ret = ocmw_send_eth_msgto_ec((int8_t *)&ecMsgFrame, (int32_t)32,
                OCMW_EC_DEV);
        if (ret != OCWDG_NUMBER_ZERO) {
            logerr ("ocmw_send_eth_msgto_ec() failed");
        }

#else
        /* Send the packetize data to ec  through uart*/
        ret = ocmw_send_uart_msg_to_ec((uint8_t *) &ecMsgFrame,
                sizeof(ecMsgFrame));
        if (ret != OCWDG_NUMBER_ZERO) {
            logerr("ocmw_send_uart_msg_to_ec() failed");
        }

#endif /* INTERFACE_ETHERNET */
        printf("Watchdog reply message sent to ec \n");
    }
}
