/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "Board.h"
#include "comm/gossiper.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/utils/util.h"

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>
#include <inc/hw_memmap.h>
#include <sys/socket.h> /* NDK BSD support */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/hal/Timer.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <string.h>
#include <stdbool.h>

#define TCPPORT         1000
#define TCPHANDLERSTACK 1024
#define TCPPACKETSIZE   OCMP_FRAME_TOTAL_LENGTH
#define NUMTCPWORKERS   3

Semaphore_Handle ethTxsem;
Semaphore_Handle ethRxsem;

Queue_Handle ethTxMsgQueue;
Queue_Struct ethTxMsg;

int bytesRcvd;

/* Prototypes */
Void tcpHandler(UArg arg0, UArg arg1);

/*
 *  ======== tcpWorker ========
 *  Task to handle TCP connection. Can be multiple Tasks running
 *  this function.
 */
Void tcp_Tx_Worker(UArg arg0, UArg arg1)
{
    int clientfd = (int) arg0;
    int bytesSent;
    uint8_t *buffer;

    System_printf("tcpWorker: start clientfd = 0x%x\n", clientfd);

    while (clientfd) {
        Semaphore_pend(ethTxsem, BIOS_WAIT_FOREVER);
        {
            buffer = Util_dequeueMsg(ethTxMsgQueue);
            bytesSent = send(clientfd, buffer, TCPPACKETSIZE, 0);
            if (bytesSent < 0) {
                System_printf("Error: send failed.\n");
                break;
            }
        }
    }
    close(clientfd);
}

/*
 *  ======== tcpWorker ========
 *  Task to handle TCP connection. Can be multiple Tasks running
 *  this function.
 */
Void tcp_Rx_Worker(UArg arg0, UArg arg1)
{
    int clientfd = (int) arg0;
    char buffer[TCPPACKETSIZE];

    System_printf("tcpWorker: start clientfd = 0x%x\n", clientfd);
    while ((bytesRcvd = recv(clientfd, buffer, TCPPACKETSIZE, 0)) > 0) {
        Util_enqueueMsg(gossiperRxMsgQueue, semGossiperMsg, (uint8_t *) buffer);
    }
    System_printf("tcpWorker stop clientfd = 0x%x\n", clientfd);

    close(clientfd);
}

/* TODO: Hack to support basic FW update */
#include "utils/swupdate.h"
#include <xdc/runtime/Types.h>
#include <ti/sysbios/BIOS.h>
static void sw_update_cb(void) {
    /* Gate whole system, start update */
    /* Note: Types_FreqHz contains a hi and lo 32 bit int, not sure why,
     * but we should only have to worry about the low bits
     */
    Types_FreqHz sys_clk;
    BIOS_getCpuFreq(&sys_clk);
    SoftwareUpdateBegin(sys_clk.lo);
}

/*
 *  ======== tcpHandler ========
 *  Creates new Task to handle new TCP connections.
 */
Void tcpHandler(UArg arg0, UArg arg1)
{
    int status;
    int clientfd;
    int server;
    struct sockaddr_in localAddr;
    struct sockaddr_in clientAddr;
    int optval;
    int optlen = sizeof(optval);
    socklen_t addrlen = sizeof(clientAddr);
    Task_Handle task_Tx_Handle;
    Task_Params task_Tx_Params;
    Task_Handle task_Rx_Handle;
    Task_Params task_Rx_Params;
    Error_Block eb;

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == -1) {
        System_printf("Error: socket not created.\n");
        goto shutdown;
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(arg0);

    status = bind(server, (struct sockaddr *) &localAddr, sizeof(localAddr));
    if (status == -1) {
        System_printf("Error: bind failed.\n");
        goto shutdown;
    }

    status = listen(server, NUMTCPWORKERS);
    if (status == -1) {
        System_printf("Error: listen failed.\n");
        goto shutdown;
    }

    optval = 1;
    if (setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        System_printf("Error: setsockopt failed\n");
        goto shutdown;
    }

    while ((clientfd = accept(server, (struct sockaddr *) &clientAddr, &addrlen))
            != -1) {

        System_printf("tcpHandler: Creating thread clientfd = %d\n", clientfd);

        /* Init the Error_Block */
        Error_init(&eb);

        /* Initialize the defaults and set the parameters. */
        Task_Params_init(&task_Tx_Params);
        task_Tx_Params.arg0 = (UArg) clientfd;
        task_Tx_Params.stackSize = 1280;
        task_Tx_Handle = Task_create((Task_FuncPtr) tcp_Tx_Worker,
                                        &task_Tx_Params, &eb);
        if (task_Tx_Handle == NULL) {
            System_printf("Error: Failed to create new Task\n");
            close(clientfd);
        }

        /* Initialize the defaults and set the parameters. */
        Task_Params_init(&task_Rx_Params);
        task_Rx_Params.arg0 = (UArg) clientfd;
        task_Rx_Params.stackSize = 1280;
        task_Rx_Handle = Task_create((Task_FuncPtr) tcp_Rx_Worker,
                                        &task_Rx_Params, &eb);
        if (task_Rx_Handle == NULL) {
            System_printf("Error: Failed to create new Task\n");
            close(clientfd);
        }

        /* addrlen is a value-result param, must reset for next accept call */
        addrlen = sizeof(clientAddr);
    }

    System_printf("Error: accept failed.\n");

    shutdown: if (server > 0) {
        close(server);
    }
}

/*****************************************************************************
 * * Below function initializes the IPC objects for BMS task
 *****************************************************************************/
void ethernet_task_init()
{
    /* Create Semaphore for TX BMS Control Message Queue */
    ethTxsem = Semaphore_create(0, NULL, NULL);

    /* Create Semaphore for RX BMS Control Message Queue */
    ethRxsem = Semaphore_create(0, NULL, NULL);

    /* Create BMS control Queue used by Big brother */
    ethTxMsgQueue = Util_constructQueue(&ethTxMsg);
}

/*
 *  ======== main ========
 */
int ethernet_start(void)
{
    ethernet_task_init();
    /*
     * Initialize the Ethernet external switch
     * For now, only thing it is doing is:
     * - Reset the Marvell switch.
     * - Make the Marvell Switch in NO CPU mode.
     */
    /* Call the EMAC init function */
    Board_initEMAC();
    /*
     * Configure necessary things for MDC/MDIO
     */
    prepare_mdio();
    return (0);
}

/*
 *  ======== netOpenHook ========
 *  NDK network open hook used to initialize IPv6
 */
void netOpenHook()
{
    Task_Handle taskHandle;
    Task_Params taskParams;
    Error_Block eb;

    /* Make sure Error_Block is initialized */
    Error_init(&eb);

    /*
     *  Create the Task that farms out incoming TCP connections.
     *  arg0 will be the port that this task listens to.
     */
    Task_Params_init(&taskParams);
    taskParams.stackSize = TCPHANDLERSTACK;
    taskParams.priority = 1;
    taskParams.arg0 = TCPPORT;
    taskHandle = Task_create((Task_FuncPtr) tcpHandler, &taskParams, &eb);
    if (taskHandle == NULL) {
        System_printf("netOpenHook: Failed to create tcpHandler Task\n");
    }

    /* Start a thread to listen for SW update packets */
    SoftwareUpdateInit(sw_update_cb);

    System_flush();
}
