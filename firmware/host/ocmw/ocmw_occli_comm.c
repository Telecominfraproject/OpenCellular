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
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <util.h>
#include <logger.h>

/* OC includes */
#include <ocmw_socket_comm.h>
#include <ocmw_occli_comm.h>
#include <ocmw_core.h>

static int32_t s_ssockFd = 0;
static struct sockaddr_in s_siServer, s_siClient;

static int32_t s_ssockfdAlert = 0;
static struct sockaddr_in s_siServerAlert;

/**************************************************************************
 * Function Name    : ocmw_init_occli_comm
 * Description      : This Function used to initialize the cli communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_init_occli_comm()
{
    int32_t ret = 0;

    /* Create socket */
    s_ssockFd = socket(OCMW_SOCK_DOMAIN, OCMW_SOCK_TYPE, OCMW_SOCK_PROTOCOL);
    if (s_ssockFd < 0) {
        ret = -errno;
        logerr("socket creation error [%d-%s]", errno, strerror(errno));
        return ret;
    }

    /* Initialize socket structure */
    memset(&s_siServer, 0, sizeof(s_siServer));
    s_siServer.sin_family = OCMW_SOCK_DOMAIN;
    s_siServer.sin_addr.s_addr = INADDR_ANY;
    s_siServer.sin_port = htons(OCMW_SOCK_SERVER_PORT);

    /* Bind host address to the socket */
    ret = bind(s_ssockFd, (struct sockaddr *)&s_siServer, sizeof(s_siServer));
    if (ret < 0) {
        ret = -errno;
        logerr("bind error [%d-%s]", errno, strerror(errno));
        return ret;
    }

    if (ret != 0) {
        ocmw_deinit_occli_comm();
        return ret;
    }

    /***** Alert related socket init *****/
    /* Create socket */
    s_ssockfdAlert =
        socket(OCMW_SOCK_DOMAIN, OCMW_SOCK_TYPE, OCMW_SOCK_PROTOCOL);
    if (s_ssockfdAlert < 0) {
        ret = -errno;
        logerr("socket creation error [%d-%s]", errno, strerror(errno));
        return ret;
    }

    if (ret != 0) {
        ocmw_deinit_occli_alert_comm();
    } else {
        /* Initialize socket structure */
        memset(&s_siServerAlert, 0, sizeof(s_siServerAlert));
        s_siServerAlert.sin_family = OCMW_SOCK_DOMAIN;
        s_siServerAlert.sin_addr.s_addr = INADDR_ANY;
        s_siServerAlert.sin_port = htons(OCMW_SOCK_SERVER_ALERT_PORT);
    }
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_deinit_occli_comm
 * Description      : This Function used to deinitialize the cli communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_deinit_occli_comm(void)
{
    close(s_ssockFd);
    s_ssockFd = 0; /* Close the IPC socket */
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocmw_deinit_occli_alert_comm
 * Description      : This Function used deinitialize the alert communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_deinit_occli_alert_comm(void)
{
    close(s_ssockfdAlert);
    s_ssockfdAlert = 0; /* Close the IPC socket */
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocmw_send_clicmd_resp_to_occli
 * Description      : This Function used to send command response to cli
 * Input(s)         : buf, buflen
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_send_clicmd_resp_to_occli(const char *buf, int32_t buflen)
{
    char errstr[OCMW_SOCKET_ERROR_SIZE];
    int32_t ret = 0;
    int32_t destLen = sizeof(s_siClient);

    /* Send the message buffer over IPC */
    ret = sendto(s_ssockFd, buf, buflen, 0,
                 (const struct sockaddr *)&s_siClient, destLen);
    if (ret == -1) {
        strerror_r(errno, errstr, OCMW_SOCKET_ERROR_SIZE);
        logerr("Error: 'sendto' [%d-%s]", errno, errstr);
    }
    return ret; /* error or number of bytes sent */
}

/**************************************************************************
 * Function Name    : ocmw_recv_clicmd_from_occli
 * Description      : This Function used to recv the command from cli
 * Input(s)         : buflen
 * Output(s)        : buf
 ***************************************************************************/
int32_t ocmw_recv_clicmd_from_occli(char *buf, int32_t buflen)
{
    char errstr[OCMW_SOCKET_ERROR_SIZE];
    int32_t ret = 0;
    int32_t destLen = sizeof(s_siClient);

    /* Receive oc messages from other processes */
    ret = recvfrom(s_ssockFd, buf, buflen, 0, (struct sockaddr *)&s_siClient,
                   (socklen_t *)&destLen);
    if (ret <= 0) {
        strerror_r(errno, errstr, OCMW_SOCKET_ERROR_SIZE);
        logerr("Error: 'recvfrom' [%d-%s]", errno, errstr);
        ret = -1;
    }
    return ret; /* error or number of bytes received */
}

/**************************************************************************
 * Function Name    : ocmw_send_alert_to_occli
 * Description      : This Function used to send alerts to cli
 * Input(s)         : buf, buflen
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_send_alert_to_occli(const char *buf, int32_t buflen)
{
    char errstr[OCMW_SOCKET_ERROR_SIZE];
    int32_t ret = 0;
    int32_t destLen = sizeof(s_siServerAlert);

    /* Send the message buffer over IPC */
    ret = sendto(s_ssockfdAlert, buf, buflen, 0,
                 (const struct sockaddr *)&s_siServerAlert, destLen);
    if (ret == -1) {
        strerror_r(errno, errstr, OCMW_SOCKET_ERROR_SIZE);
        logerr("Error: 'sendto' [%d-%s]", errno, errstr);
    }
    return ret; /* error or number of bytes sent */
}
