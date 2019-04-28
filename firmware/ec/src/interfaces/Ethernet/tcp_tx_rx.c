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
#include "inc/common/global_header.h"
#include <src/interfaces/Ethernet/tcp_tx_rx.h>

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

#define TCPPORT 1000
#define TCPHANDLERSTACK 1024
#define TCPPACKETSIZE OCMP_FRAME_TOTAL_LENGTH
#define NUMTCPWORKERS 3

Semaphore_Handle ethTxsem;
Semaphore_Handle ethRxsem;

Queue_Handle ethTxMsgQueue;
Queue_Struct ethTxMsg;

int bytesRcvd;

extern const char *destIp;
extern uint8_t numRepeat;

/* Prototypes */
Void tcpHandler(UArg arg0, UArg arg1);

/*
 *  ======== tcpWorker ========
 *  Task to handle TCP connection. Can be multiple Tasks running
 *  this function.
 */
Void tcp_Tx_Worker(UArg arg0, UArg arg1)
{
    int clientfd = (int)arg0;
    int bytesSent;
    uint8_t *buffer;
    Task_Handle task_handle = Task_self();
    LOGGER_DEBUG("tcpWorker: start clientfd = 0x%x\n", clientfd);
    while (clientfd) {
        Semaphore_pend(ethTxsem, OC_TASK_WAIT_TIME);
        {
            while (!Queue_empty(ethTxMsgQueue)) {
                buffer = Util_dequeueMsg(ethTxMsgQueue);
                bytesSent = send(clientfd, buffer, TCPPACKETSIZE, 0);
                if (bytesSent < 0) {
                    LOGGER_DEBUG("Error: send failed.\n");
                    break;
                }
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
    int clientfd = (int)arg0;
    char buffer[TCPPACKETSIZE];
    Task_Handle task_handle = Task_self();
    LOGGER_DEBUG("tcpWorker: start clientfd = 0x%x\n", clientfd);
    while ((bytesRcvd = recv(clientfd, buffer, TCPPACKETSIZE, 0)) > 0) {
        Util_enqueueMsg(gossiperRxMsgQueue, semGossiperMsg, (uint8_t *)buffer);
    }
    LOGGER_DEBUG("tcpWorker stop clientfd = 0x%x\n", clientfd);

    close(clientfd);
}

/* TODO: Hack to support basic FW update */
#include "utils/swupdate.h"
#include <xdc/runtime/Types.h>
#include <ti/sysbios/BIOS.h>
static void sw_update_cb(void)
{
    /* Gate whole system, start update */
    /* Note: Types_FreqHz contains a hi and lo 32 bit int, not sure why,
     * but we should only have to worry about the low bits
     */
    Types_FreqHz sys_clk;
    BIOS_getCpuFreq(&sys_clk);
    SoftwareUpdateBegin(sys_clk.lo);
}

/*
 *  ======== tcpHandler_client ========
 *  Create new Task to make new TCP connection and sends test data.
 */

Void tcpHandler_client(UArg arg0, UArg arg1)
{
    SOCKET lSocket;
    struct sockaddr_in sLocalAddr;
    /*Tiva operates at 120Mhz. We use delays based on this*/
    uint32_t g_ui32SysClock = 120000000;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wint-conversion"
    fdOpenSession(TaskSelf());
    lSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#pragma GCC diagnostic pop
    if (lSocket < 0) {
        LOGGER_DEBUG("tcpHandler: socket failed\n");
        Task_exit();
        return;
    } else {
        LOGGER_DEBUG(" %d socket success\n", fdError());
    }
    System_flush();
    memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
    LOGGER_DEBUG(" Ip in client: %s\n", destIp);
    sLocalAddr.sin_family = AF_INET;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
    sLocalAddr.sin_addr.s_addr = inet_addr(destIp);
#pragma GCC diagnostic pop
    sLocalAddr.sin_port = htons(arg0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
    while (connect(lSocket, (struct sockaddr *)&sLocalAddr,
                   sizeof(sLocalAddr)) < 0) {
#pragma GCC diagnostic pop
        SysCtlDelay(g_ui32SysClock / 100 / 3);
    }
    System_flush();
    /* Test pattern to be sent across the TCP connection to external server */
    char *buffer = "5A A5 5A A5 5A";
    int nbytes = 14; /* Test Pattern length */

    while (numRepeat > 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
        send(lSocket, (char *)buffer, nbytes, 0);
#pragma GCC diagnostic pop
        Task_sleep(500);
        numRepeat--;
        System_flush();
    }
    if (lSocket > 0)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
        close(lSocket);
    fdCloseSession(TaskSelf());
#pragma GCC diagnostic pop
}

/*
 *  ======== tcpHandler ========
 *  Creates new Task to handle new TCP connections.
 */
Void tcpHandler(UArg arg0, UArg arg1)
{
    int status;
    int server;
    int clientfd;
    fd_set active_fd_set, read_fd_set;
    struct timeval      timeout;
    struct sockaddr_in localAddr;
    struct sockaddr_in clientAddr;
    int optval;
    int optlen = sizeof(optval);
    socklen_t addrlen = sizeof(clientAddr);
    Task_Params task_Tx_Params;
    Task_Params task_Rx_Params;
    Error_Block eb;
    Task_Handle task_handle = Task_self();

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == -1) {
        LOGGER_DEBUG("Error: socket not created.\n");
        goto shutdown;
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(arg0);

    status = bind(server, (struct sockaddr *)&localAddr, sizeof(localAddr));
    if (status == -1) {
        LOGGER_DEBUG("Error: bind failed.\n");
        goto shutdown;
    }

    status = listen(server, NUMTCPWORKERS);
    if (status == -1) {
        LOGGER_DEBUG("Error: listen failed.\n");
        goto shutdown;
    }

    optval = 1;
    if (setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        LOGGER_DEBUG("Error: setsockopt failed\n");
        goto shutdown;
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (server, &active_fd_set);

    /* Timeout */
    timeout.tv_sec  = OC_TASK_WAIT_TIME/OC_SYS_TICK;
    timeout.tv_usec = 0;

    while (1)
    {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        System_printf("Select wait...\n");
        System_flush();
        status = select (server + 1, &read_fd_set, NULL, NULL, &timeout);
        if ( status < 0 )
        {
            System_printf("Error: Failed to create new Task\n");
            goto shutdown;
        }

        if ( status == 0 ) {
            /* Kick to watchdog */
            wd_kick(task_handle);
        } else {
            /* Service all the sockets with input pending. */
            if (FD_ISSET (server, &read_fd_set))
            {
                /* Connection request on original socket. */
                if (((clientfd = accept(server, (struct sockaddr *)&clientAddr, &addrlen)) != -1))
                {
                    LOGGER_DEBUG("tcpHandler: Creating thread clientfd = %d\n", clientfd);

                    /* Init the Error_Block */
                    Error_init(&eb);

                    /* Initialize the defaults and set the parameters. */
                    Task_Params_init(&task_Tx_Params);
                    task_Tx_Params.instance->name = "TcpTxWorker_t";
                    task_Tx_Params.arg0 = (UArg)clientfd;
                    task_Tx_Params.stackSize = 1280;
                    bool task_Tx_Handle =  Util_create_task(&task_Tx_Params, &tcp_Tx_Worker, false) ;
                    //Task_create((Task_FuncPtr)tcp_Tx_Worker, &task_Tx_Params, &eb);
                    if (task_Tx_Handle == NULL) {
                        LOGGER_DEBUG("Error: Failed to create new Task\n");
                        close(clientfd);
                    }

                    /* Initialize the defaults and set the parameters. */
                    Task_Params_init(&task_Rx_Params);
                    task_Rx_Params.instance->name="TcpRxWorker_t";
                    task_Rx_Params.arg0 = (UArg)clientfd;
                    task_Rx_Params.stackSize = 1280;
                    bool task_Rx_Handle =  Util_create_task(&task_Rx_Params, &tcp_Rx_Worker, false) ;
                    //Task_create((Task_FuncPtr)tcp_Rx_Worker, &task_Rx_Params, &eb);
                    if (task_Rx_Handle == NULL) {
                        LOGGER_DEBUG("Error: Failed to create new Task\n");
                        close(clientfd);
                    }

                    /* addrlen is a value-result param, must reset for next accept call */
                    addrlen = sizeof(clientAddr);
                }
            }
            /* In case if too busy serving request. Its good to kick watchdog */
            wd_kick(task_handle);
        }
    }

#if 0
    while ((clientfd = accept(server, (struct sockaddr *)&clientAddr,
                              &addrlen)) != -1) {
        LOGGER_DEBUG("tcpHandler: Creating thread clientfd = %d\n", clientfd);

        /* Init the Error_Block */
        Error_init(&eb);

        /* Initialize the defaults and set the parameters. */
        Task_Params_init(&task_Tx_Params);
        task_Tx_Params.instance->name = "TcpTxWorker_t";
        task_Tx_Params.arg0 = (UArg)clientfd;
        task_Tx_Params.stackSize = 1280;
        bool task_Tx_Handle =  Util_create_task(&task_Tx_Params, &tcp_Tx_Worker, false) ;
        //Task_create((Task_FuncPtr)tcp_Tx_Worker, &task_Tx_Params, &eb);
        if (task_Tx_Handle == NULL) {
            LOGGER_DEBUG("Error: Failed to create new Task\n");
            close(clientfd);
        }

        /* Initialize the defaults and set the parameters. */
        Task_Params_init(&task_Rx_Params);
        task_Rx_Params.instance->name="TcpRxWorker_t";
        task_Rx_Params.arg0 = (UArg)clientfd;
        task_Rx_Params.stackSize = 1280;
        bool task_Rx_Handle =  Util_create_task(&task_Rx_Params, &tcp_Rx_Worker, false) ;
        //Task_create((Task_FuncPtr)tcp_Rx_Worker, &task_Rx_Params, &eb);
        if (task_Rx_Handle == NULL) {
            LOGGER_DEBUG("Error: Failed to create new Task\n");
            close(clientfd);
        }

        /* addrlen is a value-result param, must reset for next accept call */
        addrlen = sizeof(clientAddr);
    }

    LOGGER_DEBUG("Error: accept failed.\n");
#endif
    shutdown:
    if (server > 0) {
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
    prepare_mdio();
#pragma GCC diagnostic pop
    return (0);
}

/*
 *  ======== netOpenHook ========
 *  NDK network open hook used to initialize IPv6
 */
void netOpenHook()
{

    Task_Params taskParams;
    Error_Block eb;

    /* Make sure Error_Block is initialized */
    Error_init(&eb);

    /*
     *  Create the Task that farms out incoming TCP connections.
     *  arg0 will be the port that this task listens to.
     */
    Task_Params_init(&taskParams);
    taskParams.instance->name = "TCPHandler_t";
    taskParams.stackSize = TCPHANDLERSTACK;
    taskParams.priority = 1;
    taskParams.arg0 = TCPPORT;
    bool taskHandle = Util_create_task(&taskParams, &tcpHandler, true);
    if (taskHandle == NULL) {
        LOGGER_DEBUG("netOpenHook: Failed to create tcpHandler Task\n");
    }

    /* Start a thread to listen for SW update packets */
    SoftwareUpdateInit(sw_update_cb);

    System_flush();
}
