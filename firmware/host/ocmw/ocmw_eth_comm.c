/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <util.h>
#include <logger.h>

/* OC includes */
#include <ocmp_frame.h>
#include <ocmw_core.h>
#include <ocmw_eth_comm.h>
#include <ocmw_socket_comm.h>

#define OCMW_ERR_STR_LEN 80
#define OCWARE_STUB_TIMEOUT_PERIOD 12

extern uint8_t mcuMsgBuf[OCMP_MSG_SIZE];

static int32_t comSockfd = 0, stub_sockfd = 0;
static struct sockaddr_in sicomServer, stub_server;

/**************************************************************************
 * Function Name    : ocmw_init_eth_comm
 * Description      : This Function used to initialize the ethernet
 *                    communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_init_eth_comm(int32_t sentDev)
{
    int32_t rc = 0;
    int32_t inetAton = 0;
    printf("Ethernet init start \n");
    struct timeval timeValObj;
    timeValObj.tv_sec = OCWARE_STUB_TIMEOUT_PERIOD;
    timeValObj.tv_usec = 0;
    if (sentDev == OCMW_EC_DEV) {
        /* Create socket */
        comSockfd = socket(OCMW_ETH_SOCK_DOMAIN, OCMW_ETH_SOCK_TYPE,
                           OCMW_ETH_SOCK_PROTOCOL);
        if (comSockfd < 0) {
            rc = -errno;
            logerr("socket creation error [%d-%s]", errno, strerror(errno));
        }

        /* Initialize socket structure */
        memset(&sicomServer, 0, sizeof(sicomServer));
        sicomServer.sin_family = OCMW_ETH_SOCK_DOMAIN;
        sicomServer.sin_port = htons(OCMW_ETH_SOCK_SERVER_PORT);
        inetAton = inet_aton(OCMW_ETH_SOCK_SERVER_IP, &sicomServer.sin_addr);
        if (inetAton == 0) {
            printf("inet_aton failed");
            rc = -1;
        }

        rc = connect(comSockfd, (struct sockaddr *)&sicomServer,
                     sizeof(sicomServer));
        if (rc != 0) {
            perror("socket connect failed:");
        }

        if (rc != 0) {
            ocmw_deinit_eth_comm(OCMW_EC_DEV);
        } else {
            printf("Ehernet init finished \n");
        }
    } else {
        /* Create socket */
        stub_sockfd = socket(OCMW_SOCK_STUB_DOMAIN, OCMW_SOCK_STUB_TYPE,
                             OCMW_SOCK_STUB_PROTOCOL);
        if (stub_sockfd < 0) {
            rc = -errno;
            logerr("socket creation error [%d-%s]", errno, strerror(errno));
        }

        /* Initialize socket structure */
        memset(&stub_server, 0, sizeof(stub_server));
        stub_server.sin_family = OCMW_SOCK_STUB_DOMAIN;
        stub_server.sin_port = htons(OCMW_SOCK_STUB_SERVER_PORT);
        stub_server.sin_addr.s_addr = inet_addr(OCMW_SOCK_SERVER_IP);
        inetAton = inet_aton(OCMW_SOCK_STUB_SERVER_IP, &stub_server.sin_addr);
        if (inetAton == 0) {
            printf("inet_aton failed");
            rc = -1;
        }

        if (setsockopt(stub_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeValObj,
                       sizeof(timeValObj)) < 0) {
            logerr("setsockopt failed");
            rc = -1;
            return rc;
        }

        if (rc != 0) {
            ocmw_deinit_eth_comm(OCMW_EC_STUB_DEV);
        } else {
            printf("Ehernet init finished \n");
        }
    }
    return rc;
}

/**************************************************************************
 * Function Name    : ocmw_deinit_eth_comm
 * Description      : This Function used to uninitialize the ethernet
 *                    communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_deinit_eth_comm(int sentDev)
{
    if (sentDev == OCMW_EC_DEV) {
        close(comSockfd);
        comSockfd = 0; /* Close the IPC socket */
    } else {
        close(stub_sockfd);
        stub_sockfd = 0; /* Close the IPC socket */
    }
    return 0;
}

