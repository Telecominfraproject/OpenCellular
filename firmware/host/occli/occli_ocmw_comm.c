/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <util.h>
#include <logger.h>

/* OC includes */
#include <ocmw_socket_comm.h>
#include <occli_common.h>

static int32_t s_sockFd = 0;
static int32_t s_alertSockFd = 0;
static struct sockaddr_in s_siServer, s_alertServer;
static char s_displayStr[OCCLI_SNPRINTF_MAX_LEN];

/**************************************************************************
 * Function Name    : occli_init_comm
 * Description      : This Function used to initialize the communication
 *                    between middleware code
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t occli_init_comm(void)
{
    int32_t ret = 0;
    int32_t inetAtonRet = 0;
    struct timeval timeValObj;

    timeValObj.tv_sec = OCCLI_TIMEOUT_PERIOD;
    timeValObj.tv_usec = 0;

    /* Create socket */
    s_sockFd = socket(OCMW_SOCK_DOMAIN, OCMW_SOCK_TYPE, OCMW_SOCK_PROTOCOL);
    if (s_sockFd < 0) {
        ret = -errno;
        logerr("socket creation error [%d-%s]", errno, strerror(errno));
        return ret;
    }

    /* Initialize socket structure */
    memset(&s_siServer, 0, sizeof(s_siServer));
    s_siServer.sin_family = OCMW_SOCK_DOMAIN;
    s_siServer.sin_port = htons(OCMW_SOCK_SERVER_PORT);
    inetAtonRet = inet_aton(OCMW_SOCK_SERVER_IP, &s_siServer.sin_addr);
    if (inetAtonRet == 0) {
        logerr("inet_aton failed");
        ret = FAILED;
        return ret;
    }

    if (setsockopt(s_sockFd, SOL_SOCKET, SO_RCVTIMEO, &timeValObj,
                   sizeof(timeValObj)) < 0) {
        logerr("setsockopt failed");
        ret = FAILED;
        return ret;
    }

    /* For Alert Capture */
    s_alertSockFd = socket(OCMW_SOCK_ALERT_DOMAIN, OCMW_SOCK_ALERT_TYPE,
                           OCMW_SOCK_ALERT_PROTOCOL);
    if (s_alertSockFd < 0) {
        ret = -errno;
        logerr("socket creation error [%d-%s]", errno, strerror(errno));
        return ret;
    }

    /* Initialize socket structure */
    memset(&s_alertServer, 0, sizeof(s_alertServer));
    s_alertServer.sin_family = OCMW_SOCK_ALERT_DOMAIN;
    s_alertServer.sin_port = htons(OCMW_SOCK_ALERT_SERVER_PORT);
    inetAtonRet = inet_aton(OCMW_SOCK_ALERT_SERVER_IP, &s_alertServer.sin_addr);
    if (inetAtonRet == 0) {
        logerr("inet_aton failed");
        ret = FAILED;
        return ret;
    }

    /* Bind host address to the socket */
    ret = bind(s_alertSockFd, (struct sockaddr *)&s_alertServer,
               sizeof(s_alertServer));
    if (ret < 0) {
        ret = -errno;
        logerr("bind error [%d-%s]", errno, strerror(errno));
        return ret;
    }

    return SUCCESS;
}

/**************************************************************************
 * Function Name    : occli_deinit_comm
 * Description      : This Function used to deinitialize the communication
 *                    betwwen middleware code
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int8_t occli_deinit_comm(void)
{
    /* Close the IPC socket */
    close(s_sockFd);
    s_sockFd = 0;
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : occli_send_cmd_to_ocmw
 * Description      : This Function used to send command to middleware
 * Input(s)         : cmd, cmdlen
 * Output(s)        :
 ***************************************************************************/
int32_t occli_send_cmd_to_ocmw(const char *cmd, int32_t cmdlen)
{
    char errstr[OCMW_SOCKET_ERROR_SIZE];
    int32_t ret = 0;
    int32_t strLen = sizeof(s_siServer);

    strncpy(s_displayStr, cmd, cmdlen);
    /* Send the CLI command string to OCMW over UDP socket */
    ret = sendto(s_sockFd, cmd, cmdlen, 0, (const struct sockaddr *)&s_siServer,
                 strLen);
    if (ret < 0) {
        strerror_r(errno, errstr, OCMW_SOCKET_ERROR_SIZE);
        logerr("Error: 'sendto' [%d-%s]", errno, errstr);
        printf("%s : Error : Socket error", s_displayStr);
    }
    return ret;
}

/**************************************************************************
 * Function Name    : occli_recv_cmd_resp_from_ocmw
 * Description      : This Function used to receive message from middlware
 * Input(s)         :
 * Output(s)        : resp, resplen
 ***************************************************************************/
int32_t occli_recv_cmd_resp_from_ocmw(char *resp, int32_t resplen)
{
    char errstr[OCMW_SOCKET_ERROR_SIZE];
    int32_t ret = 0;
    int32_t strLen = sizeof(s_siServer);

    /* Receive the CLI command execution response string from OCMW over UDP
     socket */
    ret = recvfrom(s_sockFd, resp, resplen, 0, (struct sockaddr *)&s_siServer,
                   (socklen_t *)&strLen);
    if (ret < 0) {
        strerror_r(errno, errstr, OCMW_SOCKET_ERROR_SIZE);
        logerr("Error: 'recvfrom' [%d-%s]", ret, errstr);
        printf("%s : Error : Timeout from Middleware", s_displayStr);
    }
    return ret;
}

/**************************************************************************
 * Function Name    : occli_recv_alertmsg_from_ocmw
 * Description      : This Function used to receive alert message from
 *                    middleware
 * Input(s)         :
 * Output(s)        : resp, resplen
 ***************************************************************************/
int32_t occli_recv_alertmsg_from_ocmw(char *resp, int32_t resplen)
{
    char errstr[OCMW_SOCKET_ERROR_SIZE];
    int32_t ret = 0;
    int32_t strLen = sizeof(s_alertServer);

    /* Receive the Alert Message string from OCMW over UDP socket */
    ret = recvfrom(s_alertSockFd, resp, resplen, 0,
                   (struct sockaddr *)&s_alertServer, (socklen_t *)&strLen);
    if (ret < 0) {
        strerror_r(errno, errstr, OCMW_SOCKET_ERROR_SIZE);
        logerr("Error: 'recvfrom' [%d-%s]", ret, errstr);
        printf("Error: 'recvfrom' [%d-%s]", ret, errstr);
    }
    return ret;
}