/**************************************************************************
 * Function Name    : ocmw_send_eth_msgto_ec
 * Description      : This Function used to send the message to ec
 * Input(s)         : cmd, cmdlen
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_send_eth_msgto_ec(const int8_t *cmd, int32_t cmdlen, int sentDev)
{
    const int32_t errstrLen = OCMW_ERR_STR_LEN;
    char errstr[errstrLen];
    uint8_t buf[OCMP_MSG_SIZE];
    int32_t rc = 0;
    int32_t loopCount = 0;
    int32_t sendPktNonpayloadSize = 0;
    OCMPMessageFrame *ecMsgFrame;

    if (cmd == NULL) {
        logerr("Error: Memory allocation problem");
        return -1;
    }
    if (cmdlen > OCMP_MSG_SIZE) {
        logerr("Error: msgstr size is more than %d bytes", OCMP_MSG_SIZE);
        return -EMSGSIZE;
    }

    ecMsgFrame = (OCMPMessageFrame *)cmd;
    sendPktNonpayloadSize =
        (sizeof(OCMPMessage) - sizeof(void *) + sizeof(OCMPHeader));

    /* create message frame */
    memset(buf, 0, sizeof(buf));
    memcpy(buf, ecMsgFrame, sendPktNonpayloadSize);
    memcpy(&buf[sendPktNonpayloadSize], ((ecMsgFrame->message).info),
           MAX_PARM_COUNT);
#ifndef MSG_LOG
    printf(" \n send_msg_to_ec \n");
    for (loopCount = 0; loopCount < OCMP_MSG_SIZE; loopCount++) {
        printf("0x%x  ", buf[loopCount] & 0x00ff);
    }
    printf("\n");
#endif /* MSG_LOG */

    if (sentDev == OCMW_EC_DEV) {
        rc = send(comSockfd, buf, OCMP_MSG_SIZE, 0);
        if (rc < 0) {
            strerror_r(errno, errstr, errstrLen);
            logerr("Error: 'send' [%d-%s]", errno, errstr);
        }
    } else {
        rc = sendto(stub_sockfd, buf, OCMP_MSG_SIZE, 0,
                    (struct sockaddr *)&stub_server, sizeof(stub_server));
        if (rc < 0) {
            strerror_r(errno, errstr, errstrLen);
            logerr("Error: 'send' [%d-%s]", errno, errstr);
        }
    }
    return rc;
}

/**************************************************************************
 * Function Name    : ocmw_recv_eth_msgfrom_ec
 * Description      : This Function used to receive the ec message
 * Input(s)         :
 * Output(s)        : resp, resplen
 ***************************************************************************/
int32_t ocmw_recv_eth_msgfrom_ec(int8_t *resp, int32_t resplen, int sentDev)
{
#if defined(INTERFACE_STUB_EC) || defined(INTERFACE_ETHERNET)
    const int32_t errstrLen = OCMW_ERR_STR_LEN;
    char errstr[errstrLen];
#endif
    int32_t rc = 0;
    int32_t loopCount = 0;
#ifdef INTERFACE_STUB_EC
    int32_t dataLen = sizeof(stub_server);
#endif

    /*
     * Receive the CLI command execution response string from OCMW over
     * UDP socket
     */
#ifdef INTERFACE_ETHERNET
    rc = recv(comSockfd, resp, resplen, 0);
    if (rc < 0) {
        strerror_r(rc, errstr, errstrLen);
        logerr("Error: 'recv' [%d-%s]", rc, errstr);
    }
#endif
#ifdef INTERFACE_STUB_EC
    rc = recvfrom(stub_sockfd, resp, OCMP_MSG_SIZE, 0,
                  (struct sockaddr *)&stub_server, (socklen_t *)&dataLen);
    if (rc < 0) {
        strerror_r(rc, errstr, errstrLen);
        logerr("Error: 'recv' [%d-%s]", rc, errstr);
        printf("\n Error: Recv from in OCWARE_STUB");
    }
#endif

    /* Store the ethernet msg into global buffer */
    memcpy(mcuMsgBuf, resp, resplen);

    printf("Message received from ec via ethernet :\n");
    for (loopCount = 0; loopCount < resplen; loopCount++) {
        printf("0x%x  ", (0xff) & resp[loopCount]);
    }
    printf("\n");
    return rc;
}
